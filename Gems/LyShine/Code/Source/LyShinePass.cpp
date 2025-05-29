/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Debug/Trace.h>
#include <Atom/RHI/DrawListTagRegistry.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/Pass/PassAttachment.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Image/AttachmentImagePool.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Reflect/Pass/RasterPassData.h>
#include <AzCore/std/iterator.h>

#include "LyShinePass.h"

namespace LyShine
{
    AZ::RPI::Ptr<LyShinePass> LyShinePass::Create(const AZ::RPI::PassDescriptor& descriptor)
    {
        return aznew LyShinePass(descriptor);
    }

    LyShinePass::LyShinePass(const AZ::RPI::PassDescriptor& descriptor)
        : Base(descriptor)
    {
    }

    LyShinePass::~LyShinePass()
    {
        LyShinePassRequestBus::Handler::BusDisconnect();
    }

    void LyShinePass::ResetInternal()
    {
        LyShinePassRequestBus::Handler::BusDisconnect();

        Base::ResetInternal();
    }

    void LyShinePass::SetRenderPipeline(AZ::RPI::RenderPipeline* pipeline)
    {
        if (pipeline == nullptr)
        {
            // The pipeline being set to null means this pass will soon be destroyed. Disconnect from the bus so if a
            // new LyShinePass is being created to replace it, it will be able to connect.
            LyShinePassRequestBus::Handler::BusDisconnect();
        }
        ParentPass::SetRenderPipeline(pipeline);
    }

    void LyShinePass::BuildInternal()
    {
        AZ::RPI::Scene* scene = GetScene();
        if (scene)
        {
            // Listen for rebuild requests
            LyShinePassRequestBus::Handler::BusConnect(scene->GetId());
        }

        // Always recreate children when rebuild the pass
        m_flags.m_createChildren = true;

        Base::BuildInternal();
    }

    void LyShinePass::CreateChildPassesInternal()
    {
        AZ::RPI::Scene* scene = GetScene();
        if (scene)
        {
            // Get the current list of render targets being used across all loaded UI Canvases
            LyShine::AttachmentImagesAndDependencies attachmentImagesAndDependencies;
            LyShinePassDataRequestBus::EventResult(
                attachmentImagesAndDependencies,
                scene->GetId(),
                &LyShinePassDataRequestBus::Events::GetRenderTargets
            );

            AddRttChildPasses(attachmentImagesAndDependencies);
            AddUiCanvasChildPass(attachmentImagesAndDependencies);
        }
    }

    void LyShinePass::RebuildRttChildren()
    {
        QueueForBuildAndInitialization();
    }

    AZ::RPI::RasterPass* LyShinePass::GetRttPass(const AZStd::string& name)
    {
        for (auto child:m_children)
        {
            if (child->GetName() == AZ::Name(name))
            {
                return azrtti_cast<AZ::RPI::RasterPass*>(child.get());
            }
        }
        return nullptr;
    }

    AZ::RPI::RasterPass* LyShinePass::GetUiCanvasPass()
    {
        return m_uiCanvasChildPass.get();
    }

    void LyShinePass::AddRttChildPasses(LyShine::AttachmentImagesAndDependencies attachmentImagesAndDependencies)
    {
        for (const auto& attachmentImageAndDependencies : attachmentImagesAndDependencies)
        {
            AddRttChildPass(attachmentImageAndDependencies.first, attachmentImageAndDependencies.second);
        }
    }

    void LyShinePass::AddRttChildPass(AZ::Data::Instance<AZ::RPI::AttachmentImage> attachmentImage, AttachmentImages attachmentImageDependencies)
    {
        // Add a pass that renders to the specified texture
        AZ::RPI::PassSystemInterface* passSystem = AZ::RPI::PassSystemInterface::Get();
        if (attachmentImage)
        {
            auto passName = attachmentImage->GetRHIImage()->GetName(); // Use attachment name (but not attachment id) as pass name so the pass can be found by GetRttPass() function
            AZ::RPI::Ptr<RttChildPass> rttChildPass = azrtti_cast<RttChildPass*>(passSystem->CreatePassFromTemplate(AZ::Name("RttChildPassTemplate"), passName).get());
            AZ_Assert(rttChildPass, "[LyShinePass] Unable to create a RttChildPass.");

            // Store the info needed to attach to slots and set up frame graph dependencies
            rttChildPass->m_attachmentImage = attachmentImage;
            rttChildPass->m_attachmentImageDependencies = attachmentImageDependencies;

            // Disable by default, the RenderGraph will enable it when render to render target
            rttChildPass->SetEnabled(false);

            AddChild(rttChildPass);
        }
    }

