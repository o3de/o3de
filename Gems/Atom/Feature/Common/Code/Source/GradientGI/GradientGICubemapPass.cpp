/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "GradientGICubemapPass.h"

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/DispatchItem.h>
#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI/FrameGraphCompileContext.h>
#include <Atom/RHI/FrameGraphExecuteContext.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Reflect/Pass/PassDescriptor.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Name/Name.h>

namespace AZ::Render
{
    // =========================================================================
    // Platform Support
    // =========================================================================

    /*static*/ bool GradientGICubemapPass::IsGpuComputeSupported()
    {
        AZ_TracePrintf("GradientGI", "Pass::IsGpuComputeSupported() probing...\n");

        auto* imageSystem = RPI::ImageSystemInterface::Get();
        if (!imageSystem)
        {
            AZ_TracePrintf("GradientGI", "  FAIL: ImageSystemInterface is NULL\n");
            return false;
        }

        auto* attachmentPool = imageSystem->GetSystemAttachmentPool().get();
        if (!attachmentPool)
        {
            AZ_TracePrintf("GradientGI", "  FAIL: SystemAttachmentPool is NULL\n");
            return false;
        }

        RHI::ImageDescriptor testDesc = RHI::ImageDescriptor::CreateCubemap(
            RHI::ImageBindFlags::ShaderRead | RHI::ImageBindFlags::ShaderWrite,
            /*width=*/4,
            RHI::Format::R16G16B16A16_FLOAT);

        auto testImage = RPI::AttachmentImage::Create(
            *attachmentPool, testDesc, AZ::Name("GradientGI_CapProbe"), nullptr, nullptr);

        bool supported = (testImage != nullptr);
        AZ_TracePrintf("GradientGI", "  Result: %s\n", supported ? "SUPPORTED" : "NOT SUPPORTED");
        return supported;
    }

    // =========================================================================
    // Lifecycle
    // =========================================================================

    GradientGICubemapPass::GradientGICubemapPass(const RPI::PassDescriptor& descriptor)
        : RPI::RenderPass(descriptor)
    {
    }

    // =========================================================================
    // Public API
    // =========================================================================

    void GradientGICubemapPass::SetGradientColors(
        const Color& low, const Color& mid, const Color& high,
        float exposure, uint32_t faceSize)
    {
        AZ_TracePrintf("GradientGI", "Pass::SetGradientColors: low=(%.2f,%.2f,%.2f) mid=(%.2f,%.2f,%.2f) high=(%.2f,%.2f,%.2f) exp=%.2f face=%u\n",
            low.GetR(), low.GetG(), low.GetB(),
            mid.GetR(), mid.GetG(), mid.GetB(),
            high.GetR(), high.GetG(), high.GetB(),
            exposure, faceSize);

        m_lowColor  = low;
        m_midColor  = mid;
        m_highColor = high;
        m_exposure  = exposure;
        m_faceSize  = faceSize;
        m_dirty     = true;
    }

    Data::Instance<RPI::AttachmentImage> GradientGICubemapPass::GetCubemapImage() const
    {
        return m_cubemapImage;
    }

    // =========================================================================
    // BuildInternal -- load shader, create pipeline state, SRG, and output image
    // =========================================================================

