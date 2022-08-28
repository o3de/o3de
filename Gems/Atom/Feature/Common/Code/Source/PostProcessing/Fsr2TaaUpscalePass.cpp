/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <PostProcessing/Fsr2TaaUpscalePass.h>

#include <AzCore/Math/SimdMath.h>
#include <AzCore/Console/Console.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FrameGraphBuilder.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/View.h>

AZ_CVAR_EXTERNED(float, r_renderScaleMin);

AZ_CVAR(
    bool,
    r_fsr2SharpeningEnabled,
    true,
    nullptr,
    AZ::ConsoleFunctorFlags::DontReplicate,
    "Set to enable FSR2's built-in contrast adaptive sharpening.");

AZ_CVAR(
    float,
    r_fsr2SharpeningStrength,
    0.8f,
    nullptr,
    AZ::ConsoleFunctorFlags::DontReplicate,
    "If FSR2 sharpening is enabled, modulates the strenght of the sharpening.");

namespace AZ::Render
{
    void Fsr2TaaUpscalePassData::Reflect(ReflectContext* context)
    {
        if (auto* sc = azrtti_cast<SerializeContext*>(context))
        {
            sc->Class<Fsr2TaaUpscalePassData, RPI::PassData>()
                ->Version(1)
                ->Field("DepthInverted", &Fsr2TaaUpscalePassData::m_depthInverted)
                ->Field("DepthInfinite", &Fsr2TaaUpscalePassData::m_depthInfinite)
                ->Field("AutoExposure", &Fsr2TaaUpscalePassData::m_autoExposure)
                ->Field("DynamicResolution", &Fsr2TaaUpscalePassData::m_dynamicResolution);
        }
    }

    RPI::Ptr<Fsr2TaaUpscalePass> Fsr2TaaUpscalePass::Create(const RPI::PassDescriptor& descriptor)
    {
        return { aznew Fsr2TaaUpscalePass{ descriptor } };
    }

    Fsr2TaaUpscalePass::Fsr2TaaUpscalePass(const RPI::PassDescriptor& descriptor)
        : RPI::Pass{ descriptor }
    {
        SetHardwareQueueClass(RHI::HardwareQueueClass::Graphics);

        if (!RHI::RHISystemInterface::Get()->GetDevice()->HasFsr2Support())
        {
            SetEnabled(false);
        }
    }

    Fsr2TaaUpscalePass::~Fsr2TaaUpscalePass()
    {
        if (IsFsr2ContextValid())
        {
            ffxFsr2ContextDestroy(&m_fsr2Context);
        }
    }

    void Fsr2TaaUpscalePass::MaybeCreateFsr2Context()
    {
        // Populate the FSR2 context description and determine if it matches the context
        // description used last frame. If not, destroy the previous context and recreate
        // it. On the first frame, fsr2Desc.device is nullptr so this context will always
        // be initialized properly before use.

        FfxFsr2ContextDescription fsr2Desc{};

        const Fsr2TaaUpscalePassData* passData = RPI::PassUtils::GetPassData<Fsr2TaaUpscalePassData>(GetPassDescriptor());

        fsr2Desc.flags |= FFX_FSR2_ENABLE_HIGH_DYNAMIC_RANGE;

        if (passData->m_depthInverted)
        {
            fsr2Desc.flags |= FFX_FSR2_ENABLE_DEPTH_INVERTED;
        }
        if (passData->m_depthInfinite)
        {
            fsr2Desc.flags |= FFX_FSR2_ENABLE_DEPTH_INFINITE;
        }
        if (passData->m_autoExposure)
        {
            fsr2Desc.flags |= FFX_FSR2_ENABLE_AUTO_EXPOSURE;
        }
        if (passData->m_dynamicResolution)
        {
            fsr2Desc.flags |= FFX_FSR2_ENABLE_DYNAMIC_RESOLUTION;
        }

        RHI::ImageDescriptor& outputColorDesc = m_outputColor->GetAttachment()->m_descriptor.m_image;
        fsr2Desc.displaySize.width = outputColorDesc.m_size.m_width;
        fsr2Desc.displaySize.height = outputColorDesc.m_size.m_height;

        float invRenderScaleMin = 1.f / r_renderScaleMin;
        fsr2Desc.maxRenderSize.width = aznumeric_cast<uint32_t>(invRenderScaleMin * fsr2Desc.displaySize.width);
        fsr2Desc.maxRenderSize.height = aznumeric_cast<uint32_t>(invRenderScaleMin * fsr2Desc.displaySize.height);

        // Check if the flags or render/display dimensions since the last time the context was created (or
        // if the context was created at all) and create the FSR2 context if needed.
        if (memcmp(&fsr2Desc, &m_fsr2ContextDesc, sizeof(FfxFsr2ContextDescription)) != 0)
        {
            if (IsFsr2ContextValid())
            {
                ffxFsr2ContextDestroy(&m_fsr2Context);
            }

            m_lastFrameTimeMS = RHI::RHISystemInterface::Get()->GetCpuFrameTime();

            AZ_Printf("Fsr2TaaUpscalePass", "Creating FSR2 context");
            m_fsr2ContextDesc = fsr2Desc;
            RHI::ResultCode result = RHI::RHISystemInterface::Get()->GetDevice()->CreateFsr2Context(m_fsr2Context, fsr2Desc);
            if (result != RHI::ResultCode::Success)
            {
                // A 0 display size on the FSR2 context is a sentinel for failed context creation
                m_fsr2ContextDesc.displaySize.width = 0;

                AZ_Error("Fsr2TaaUpscalePass", false, "Failed to create FSR2 context");
            }
        }
    }