    void LyShinePass::AddUiCanvasChildPass(LyShine::AttachmentImagesAndDependencies AttachmentImagesAndDependencies)
    {
        if (!m_uiCanvasChildPass)
        {
            AZ::RPI::PassSystemInterface* passSystem = AZ::RPI::PassSystemInterface::Get();
            m_uiCanvasChildPass = azrtti_cast<LyShineChildPass*>(passSystem->CreatePassFromTemplate(AZ::Name("LyShineChildPassTemplate"), AZ::Name("LyShineChildPass")).get());
            AZ_Assert(m_uiCanvasChildPass, "[LyShinePass] Unable to create a LyShineChildPass.");
        }

        // Store the info needed to set up frame graph dependencies
        m_uiCanvasChildPass->m_attachmentImageDependencies.clear();
        for (const auto& attachmentImageAndDescendents : AttachmentImagesAndDependencies)
        {
            m_uiCanvasChildPass->m_attachmentImageDependencies.emplace_back(attachmentImageAndDescendents.first);
        }

        AddChild(m_uiCanvasChildPass);
    }

    AZ::RPI::Ptr<LyShineChildPass> LyShineChildPass::Create(const AZ::RPI::PassDescriptor& descriptor)
    {
        return aznew LyShineChildPass(descriptor);
    }

    LyShineChildPass::LyShineChildPass(const AZ::RPI::PassDescriptor& descriptor)
        : RasterPass(descriptor)
    {
    }

    LyShineChildPass::~LyShineChildPass()
    {
    }

    void LyShineChildPass::SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph)
    {
        AZ::RPI::RasterPass::SetupFrameGraphDependencies(frameGraph);

        for (auto attachmentImage : m_attachmentImageDependencies)
        {
            // Ensure that the image is imported into the attachment database.
            // The image may not be imported if the owning pass has been disabled.
            auto attachmentImageId = attachmentImage->GetAttachmentId();
            if (!frameGraph.GetAttachmentDatabase().IsAttachmentValid(attachmentImageId))
            {
                frameGraph.GetAttachmentDatabase().ImportImage(attachmentImageId, attachmentImage->GetRHIImage());
            }

            AZ::RHI::ImageScopeAttachmentDescriptor desc;
            desc.m_attachmentId = attachmentImageId;
            desc.m_imageViewDescriptor = attachmentImage->GetImageView()->GetDescriptor();
            desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

            frameGraph.UseShaderAttachment(desc, AZ::RHI::ScopeAttachmentAccess::Read, AZ::RHI::ScopeAttachmentStage::FragmentShader);
        }
    }

    AZ::RPI::Ptr<RttChildPass> RttChildPass::Create(const AZ::RPI::PassDescriptor& descriptor)
    {
        return aznew RttChildPass(descriptor);
    }

    RttChildPass::RttChildPass(const AZ::RPI::PassDescriptor& descriptor)
        : LyShineChildPass(descriptor)
    {
    }

    RttChildPass::~RttChildPass()
    {
    }

    void RttChildPass::BuildInternal()
    {
        AttachImageToSlot(AZ::Name("RenderTargetOutput"), m_attachmentImage);

        auto imageSize = m_attachmentImage->GetDescriptor().m_size;
        // use render target's size to setup override scissor and viewport
        m_scissorState = AZ::RHI::Scissor(0, 0, imageSize.m_width, imageSize.m_height);
        m_viewportState = AZ::RHI::Viewport(0.f, aznumeric_cast<float>(imageSize.m_width), 0.f, aznumeric_cast<float>(imageSize.m_height));
        m_overrideScissorSate = false;
        m_overrideViewportState = false;
    }
} // namespace LyShine