    void GradientGICubemapPass::BuildInternal()
    {
        AZ_TracePrintf("GradientGI", "=== Pass::BuildInternal() BEGIN === faceSize=%u\n", m_faceSize);

        // ---- Load shader asset ----
        const char* shaderPath = "Shaders/GradientGI/GradientGICubemap.azshader";
        AZ_TracePrintf("GradientGI", "  [1/5] Loading shader: %s\n", shaderPath);
        auto shaderAsset = RPI::AssetUtils::LoadCriticalAsset<RPI::ShaderAsset>(shaderPath);
        if (!shaderAsset.IsReady())
        {
            AZ_Error("GradientGICubemapPass", false, "Failed to load shader: %s", shaderPath);
            AZ_TracePrintf("GradientGI", "  FATAL: Shader asset not ready. Check Asset Processor for GradientGICubemap.\n");
            SetEnabled(false);
            return;
        }
        AZ_TracePrintf("GradientGI", "  [1/5] Shader asset loaded OK (assetId=%s)\n",
            shaderAsset.GetId().ToString<AZStd::string>().c_str());

        m_shader = RPI::Shader::FindOrCreate(shaderAsset);
        if (!m_shader)
        {
            AZ_Error("GradientGICubemapPass", false, "Failed to create Shader instance from: %s", shaderPath);
            SetEnabled(false);
            return;
        }
        AZ_TracePrintf("GradientGI", "  [1/5] Shader instance created OK\n");

        // ---- Build compute pipeline state ----
        AZ_TracePrintf("GradientGI", "  [2/5] Building compute pipeline state...\n");
        auto& shaderVariant = m_shader->GetVariant(RPI::ShaderAsset::RootShaderVariantStableId);
        RHI::PipelineStateDescriptorForDispatch pipelineDesc;
        shaderVariant.ConfigurePipelineState(pipelineDesc);
        m_pipelineState = m_shader->AcquirePipelineState(pipelineDesc);
        if (!m_pipelineState)
        {
            AZ_Error("GradientGICubemapPass", false, "Failed to acquire pipeline state for GradientGICubemap.");
            SetEnabled(false);
            return;
        }
        AZ_TracePrintf("GradientGI", "  [2/5] Pipeline state acquired OK: %p\n", m_pipelineState);

        // ---- Create pass SRG (SRG_PerPass = "PassSrg") ----
        AZ_TracePrintf("GradientGI", "  [3/5] Creating PassSrg...\n");
        auto srgLayout = m_shader->FindShaderResourceGroupLayout(AZ::Name("PassSrg"));
        if (srgLayout)
        {
            AZ_TracePrintf("GradientGI", "  [3/5] PassSrg layout found\n");
            m_passSrg = RPI::ShaderResourceGroup::Create(
                m_shader->GetAsset(),
                m_shader->GetSupervariantIndex(),
                srgLayout->GetName());

            AZ_Error("GradientGICubemapPass", m_passSrg, "Failed to create PassSrg for GradientGICubemap.");
            if (m_passSrg)
            {
                AZ_TracePrintf("GradientGI", "  [3/5] PassSrg created OK: %p\n", m_passSrg.get());
            }
        }
        else
        {
            AZ_Error("GradientGICubemapPass", false, "Could not find PassSrg layout in GradientGICubemap shader.");
            SetEnabled(false);
            return;
        }

        // ---- Create persistent output cubemap AttachmentImage ----
        AZ_TracePrintf("GradientGI", "  [4/5] Creating AttachmentImage (cubemap %ux%u, R16G16B16A16_FLOAT)...\n",
            m_faceSize, m_faceSize);
        auto* imageSystem = RPI::ImageSystemInterface::Get();
        auto* attachmentPool = imageSystem ? imageSystem->GetSystemAttachmentPool().get() : nullptr;
        if (!attachmentPool)
        {
            AZ_Error("GradientGICubemapPass", false, "System attachment pool not available.");
            SetEnabled(false);
            return;
        }

        RHI::ImageDescriptor imageDesc = RHI::ImageDescriptor::CreateCubemap(
            RHI::ImageBindFlags::ShaderRead | RHI::ImageBindFlags::ShaderWrite,
            m_faceSize,
            RHI::Format::R16G16B16A16_FLOAT);

        auto cubemapViewDesc = RHI::ImageViewDescriptor::CreateCubemap();

        m_cubemapImage = RPI::AttachmentImage::Create(
            *attachmentPool, imageDesc,
            AZ::Name("GradientGI_Cubemap"),
            nullptr,
            &cubemapViewDesc);

        if (!m_cubemapImage)
        {
            AZ_Error("GradientGICubemapPass", false,
                "Failed to create AttachmentImage for gradient cubemap. "
                "GPU compute (UAV cubemap) is not supported on this platform.");
            SetEnabled(false);
            return;
        }
        AZ_TracePrintf("GradientGI", "  [4/5] AttachmentImage created OK: %p, attachmentId=%s\n",
            m_cubemapImage.get(), m_cubemapImage->GetAttachmentId().GetCStr());

        // ---- Verify default image view (cubemap SRV) ----
        const RHI::ImageView* defaultView = m_cubemapImage->GetImageView();
        AZ_TracePrintf("GradientGI", "  [5/5] Default ImageView (cubemap SRV): %p\n", defaultView);
        if (!defaultView)
        {
            AZ_Warning("GradientGI", false, "AttachmentImage has NO default ImageView! Scene SRG binding will fail.");
        }

        m_dirty = true;
        AZ_TracePrintf("GradientGI", "=== Pass::BuildInternal() END === SUCCESS\n");
    }

