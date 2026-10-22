/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: MIT
 */

#include "SceneColorViewCapturePass.h"
#include "SceneColorViewFeatureProcessor.h"
#include <SceneColorView/CaptureSize.h>
#include <Atom/RHI/FrameGraphBuilder.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Image/AttachmentImagePool.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Reflect/Pass/PassAttachmentReflect.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/std/string/string.h>

namespace SceneColorView
{
    namespace
    {
        AZ::RHI::ImageDescriptor MakeDescriptor(uint32_t width, uint32_t height)
        {
            return AZ::RHI::ImageDescriptor::Create2D(
                AZ::RHI::ImageBindFlags::Color | AZ::RHI::ImageBindFlags::ShaderRead,
                width, height, AZ::RHI::Format::R16G16B16A16_FLOAT);
        }

        AZ::RPI::Ptr<AZ::RPI::PassAttachment> MakeAttachment(
            const AZ::RHI::ImageDescriptor& imageDescriptor, const AZ::Name& path,
            const AZ::Data::Instance<AZ::RPI::AttachmentImage>& image)
        {
            AZ::RPI::PassImageAttachmentDesc description;
            description.m_name = AZ::Name("SceneColorViewImage");
            description.m_imageDescriptor = imageDescriptor;
            description.m_lifetime = image ? AZ::RHI::AttachmentLifetimeType::Imported : AZ::RHI::AttachmentLifetimeType::Transient;
            AZ::RPI::Ptr<AZ::RPI::PassAttachment> attachment = aznew AZ::RPI::PassAttachment(description);
            attachment->ComputePathName(path);
            // This output is explicitly imported in FrameBeginInternal instead of
            // the base pass's owned-attachment list: resizing must happen first.
            if (image)
            {
                attachment->m_path = image->GetAttachmentId();
                attachment->m_importedResource = image;
            }
            return attachment;
        }
    }

    AZ::RPI::Ptr<SceneColorViewCapturePass> SceneColorViewCapturePass::Create(const AZ::RPI::PassDescriptor& descriptor)
    {
        return aznew SceneColorViewCapturePass(descriptor);
    }

    SceneColorViewCapturePass::SceneColorViewCapturePass(const AZ::RPI::PassDescriptor& descriptor)
        : FullscreenTrianglePass(descriptor)
    {
    }

    void SceneColorViewCapturePass::BuildInternal()
    {
        FullscreenTrianglePass::BuildInternal();
        // A descriptor is needed when FullscreenTriangle initializes its output
        // pipeline state, even while capture is disabled. No GPU image is allocated
        // for this placeholder. The fixed RGBA16F format also makes resizes safe.
        const auto descriptor = m_image ? m_image->GetRHIImage()->GetDescriptor() : MakeDescriptor(1, 1);
        m_captureAttachment = MakeAttachment(descriptor, GetPathName(), m_image);
        GetOutputBinding(0).SetOriginalAttachment(m_captureAttachment);
    }

    bool SceneColorViewCapturePass::EnsureImage(uint32_t width, uint32_t height)
    {
        if (m_image)
        {
            const auto& size = m_image->GetRHIImage()->GetDescriptor().m_size;
            if (size.m_width == width && size.m_height == height) { return true; }
        }
        auto* images = AZ::RPI::ImageSystemInterface::Get();
        if (!images || !images->GetSystemAttachmentPool()) { return false; }
        const auto descriptor = MakeDescriptor(width, height);
        auto image = AZ::RPI::AttachmentImage::Create(
            *images->GetSystemAttachmentPool(), descriptor,
            AZ::Name(AZStd::string::format("SceneColorView_%p", static_cast<void*>(this))));
        if (!image)
        {
            AZ_Error("SceneColorView", false, "Could not allocate the scene-color capture image.");
            return false;
        }
        m_image = image;
        m_captureAttachment = MakeAttachment(descriptor, GetPathName(), m_image);
        GetOutputBinding(0).SetOriginalAttachment(m_captureAttachment);
        AZ_Printf("SceneColorView", "Allocated %u x %u RGBA16F capture for %s.\n", width, height, GetPathName().GetCStr());
        return true;
    }

