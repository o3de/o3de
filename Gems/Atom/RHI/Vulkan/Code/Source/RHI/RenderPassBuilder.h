/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/AttachmentId.h>
#include <Atom/RHI.Reflect/ClearValue.h>
#include <RHI/RenderPass.h>
#include <RHI/Framebuffer.h>
#include <RHI/Scope.h>

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/array.h>

namespace AZ
{
    namespace Vulkan
    {
        class Device;
        class Scope;

        struct RenderPassContext
        {
            RHI::Ptr<Framebuffer> m_framebuffer;
            RHI::Ptr<RenderPass> m_renderPass;
            AZStd::vector<RHI::ClearValue> m_clearValues;

            bool IsValid() const;
        };

        //! Utility class that builds a renderpass and a framebuffer from the
        //! resource attachments of a Scope. It uses the load and store actions and the clear colors
        //! to build the proper renderpass that will be used for rendering.
        class RenderPassBuilder
        {
        public:
            RenderPassBuilder(Device& device, uint32_t subpassesCount);
            ~RenderPassBuilder() = default;

            //! Adds the attachments that are used by the Scope into the renderpass descriptor.
            void AddScopeAttachments(const Scope& scope);

            //! Builds the renderpass and framebuffer from the information collected from the scopes.
            RHI::ResultCode End(RenderPassContext& builtContext);

        private:
            // Adds the subpass dependencies for a resource view that is being used in a subpass.
            // It will use the information about barriers (prologue and epilogue) that the scope contains.
            template<class T>
            void AddResourceDependency(uint32_t subpassIndex, const Scope& scope, const T* resourceView);

            // Inserts a subpass dependency between two subpasses.
            void AddSubpassDependency(uint32_t srcSubpass, uint32_t dstSubpass, const Scope::Barrier& barrier);

            VkImageLayout GetInitialLayout(const Scope& scope, const RHI::ImageScopeAttachment& attachment) const;
            VkImageLayout GetFinalLayout(const Scope& scope, const RHI::ImageScopeAttachment& attachment) const;

            Device& m_device;
            AZStd::unordered_map<RHI::AttachmentId, uint32_t> m_attachmentsMap;
            AZStd::unordered_map<const RHI::ResourceView*, uint32_t> m_lastSubpassResourceUse;
            RenderPass::Descriptor m_renderpassDesc;
            Framebuffer::Descriptor m_framebufferDesc;
            AZStd::vector<RHI::ClearValue> m_clearValues;
            AZStd::vector<AZStd::bitset<RHI::Limits::Pipeline::RenderAttachmentCountMax>> m_usedAttachmentsPerSubpass;
            uint32_t m_subpassCount = 0;
        };

        template<class T>
        void RenderPassBuilder::AddResourceDependency(uint32_t subpassIndex, const Scope& scope, const T* resourceView)
        {
            const AZStd::vector<Scope::Barrier>& prologueBarriers = scope.m_subpassBarriers[static_cast<uint32_t>(Scope::BarrierSlot::Prologue)];
            const AZStd::vector<Scope::Barrier>& epilogueBarriers = scope.m_subpassBarriers[static_cast<uint32_t>(Scope::BarrierSlot::Epilogue)];

            auto prologueBarrierFoundIter = AZStd::find_if(prologueBarriers.begin(), prologueBarriers.end(), [resourceView](const Scope::Barrier& barrier)
            {
                return barrier.BlocksResource(*resourceView, Scope::OverlapType::Partial);
            });

            auto lastUseIter = m_lastSubpassResourceUse.find(resourceView);
            if (prologueBarrierFoundIter != prologueBarriers.end())
            {
                uint32_t sourceSubpassIndex = lastUseIter != m_lastSubpassResourceUse.end() ? lastUseIter->second : VK_SUBPASS_EXTERNAL;
                AddSubpassDependency(sourceSubpassIndex, subpassIndex, *prologueBarrierFoundIter);
            }

            auto epilogueBarrierFoundIter = AZStd::find_if(epilogueBarriers.begin(), epilogueBarriers.end(), [resourceView](const Scope::Barrier& barrier)
            {
                return barrier.BlocksResource(*resourceView, Scope::OverlapType::Partial);
            });

            if (epilogueBarrierFoundIter != epilogueBarriers.end())
            {
                uint32_t destinationSubpassIndex = subpassIndex == m_subpassCount - 1 ? VK_SUBPASS_EXTERNAL : subpassIndex + 1;
                AddSubpassDependency(subpassIndex, destinationSubpassIndex, *epilogueBarrierFoundIter);
            }
        }
    }
}