    void Fsr2TaaUpscalePass::ResetInternal()
    {
        m_frameCount = 0u;
    }

    void Fsr2TaaUpscalePass::BuildInternal()
    {
        m_inputColor = FindAttachmentBinding(Name{ "InputColor" });
        AZ_Error("Fsr2TaaUpscalePass", m_inputColor, "Fsr2TaaUpscalePass missing InputColor attachment.");

        m_inputDepth = FindAttachmentBinding(Name{ "InputDepth" });
        AZ_Error("Fsr2TaaUpscalePass", m_inputDepth, "Fsr2TaaUpscalePass missing InputDepth attachment.");

        m_inputMotionVectors = FindAttachmentBinding(Name{ "MotionVectors" });
        AZ_Error("Fsr2TaaUpscalePass", m_inputMotionVectors, "Fsr2TaaUpscalePass missing MotionVectors attachment.");

        m_outputColor = FindAttachmentBinding(Name{ "OutputColor" });
        AZ_Error("Fsr2TaaUpscalePass", m_outputColor, "Fsr2TaaUpscalePass missing OutputColor attachment.");

        // Synchronize the attachment size with the pipeline output
        RPI::Ptr<RPI::PassAttachment> output = FindOwnedAttachment(Name{ "OutputColor" });
        output->Update();
        m_outputColor->SetAttachment(output);

        MaybeCreateFsr2Context();
    }

    void Fsr2TaaUpscalePass::FrameBeginInternal(FramePrepareParams params)
    {
        if (GetScopeId().IsEmpty())
        {
            InitScope(RHI::ScopeId(GetPathName()), RHI::HardwareQueueClass::Graphics);
        }

        params.m_frameGraphBuilder->ImportScopeProducer(*this);

        RHI::ImageDescriptor& inputColorDesc = m_inputColor->GetAttachment()->m_descriptor.m_image;
        m_fsr2DispatchDesc.renderSize.width = inputColorDesc.m_size.m_width;
        m_fsr2DispatchDesc.renderSize.height = inputColorDesc.m_size.m_height;

        // The motion vector scale * motion vector produces a vector with a length given in pixel units.
        // Negation is needed because we compute motion as (current clip position - previous clip position)
        // but FSR2 expects the additive inverse.
        m_fsr2DispatchDesc.motionVectorScale.x = aznumeric_cast<float>(m_fsr2DispatchDesc.renderSize.width);
        m_fsr2DispatchDesc.motionVectorScale.y = aznumeric_cast<float>(m_fsr2DispatchDesc.renderSize.height);

        // Determine the jitter offset based on current render scale. The sequence length grows as a function
        // of the upscale amount. The FSR2 provided jitter sequence is a Halton sequence (outputs are in
        // unit pixel space).
        int jitterPhaseCount = ffxFsr2GetJitterPhaseCount(m_fsr2DispatchDesc.renderSize.width, m_fsr2ContextDesc.displaySize.width);
        ffxFsr2GetJitterOffset(&m_fsr2DispatchDesc.jitterOffset.x, &m_fsr2DispatchDesc.jitterOffset.y, ++m_frameCount, jitterPhaseCount);

        // Transfer from [-0.5, 0.5] to [-1/w, 1/w] in x and [-1/h, 1/h] in y
        Vector2 clipSpaceJitter{ 2.f * m_fsr2DispatchDesc.jitterOffset.x / m_fsr2DispatchDesc.renderSize.width,
                                 -2.f * m_fsr2DispatchDesc.jitterOffset.y / m_fsr2DispatchDesc.renderSize.height };

        RPI::ViewPtr view = GetRenderPipeline()->GetDefaultView();
        view->SetClipSpaceOffset(-clipSpaceJitter.GetX(), -clipSpaceJitter.GetY());
        // FSR2 expects negative velocities in the motion vector buffer (current - previous clip position)
        view->SetMotionVectorScale(-1.f, -1.f);

        Vector2 nearFar = view->GetClipNearFar();
        // TODO: Fix this when the near and far planes are actually fixed in the view
        m_fsr2DispatchDesc.cameraNear = nearFar.GetY();
        m_fsr2DispatchDesc.cameraFar = nearFar.GetX();

        const AZ::Matrix4x4& viewToClip = view->GetViewToClipMatrix();
        // TODO: this is an ugly way to compute FOV (assumes perspective RH view-to-clip matrix construction)
        float viewToClip_2_2 = viewToClip.GetRow(1).GetY();
        m_fsr2DispatchDesc.cameraFovAngleVertical = 2.f / AZ::Atan(viewToClip_2_2);

        m_fsr2DispatchDesc.enableSharpening = r_fsr2SharpeningEnabled;
        m_fsr2DispatchDesc.sharpness = r_fsr2SharpeningStrength;

        double currentTimeMS = RHI::RHISystemInterface::Get()->GetCpuFrameTime();
        m_fsr2DispatchDesc.frameTimeDelta = aznumeric_cast<float>(currentTimeMS - m_lastFrameTimeMS);
        m_lastFrameTimeMS = currentTimeMS;

        m_fsr2DispatchDesc.reset = false;

        // TODO: Investigate if we preexposed values before writing to 16bit color target
        m_fsr2DispatchDesc.preExposure = 1.f;
    }

