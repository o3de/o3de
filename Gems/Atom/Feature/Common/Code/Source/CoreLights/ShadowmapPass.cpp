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
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/View.h>

namespace AZ
{
    namespace Render
    {
        // --- Pass Creation ---

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
            const AZStd::shared_ptr<const RPI::PassTemplate> childTemplate = RPI::PassSystemInterface::Get()->GetPassTemplate(childRequest.m_templateName);
            AZ_Assert(childTemplate, "ShadowmapPass::CreateWIthPassRequest - attempting to create a shadowmap pass before the template has been created.");

            // Create the pass
            RPI::PassDescriptor descriptor{ passName, childTemplate, &childRequest };
            descriptor.m_passData = AZStd::move(passData);
            return Create(descriptor);
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

        // --- Build Override ---

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

            RPI::Ptr<RPI::PassAttachment> attachment = parentPass->GetOutputBinding(0).GetAttachment();
            if (!attachment)
            {
                AZ_Assert(false, "[ShadowmapPass %s] Cannot find shadowmap image attachment.", GetPathName().GetCStr());
                return;
            }
            const RHI::AttachmentId attachmentId = attachment->GetAttachmentId();

            RHI::AttachmentLoadStoreAction action;
            action.m_clearValue = RHI::ClearValue::CreateDepth(1.f);
            action.m_loadAction = m_clearEnabled ? RHI::AttachmentLoadAction::Clear : RHI::AttachmentLoadAction::DontCare;
            action.m_loadActionStencil = RHI::AttachmentLoadAction::None;
            action.m_storeActionStencil = RHI::AttachmentStoreAction::None;
            binding.m_unifiedScopeDesc = RHI::UnifiedScopeAttachmentDescriptor(attachmentId, imageViewDescriptor, action);

            Base::BuildInternal();
        }

        // --- Setters ---

        void ShadowmapPass::SetArraySlice(uint16_t arraySlice)
        {
            m_arraySlice = arraySlice;
        }

        void ShadowmapPass::SetClearEnabled(bool enabled)
        {
            m_clearEnabled = enabled;
        }

        void ShadowmapPass::SetIsStatic(bool isStatic)
        {
            m_isStatic = isStatic;
        }

        void ShadowmapPass::SetCasterMovedBit(RHI::Handle<uint32_t> bit)
        {
            m_casterMovedBit = bit;
        }

        void ShadowmapPass::ForceRenderNextFrame()
        {
            m_forceRenderNextFrame = true;
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

        void ShadowmapPass::SetClearShadowDrawPacket(AZ::RHI::ConstPtr<RHI::DrawPacket> clearShadowDrawPacket)
        {
            m_clearShadowDrawPacket = clearShadowDrawPacket;
            m_clearShadowDrawItemProperties = clearShadowDrawPacket->GetDrawItemProperties(0);
        }

        void ShadowmapPass::UpdatePipelineViewTag(const RPI::PipelineViewTag& viewTag)
        {
            SetPipelineViewTag(viewTag);
        }
        
        void ShadowmapPass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            Base::SetupFrameGraphDependencies(frameGraph);

            // Override the estimated item count set by the base class. Draw item count is compared against
            // the last frame to detect cases where a moving object leaves the shadow frustum. It wouldn't
            // set m_casterMovedBit since its outside the view, but needs to trigger a re-render anyway.
            if (m_isStatic && !m_forceRenderNextFrame && m_lastFrameDrawCount == m_drawItemCount)
            {
                const auto& views = m_pipeline->GetViews(GetPipelineViewTag());
                if (!views.empty())
                {
                    const RPI::ViewPtr& view = views.front();
                    if (view && (view->GetOrFlags() & m_casterMovedBit.GetIndex()) == 0)
                    {
                        // Shadow is static and no casters moved since last frame.
                        frameGraph.SetEstimatedItemCount(0);
                    }
                }
            }
            else
            {
                // Report + 1 to make room for the clear draw packet.
                frameGraph.SetEstimatedItemCount(static_cast<uint32_t>(m_drawListView.size() + 1));
            }
        }

        void ShadowmapPass::SubmitDrawItems(const RHI::FrameGraphExecuteContext& context, uint32_t startIndex, uint32_t endIndex, uint32_t offset) const
        {
            if (m_clearShadowDrawPacket)
            {
                if (startIndex == 0)
                {
                    RHI::CommandList* commandList = context.GetCommandList();
                    commandList->Submit(m_clearShadowDrawPacket->GetDrawItemProperties(0).m_item->GetDeviceDrawItem(context.GetDeviceIndex()), 0);
                }
                else
                {
                    // Only decrement startIndex if startIndex is greater than 0. This means that RasterPass will submit one less
                    // draw item for the first batch since the clear draw was submitted in this batch.
                    --startIndex; 
                }
                // RasterPass's draw item indices need to be adjusted to make sure it stays inside its expected bounds.
                --endIndex;
                ++offset;
            }
            Base::SubmitDrawItems(context, startIndex, endIndex, offset);
        }

        void ShadowmapPass::FrameEndInternal()
        {
            if (m_clearShadowDrawPacket)
            {
                // If m_clearShadowDrawPacket is valid, then this pass would have rendered this frame if
                // m_forceRenderNextFrame was set to true, so set it to false now. This can't be done safely
                // in SubmitDrawItems because it may be called multiple times from different threads.
                m_forceRenderNextFrame = false;
            }
            m_lastFrameDrawCount = m_drawItemCount;
            Base::FrameEndInternal();
        }

    } // namespace Render
} // namespace AZ