    void SceneColorViewCapturePass::SetConsumerAttachment(AZ::RPI::Ptr<AZ::RPI::PassAttachment> attachment)
    {
        if (!m_consumer) { return; }
        const AZ::Name name("SceneColorViewReadDependency");
        if (!attachment)
        {
            // SetAttachment(nullptr) is not allowed by the engine's implementation.
            if (auto* binding = m_consumer->FindAttachmentBinding(name)) { binding->Clear(); }
            return;
        }
        if (!m_consumer->FindAttachmentBinding(name))
        {
            AZ::RPI::PassSlot slot;
            slot.m_name = name;
            slot.m_slotType = AZ::RPI::PassSlotType::Input;
            slot.m_shaderInputName = AZ::Name("NoBind");
            slot.m_scopeAttachmentUsage = AZ::RHI::ScopeAttachmentUsage::Shader;
            slot.m_scopeAttachmentStage = AZ::RHI::ScopeAttachmentStage::FragmentShader;
            AZ::RPI::PassAttachmentBinding binding(slot);
            binding.m_shaderInputIndex = AZ::RPI::PassAttachmentBinding::ShaderInputNoBind;
            m_consumer->AddAttachmentBinding(binding);
        }
        auto* binding = m_consumer->FindAttachmentBinding(name);
        // Reapply each frame and after pass rebuilds. This declares the read for
        // barriers/lifetime, without adding anything to the forward PassSrg.
        binding->SetOriginalAttachment(attachment);
    }

    void SceneColorViewCapturePass::FrameBeginInternal(FramePrepareParams params)
    {
        SetConsumerAttachment(nullptr);
        auto* fp = m_pipeline && m_pipeline->GetScene()
            ? m_pipeline->GetScene()->GetFeatureProcessor<SceneColorViewFeatureProcessor>() : nullptr;
        const auto view = GetView();
        auto srg = view ? view->GetShaderResourceGroup() : nullptr;
        if (!fp || !fp->GetConfiguration().m_enabled || !srg || !m_consumer)
        {
            if (m_image)
            {
                m_image = nullptr;
                m_captureAttachment = MakeAttachment(MakeDescriptor(1, 1), GetPathName(), {});
                GetOutputBinding(0).SetOriginalAttachment(m_captureAttachment);
            }
            // No base call: no scope producer/draw is submitted while off. The
            // feature processor has already supplied the black fallback.
            return;
        }

        const auto imageIndex = srg->FindShaderInputImageIndex(AZ::Name("m_sceneColorViewImage"));
        const auto validIndex = srg->FindShaderInputConstantIndex(AZ::Name("m_sceneColorViewAvailable"));
        const auto texelIndex = srg->FindShaderInputConstantIndex(AZ::Name("m_sceneColorViewTexelSize"));
        if (!imageIndex.IsValid() || !validIndex.IsValid() || !texelIndex.IsValid()) { return; }

        // A ViewSrg is shared by every pass using that View. If that very same
        // View object occurs in another pipeline, reuse its FIRST capture and
        // declare a read dependency instead of overwriting the per-view binding.
        if (const auto* existing = fp->FindCapture(view.get()))
        {
            SetConsumerAttachment(existing->m_attachment);
            return;
        }

        auto* sourceBinding = FindAttachmentBinding(AZ::Name("Source"));
        auto source = sourceBinding ? sourceBinding->GetAttachment() : nullptr;
        if (!source || source->GetAttachmentType() != AZ::RHI::AttachmentType::Image) { return; }
        const auto& sourceDescriptor = source->m_descriptor.m_image;
        if (sourceDescriptor.m_multisampleState.m_samples != 1 || sourceDescriptor.m_arraySize != 1)
        {
            if (!m_reportedUnsupported)
            {
                AZ_Warning("SceneColorView", false, "Capture requires a resolved, single-layer desktop scene-color input; MSAA/array inputs are unsupported.");
                m_reportedUnsupported = true;
            }
            return;
        }
        const auto size = CalculateCaptureSize(sourceDescriptor.m_size.m_width, sourceDescriptor.m_size.m_height,
            static_cast<AZ::u32>(fp->GetConfiguration().m_resolutionDivisor));
        if (!EnsureImage(size.m_width, size.m_height)) { return; }

        auto database = params.m_frameGraphBuilder->GetAttachmentDatabase();
        if (!database.FindAttachment(m_captureAttachment->GetAttachmentId()))
        {
            database.ImportImage(m_captureAttachment->GetAttachmentId(), m_image->GetRHIImage());
        }
        SetConsumerAttachment(m_captureAttachment);
        srg->SetImage(imageIndex, m_image);
        srg->SetConstant(validIndex, AZ::u32(1));
        srg->SetConstant(texelIndex, AZ::Vector2(1.0f / size.m_width, 1.0f / size.m_height));
        // Critical: do NOT call srg->Compile(). View::UpdateSrg queues its one
        // normal compile after PassSystem::FrameUpdate finishes.
        fp->PublishCapture(view.get(), {m_image, m_captureAttachment});
        FullscreenTrianglePass::FrameBeginInternal(params);
    }
}