    // =========================================================================
    // SetupFrameGraphDependencies -- import image and declare UAV scope each frame
    // =========================================================================

    void GradientGICubemapPass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
    {
        RPI::RenderPass::SetupFrameGraphDependencies(frameGraph);

        if (!m_cubemapImage)
        {
            if (m_diagnosticLogFrameGraph)
            {
                AZ_TracePrintf("GradientGI", "Pass::SetupFrameGraphDependencies() SKIP: cubemapImage is NULL\n");
                m_diagnosticLogFrameGraph = false;
            }
            return;
        }

        if (m_diagnosticLogFrameGraph)
        {
            AZ_TracePrintf("GradientGI", "Pass::SetupFrameGraphDependencies(): importing image %s, declaring UAV 2DArray[0..5]\n",
                m_cubemapImage->GetAttachmentId().GetCStr());
            m_diagnosticLogFrameGraph = false;
        }

        frameGraph.GetAttachmentDatabase().ImportImage(
            m_cubemapImage->GetAttachmentId(),
            m_cubemapImage->GetRHIImage());

        RHI::ImageScopeAttachmentDescriptor uavDesc;
        uavDesc.m_attachmentId = m_cubemapImage->GetAttachmentId();
        uavDesc.m_imageViewDescriptor = RHI::ImageViewDescriptor::Create(
            RHI::Format::Unknown,
            /*mipSliceMin=*/0,
            /*mipSliceMax=*/0,
            /*arraySliceMin=*/0,
            /*arraySliceMax=*/5);
        uavDesc.m_loadStoreAction.m_loadAction  = RHI::AttachmentLoadAction::DontCare;
        uavDesc.m_loadStoreAction.m_storeAction = RHI::AttachmentStoreAction::Store;

        frameGraph.UseShaderAttachment(uavDesc, RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::ComputeShader);
    }

    // =========================================================================
    // CompileResources -- bind SRG constants and UAV image view
    // =========================================================================

