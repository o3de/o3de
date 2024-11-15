/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Metal/Metal.h>
#include <AzCore/std/containers/unordered_map.h>

namespace AZ::RHI
{
    class SwapChainFrameAttachment;
}

namespace AZ::Metal
{
    class Scope;
    //! Encapsulates renderpass information used by scopes when executing their work.
    struct RenderPassContext
    {
        //! Renderpass object
        MTLRenderPassDescriptor* m_renderPassDescriptor = nil;
        //! Multisample state used by the renderpass
        RHI::MultisampleState m_scopeMultisampleState;
        //! Color index for the swapchain texture in the renderpass. -1 if no swapchain is being used by the renderpass.
        int m_swapChainAttachmentIndex = -1;
        //! SwapChainFrameAttachment for the swapchain. Null if no swapchain is being used by the renderpass.
        const RHI::SwapChainFrameAttachment* m_swapChainAttachment = nullptr;
    };

    //! Helper class for building a MTLRenderPassDescriptor from the scope attachments.
    class RenderPassBuilder
    {
    public:
        AZ_CLASS_ALLOCATOR(RenderPassBuilder, AZ::SystemAllocator);
        
        RenderPassBuilder() = default;
        ~RenderPassBuilder();

        //! Initialize the builder
        void Init();
        //! Adds the attachments from the scope to the MTLRenderPassDescriptor
        void AddScopeAttachments(Scope* scope);
        //! Ends the building process and populate the provided RenderPassContext with the
        //! collected information.
        RHI::ResultCode End(RenderPassContext& context);
        
    private:
        //! Used to store resolve related attachment information
        struct ResolveAttachmentData
        {
            int m_colorAttachmentIndex = 0;
            MTLRenderPassDescriptor* m_renderPassDesc = nil;
            RHI::ScopeAttachmentUsage m_attachmentUsage = RHI::ScopeAttachmentUsage::Uninitialized;
            bool m_isStoreAction = false;
        };
        
        void Reset();
        
        //! Cache the multisample state and at the same time hook up the custom sample msaa positions for the render pass.
        void ApplyMSAACustomPositions(const ImageView* imageView);

        //! Renderpass to populate
        MTLRenderPassDescriptor* m_renderPassDescriptor = nil;
        //! Sample state of the renderpass
        RHI::MultisampleState m_scopeMultisampleState;
        //! List of scopes added
        AZStd::vector<Scope*> m_scopes;
        //! Mapping between an attachmentId and a color attachment index in the renderpass
        AZStd::unordered_map<RHI::AttachmentId, uint32_t> m_colorAttachmentsIndex;
        //! Number of color attachments at the moment.
        uint32_t m_currentColorAttachmentIndex = 0;
        //! Color attachment index for a swapchain attachment being used. -1 if no swapchain is being used by the renderpass.
        int m_swapChainAttachmentIndex = -1;
        //! SwapChainFrameAttachment used by the renderpass. Null if no swapchain is being used.
        const RHI::SwapChainFrameAttachment* m_swapChainFrameAttachment = nullptr;
    };
}
