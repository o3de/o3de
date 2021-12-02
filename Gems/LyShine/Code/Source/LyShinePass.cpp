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

    void LyShinePass::BuildInternal()
    {
        AZ::RPI::Scene* scene = GetScene();
        if (scene)
        {
            // Listen for rebuild requests
            LyShinePassRequestBus::Handler::BusConnect(scene->GetId());

            RemoveChildren();

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

        Base::BuildInternal();
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

        // Create a pass template
        auto passTemplate = AZStd::make_shared<AZ::RPI::PassTemplate>();
        passTemplate->m_name = "RttChildPass";
        passTemplate->m_passClass = AZ::Name("RttChildPass");

        // Slots
        passTemplate->m_slots.resize(2);

        AZ::RPI::PassSlot& depthInOutSlot = passTemplate->m_slots[0];
        depthInOutSlot.m_name = "DepthInputOutput";
        depthInOutSlot.m_slotType = AZ::RPI::PassSlotType::InputOutput;
        depthInOutSlot.m_scopeAttachmentUsage = AZ::RHI::ScopeAttachmentUsage::DepthStencil;
        depthInOutSlot.m_loadStoreAction.m_clearValue = AZ::RHI::ClearValue::CreateDepthStencil(0.0f, 0);
        depthInOutSlot.m_loadStoreAction.m_loadActionStencil = AZ::RHI::AttachmentLoadAction::Clear;

        AZ::RPI::PassSlot& outSlot = passTemplate->m_slots[1];
        outSlot.m_name = AZ::Name("RenderTargetOutput");
        outSlot.m_slotType = AZ::RPI::PassSlotType::Output;
        outSlot.m_scopeAttachmentUsage = AZ::RHI::ScopeAttachmentUsage::RenderTarget;
        outSlot.m_loadStoreAction.m_clearValue = AZ::RHI::ClearValue::CreateVector4Float(0.0f, 0.0f, 0.0f, 0.0f);
        outSlot.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Clear;

        // Connections
        passTemplate->m_connections.resize(1);

        AZ::RPI::PassConnection& depthInOutConnection = passTemplate->m_connections[0];
        depthInOutConnection.m_localSlot = "DepthInputOutput";
        depthInOutConnection.m_attachmentRef.m_pass = "Parent";
        depthInOutConnection.m_attachmentRef.m_attachment = "DepthInputOutput";

        // Pass data
        AZStd::shared_ptr<AZ::RPI::RasterPassData> passData = AZStd::make_shared<AZ::RPI::RasterPassData>();
        passData->m_drawListTag = AZ::Name("uicanvas");
        passData->m_pipelineViewTag = AZ::Name("MainCamera");
        auto size = attachmentImage->GetRHIImage()->GetDescriptor().m_size;
        passData->m_overrideScissor = AZ::RHI::Scissor(0, 0, size.m_width, size.m_height);
        passData->m_overrideViewport = AZ::RHI::Viewport(0, static_cast<float>(size.m_width), 0, static_cast<float>(size.m_height));
        passTemplate->m_passData = AZStd::move(passData);
        // Create a pass descriptor for the new child pass
        AZ::RPI::PassDescriptor childDesc;
        childDesc.m_passTemplate = passTemplate;
        childDesc.m_passName = attachmentImage->GetAttachmentId();

        AZ::RPI::PassSystemInterface* passSystem = AZ::RPI::PassSystemInterface::Get();
        AZ::RPI::Ptr<RttChildPass> rttChildPass = passSystem->CreatePass<RttChildPass>(childDesc);
        AZ_Assert(rttChildPass, "[LyShinePass] Unable to create %s.", passTemplate->m_name.GetCStr());

        // Store the info needed to attach to slots and set up frame graph dependencies
        rttChildPass->m_attachmentImage = attachmentImage;
        rttChildPass->m_attachmentImageDependencies = attachmentImageDependencies;

        AddChild(rttChildPass);
    }

    void LyShinePass::AddUiCanvasChildPass(LyShine::AttachmentImagesAndDependencies AttachmentImagesAndDependencies)
    {
        if (!m_uiCanvasChildPass)
        {
            // Create a pass template
            auto passTemplate = AZStd::make_shared<AZ::RPI::PassTemplate>();
            passTemplate->m_name = AZ::Name("LyShineChildPass");
            passTemplate->m_passClass = AZ::Name("LyShineChildPass");

            // Slots
            passTemplate->m_slots.resize(2);

            AZ::RPI::PassSlot& depthInOutSlot = passTemplate->m_slots[0];
            depthInOutSlot.m_name = "DepthInputOutput";
            depthInOutSlot.m_slotType = AZ::RPI::PassSlotType::InputOutput;
            depthInOutSlot.m_scopeAttachmentUsage = AZ::RHI::ScopeAttachmentUsage::DepthStencil;
            depthInOutSlot.m_loadStoreAction.m_clearValue = AZ::RHI::ClearValue::CreateDepthStencil(0.0f, 0);
            depthInOutSlot.m_loadStoreAction.m_loadActionStencil = AZ::RHI::AttachmentLoadAction::Clear;

            AZ::RPI::PassSlot& inOutSlot = passTemplate->m_slots[1];
            inOutSlot.m_name = "ColorInputOutput";
            inOutSlot.m_slotType = AZ::RPI::PassSlotType::InputOutput;
            inOutSlot.m_scopeAttachmentUsage = AZ::RHI::ScopeAttachmentUsage::RenderTarget;

            // Connections
            passTemplate->m_connections.resize(2);

            AZ::RPI::PassConnection& depthInOutConnection = passTemplate->m_connections[0];
            depthInOutConnection.m_localSlot = "DepthInputOutput";
            depthInOutConnection.m_attachmentRef.m_pass = "Parent";
            depthInOutConnection.m_attachmentRef.m_attachment = "DepthInputOutput";

            AZ::RPI::PassConnection& inOutConnection = passTemplate->m_connections[1];
            inOutConnection.m_localSlot = "ColorInputOutput";
            inOutConnection.m_attachmentRef.m_pass = "Parent";
            inOutConnection.m_attachmentRef.m_attachment = "ColorInputOutput";

            // Pass data
            AZStd::shared_ptr<AZ::RPI::RasterPassData> passData = AZStd::make_shared<AZ::RPI::RasterPassData>();
            passData->m_drawListTag = AZ::Name("uicanvas");
            passData->m_pipelineViewTag = AZ::Name("MainCamera");
            passTemplate->m_passData = AZStd::move(passData);

            // Create a pass descriptor for the new child pass
            AZ::RPI::PassDescriptor childDesc;
            childDesc.m_passTemplate = passTemplate;
            childDesc.m_passName = AZ::Name("LyShineChildPass");

            AZ::RPI::PassSystemInterface* passSystem = AZ::RPI::PassSystemInterface::Get();
            m_uiCanvasChildPass = passSystem->CreatePass<LyShineChildPass>(childDesc);
            AZ_Assert(m_uiCanvasChildPass, "[LyShinePass] Unable to create %s.", passTemplate->m_name.GetCStr());
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

            frameGraph.UseShaderAttachment(desc, AZ::RHI::ScopeAttachmentAccess::Read);
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
    }
} // namespace LyShine
