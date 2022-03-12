/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CoreLights/ShadowmapPass.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>

namespace AZ
{
    namespace Render
    {        
        RPI::Ptr<Render::ShadowmapPass> ShadowmapPass::Create(const RPI::PassDescriptor& descriptor)
        {
            return aznew ShadowmapPass(descriptor);
        }

        ShadowmapPass::ShadowmapPass(const RPI::PassDescriptor& descriptor)
            : Base(descriptor)
        {
            m_overrideViewportState = true;
            m_overrideScissorSate = true;
        }

        RPI::Ptr<Render::ShadowmapPass> ShadowmapPass::CreateWithPassRequest(const Name& passName, AZStd::shared_ptr<RPI::RasterPassData> passData)
        {
            // Create a pass request for the descriptor so we can connect it to the parent class input connections
            RPI::PassRequest childRequest;
            childRequest.m_templateName = Name{ "ShadowmapPassTemplate" };
            childRequest.m_passName = passName;

            // Add a connection to the skinned mesh input
            RPI::PassConnection passConnection;
            passConnection.m_localSlot = Name{ "SkinnedMeshes" };
            passConnection.m_attachmentRef.m_pass = Name{ "Parent" };
            passConnection.m_attachmentRef.m_attachment = Name{ "SkinnedMeshes" };
            childRequest.m_connections.emplace_back(passConnection);

            // Get the template
            AZStd::shared_ptr<RPI::PassTemplate> childTemplate = RPI::PassSystemInterface::Get()->GetPassTemplate(childRequest.m_templateName);
            AZ_Assert(childTemplate, "ShadowmapPass::CreateWIthPassRequest - attempting to create a shadowmap pass before the template has been created.");

            // Create the pass
            RPI::PassDescriptor descriptor{ passName, childTemplate, &childRequest };
            descriptor.m_passData = AZStd::move(passData);
            return Create(descriptor);
        }

        void ShadowmapPass::SetArraySlice(uint16_t arraySlice)
        {
            m_arraySlice = arraySlice;
        }

        void ShadowmapPass::CreatePassTemplate()
        {
            AZStd::shared_ptr<RPI::PassTemplate> m_childTemplate = AZStd::make_shared<RPI::PassTemplate>();
            m_childTemplate->m_name = Name{ "ShadowmapPassTemplate" };
            m_childTemplate->m_passClass = "ShadowmapPass";

            m_childTemplate->m_slots.resize(2);
            RPI::PassSlot& slot = m_childTemplate->m_slots[0];
            slot.m_name = Name{ "Shadowmap" };
            slot.m_slotType = RPI::PassSlotType::Output;
            slot.m_scopeAttachmentUsage = RHI::ScopeAttachmentUsage::DepthStencil;

            // This slot it used to create a connection between the skinned mesh compute pass and the cascade shadow map pass
            RPI::PassSlot& skinnedMeshSlot = m_childTemplate->m_slots[1];
            skinnedMeshSlot.m_name = Name{ "SkinnedMeshes" };
            skinnedMeshSlot.m_slotType = RPI::PassSlotType::Input;
            skinnedMeshSlot.m_scopeAttachmentUsage = RHI::ScopeAttachmentUsage::InputAssembly;

            m_childTemplate->m_connections.resize(1);
            RPI::PassConnection& connection = m_childTemplate->m_connections[0];
            connection.m_localSlot = Name{ "Shadowmap" };
            connection.m_attachmentRef.m_pass = Name{ "Parent" };
            connection.m_attachmentRef.m_attachment = Name{ "Shadowmap" };

            RPI::PassSystemInterface::Get()->AddPassTemplate(Name{ "ShadowmapPassTemplate" }, m_childTemplate);
        }

        void ShadowmapPass::SetClearEnabled(bool enabled)
        {
            m_clearEnabled = enabled;
        }

        void ShadowmapPass::SetViewportScissorFromImageSize(const RHI::Size& imageSize)
        {
            const RHI::Viewport viewport(
                0.f, imageSize.m_width * 1.f,
                0.f, imageSize.m_height * 1.f);
            const RHI::Scissor scissor(
                0, 0, imageSize.m_width, imageSize.m_height);
            SetViewportScissor(viewport, scissor);
        }

        void ShadowmapPass::SetViewportScissor(const RHI::Viewport& viewport, const RHI::Scissor& scissor)
        {
            m_viewportState = viewport;
            m_scissorState = scissor;
        }

        void ShadowmapPass::BuildInternal()
        {
            RPI::Ptr<RPI::ParentPass> parentPass = GetParent();
            if (!parentPass)
            {
                return;
            }

            RHI::ImageViewDescriptor imageViewDescriptor;
            imageViewDescriptor.m_arraySliceMin = m_arraySlice;
            imageViewDescriptor.m_arraySliceMax = m_arraySlice;

            RPI::PassAttachmentBinding& binding = GetOutputBinding(0);

            // [GFX TODO][ATOM-2470] stop caring about attachment
            RPI::Ptr<RPI::PassAttachment> attachment = parentPass->GetOwnedAttachment(Name("ShadowmapImage"));
            if (!attachment)
            {
                AZ_Assert(false, "[ShadowmapPass %s] Cannot find shadowmap image attachment.", GetPathName().GetCStr());
                return;
            }
            const RHI::AttachmentId attachmentId = attachment->GetAttachmentId();

            RHI::AttachmentLoadStoreAction action;
            action.m_clearValue = RHI::ClearValue::CreateDepth(1.f);
            action.m_loadAction = m_clearEnabled ? RHI::AttachmentLoadAction::Clear : RHI::AttachmentLoadAction::DontCare;
            binding.m_unifiedScopeDesc = RHI::UnifiedScopeAttachmentDescriptor(attachmentId, imageViewDescriptor, action);

            Base::BuildInternal();
        }

      } // namespace Render
} // namespace AZ