    void Fsr2TaaUpscalePass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
    {
        frameGraph.UseAttachment(
            m_inputColor->m_unifiedScopeDesc.GetAsImage(), m_inputColor->GetAttachmentAccess(), RHI::ScopeAttachmentUsage::Shader);

        frameGraph.UseAttachment(
            m_inputDepth->m_unifiedScopeDesc.GetAsImage(), m_inputDepth->GetAttachmentAccess(), RHI::ScopeAttachmentUsage::Shader);

        frameGraph.UseAttachment(
            m_inputMotionVectors->m_unifiedScopeDesc.GetAsImage(),
            m_inputMotionVectors->GetAttachmentAccess(),
            RHI::ScopeAttachmentUsage::Shader);

        frameGraph.UseAttachment(
            m_outputColor->m_unifiedScopeDesc.GetAsImage(), m_outputColor->GetAttachmentAccess(), RHI::ScopeAttachmentUsage::Shader);

        for (RPI::Pass* pass : m_executeAfterPasses)
        {
            if (Fsr2TaaUpscalePass* fsr2Pass = azrtti_cast<Fsr2TaaUpscalePass*>(pass))
            {
                frameGraph.ExecuteAfter(fsr2Pass->GetScopeId());
            }
        }

        for (RPI::Pass* pass : m_executeBeforePasses)
        {
            if (Fsr2TaaUpscalePass* fsr2Pass = azrtti_cast<Fsr2TaaUpscalePass*>(pass))
            {
                frameGraph.ExecuteBefore(fsr2Pass->GetScopeId());
            }
        }
    }

    void Fsr2TaaUpscalePass::CompileResources(const RHI::FrameGraphCompileContext& context)
    {
        // Retrieve dependencies allocated as resources for this pass to forward in the
        // subsequent dispatch

        RHI::Device* device = RHI::RHISystemInterface::Get()->GetDevice();

        const RHI::Image* inputColor = context.GetImage(m_inputColor->GetAttachment()->GetAttachmentId());
        device->PopulateFsr2Resource(m_fsr2Context, m_fsr2DispatchDesc.color, *inputColor, L"FSR2 Input Color", false);

        const RHI::Image* inputDepth = context.GetImage(m_inputDepth->GetAttachment()->GetAttachmentId());
        device->PopulateFsr2Resource(m_fsr2Context, m_fsr2DispatchDesc.depth, *inputDepth, L"FSR2 Input Depth", false);

        const RHI::Image* inputMotionVectors = context.GetImage(m_inputMotionVectors->GetAttachment()->GetAttachmentId());
        device->PopulateFsr2Resource(
            m_fsr2Context, m_fsr2DispatchDesc.motionVectors, *inputMotionVectors, L"FSR2 Input Motion Vectors", false);

        const RHI::Image* outputColor = context.GetImage(m_outputColor->GetAttachment()->GetAttachmentId());
        device->PopulateFsr2Resource(m_fsr2Context, m_fsr2DispatchDesc.output, *outputColor, L"FSR2 Output Color", true);
    }

    void Fsr2TaaUpscalePass::BuildCommandList(const RHI::FrameGraphExecuteContext& context)
    {
        context.GetCommandList()->Submit(m_fsr2Context, m_fsr2DispatchDesc);
    }

    bool Fsr2TaaUpscalePass::IsFsr2ContextValid() const
    {
        return m_fsr2ContextDesc.displaySize.width != 0;
    }
} // namespace AZ::Render
