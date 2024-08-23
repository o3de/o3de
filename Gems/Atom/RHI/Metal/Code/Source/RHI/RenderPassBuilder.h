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
    struct RenderPassContext
    {
        MTLRenderPassDescriptor* m_renderPassDescriptor = nil;
        RHI::MultisampleState m_scopeMultisampleState;
        int m_swapChainAttachmentIndex = -1;
        const RHI::SwapChainFrameAttachment* m_swapChainAttachment = nullptr;
    };

    class RenderPassBuilder
    {
    public:
        AZ_CLASS_ALLOCATOR(RenderPassBuilder, AZ::SystemAllocator);
        
        RenderPassBuilder() = default;
        ~RenderPassBuilder();
        
        void Init();
        void AddScopeAttachments(Scope* scope);
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
        
        MTLRenderPassDescriptor* m_renderPassDescriptor = nil;
        RHI::MultisampleState m_scopeMultisampleState;
        AZStd::vector<Scope*> m_scopes;
        AZStd::unordered_map<RHI::AttachmentId, uint32_t> m_colorAttachmentsIndex;
        uint32_t m_currentColorAttachmentIndex = 0;
        int m_swapChainAttachmentIndex = -1;
        const RHI::SwapChainFrameAttachment* m_swapChainFrameAttachment = nullptr;
    };
}