    void GradientGICubemapPass::CompileResources(const RHI::FrameGraphCompileContext& context)
    {
        if (!m_passSrg)
        {
            AZ_TracePrintf("GradientGI", "Pass::CompileResources() SKIP: passSrg is NULL\n");
            return;
        }

        if (m_dirty)
        {
            AZ_TracePrintf("GradientGI", "Pass::CompileResources() DIRTY: binding SRG constants + UAV\n");

            // ---- Bind gradient color constants ----
            const auto lowIdx   = m_passSrg->FindShaderInputConstantIndex(AZ::Name("m_lowColor"));
            const auto midIdx   = m_passSrg->FindShaderInputConstantIndex(AZ::Name("m_midColor"));
            const auto highIdx  = m_passSrg->FindShaderInputConstantIndex(AZ::Name("m_highColor"));
            const auto expIdx   = m_passSrg->FindShaderInputConstantIndex(AZ::Name("m_exposure"));
            const auto sizeIdx  = m_passSrg->FindShaderInputConstantIndex(AZ::Name("m_faceSize"));
            const auto imgIdx   = m_passSrg->FindShaderInputImageIndex(AZ::Name("m_outputCubemap"));

            AZ_TracePrintf("GradientGI", "  SRG indices: low=%d(%d) mid=%d(%d) high=%d(%d) exp=%d(%d) size=%d(%d) img=%d(%d)\n",
                lowIdx.GetIndex(), lowIdx.IsValid(),
                midIdx.GetIndex(), midIdx.IsValid(),
                highIdx.GetIndex(), highIdx.IsValid(),
                expIdx.GetIndex(), expIdx.IsValid(),
                sizeIdx.GetIndex(), sizeIdx.IsValid(),
                imgIdx.GetIndex(), imgIdx.IsValid());

            m_passSrg->SetConstant(lowIdx,  AZ::Vector3(m_lowColor.GetR(),  m_lowColor.GetG(),  m_lowColor.GetB()));
            m_passSrg->SetConstant(midIdx,  AZ::Vector3(m_midColor.GetR(),  m_midColor.GetG(),  m_midColor.GetB()));
            m_passSrg->SetConstant(highIdx, AZ::Vector3(m_highColor.GetR(), m_highColor.GetG(), m_highColor.GetB()));
            m_passSrg->SetConstant(expIdx,  m_exposure);
            m_passSrg->SetConstant(sizeIdx, m_faceSize);

            // ---- Bind the UAV image view from the frame graph (2DArray, not cubemap) ----
            if (m_cubemapImage)
            {
                const RHI::ImageView* uavView = context.GetImageView(m_cubemapImage->GetAttachmentId());
                AZ_TracePrintf("GradientGI", "  UAV image view from frame graph: %p (attachmentId=%s)\n",
                    uavView, m_cubemapImage->GetAttachmentId().GetCStr());
                if (uavView)
                {
                    m_passSrg->SetImageView(imgIdx, uavView);
                }
                else
                {
                    AZ_Warning("GradientGI", false, "  UAV image view is NULL! Frame graph did not provide view for attachment.");
                }
            }
        }

        m_passSrg->Compile();
    }

    // =========================================================================
    // BuildCommandListInternal -- dispatch compute when dirty
    // =========================================================================

    void GradientGICubemapPass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
    {
        if (!m_dirty || !m_pipelineState || !m_passSrg)
        {
            if (m_diagnosticLogDispatch)
            {
                AZ_TracePrintf("GradientGI", "Pass::BuildCommandListInternal() SKIP: dirty=%d, pipeline=%p, srg=%p\n",
                    m_dirty, m_pipelineState, m_passSrg.get());
                m_diagnosticLogDispatch = false;
            }
            return;
        }

        const uint32_t groupsX = (m_faceSize + 7) / 8;
        const uint32_t groupsY = (m_faceSize + 7) / 8;
        const uint32_t groupsZ = 6;

        AZ_TracePrintf("GradientGI", "Pass::BuildCommandListInternal() DISPATCHING: groups=(%u,%u,%u), faceSize=%u, deviceIdx=%d\n",
            groupsX, groupsY, groupsZ, m_faceSize, context.GetDeviceIndex());

        RHI::DispatchDirect directArgs;
        directArgs.m_totalNumberOfThreadsX = groupsX * 8;
        directArgs.m_totalNumberOfThreadsY = groupsY * 8;
        directArgs.m_totalNumberOfThreadsZ = groupsZ;
        directArgs.m_threadsPerGroupX      = 8;
        directArgs.m_threadsPerGroupY      = 8;
        directArgs.m_threadsPerGroupZ      = 1;

        RHI::DispatchItem dispatchItem(RHI::MultiDevice::AllDevices);
        dispatchItem.SetArguments(RHI::DispatchArguments(directArgs));
        dispatchItem.SetPipelineState(m_pipelineState);

        const RHI::ShaderResourceGroup* srgRhi = m_passSrg->GetRHIShaderResourceGroup();
        AZ_TracePrintf("GradientGI", "  RHI SRG: %p\n", srgRhi);
        dispatchItem.SetShaderResourceGroups(AZStd::span<const RHI::ShaderResourceGroup*>(&srgRhi, 1));

        context.GetCommandList()->Submit(
            dispatchItem.GetDeviceDispatchItem(context.GetDeviceIndex()));

        AZ_TracePrintf("GradientGI", "  Dispatch submitted OK. Clearing dirty flag.\n");
        m_dirty = false;
    }

} // namespace AZ::Render
