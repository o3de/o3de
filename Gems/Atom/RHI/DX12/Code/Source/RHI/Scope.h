/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/Fence.h>
#include <RHI/CommandList.h>
#include <Atom/RHI/Scope.h>
#include <AzCore/Memory/PoolAllocator.h>

namespace AZ
{
    namespace DX12
    {
        class SwapChain;
        class Image;
        class Buffer;

        class Scope final
            : public RHI::Scope
        {
            using Base = RHI::Scope;
        public:
            AZ_RTTI(Scope, "{DE4C74AA-FD4C-4BC3-80DC-F405B72E4327}", Base);
            AZ_CLASS_ALLOCATOR(Scope, AZ::SystemAllocator, 0);

            static RHI::Ptr<Scope> Create();

            void Begin(
                CommandList& commandList,
                uint32_t commandListIndex,
                uint32_t commandListCount) const;

            void End(
                CommandList& commandList,
                uint32_t commandListIndex,
                uint32_t commandListCount) const;

            const bool IsStateSupportedByQueue(D3D12_RESOURCE_STATES state) const;

            void QueuePrologueTransition(const D3D12_RESOURCE_TRANSITION_BARRIER& transitionBarrier);
            void QueueEpilogueTransition(const D3D12_RESOURCE_TRANSITION_BARRIER& transitionBarrier);
            void QueueAliasingBarrier(const D3D12_RESOURCE_ALIASING_BARRIER& aliasingBarrier);
            void QueueResolveTransition(const D3D12_RESOURCE_TRANSITION_BARRIER& transitionBarrier);
            void QueuePreDiscardTransition(const D3D12_RESOURCE_TRANSITION_BARRIER& transitionBarrier);

            bool HasSignalFence() const;
            bool HasWaitFences() const;

            void SetSignalFenceValue(uint64_t fenceValue);
            uint64_t GetSignalFenceValue() const;

            void SetWaitFenceValueByQueue(RHI::HardwareQueueClass hardwareQueueClass, uint64_t fenceValue);
            uint64_t GetWaitFenceValueByQueue(RHI::HardwareQueueClass hardwareQueueClass) const;
            const FenceValueSet& GetWaitFences() const;

            // Returns true if the resource within the scopeAttachment is scheduled to be discarded
            bool IsResourceDiscarded(const RHI::ImageScopeAttachment& scopeAttachment) const;
            bool IsResourceDiscarded(const RHI::BufferScopeAttachment& scopeAttachment) const;
            
        private:
            Scope() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::Scope
            void DeactivateInternal() override;
            void CompileInternal(RHI::Device& device) override;
            //////////////////////////////////////////////////////////////////////////

            void CompileAttachmentInternal(
                bool isFullResourceClear,
                const RHI::ScopeAttachment& binding,
                ID3D12Resource* resource);

            // Returns true if nativeResource is present within m_discardResourceRequests
            bool IsInDiscardResourceRequests(ID3D12Resource* nativeResource) const;
            
            /// A set of transition barriers for both before and after the scope.
            AZStd::vector<D3D12_RESOURCE_TRANSITION_BARRIER> m_prologueTransitionBarrierRequests;
            AZStd::vector<D3D12_RESOURCE_TRANSITION_BARRIER> m_epilogueTransitionBarrierRequests;
            AZStd::vector<D3D12_RESOURCE_TRANSITION_BARRIER> m_preDiscardTransitionBarrierRequests;

            /// A set of transition barriers for resolving a multisample image.
            AZStd::vector<D3D12_RESOURCE_TRANSITION_BARRIER> m_resolveTransitionBarrierRequests;

            /// Aliasing barrier requests for transient resources.
            AZStd::vector<D3D12_RESOURCE_ALIASING_BARRIER> m_aliasingBarriers;

            /// Array of color attachments, bound by index.
            AZStd::vector<const ImageView*> m_colorAttachments;

            /// [optional] Depth stencil attachment.
            const ImageView* m_depthStencilAttachment = nullptr;

            /// Depth stencil attachment access.
            RHI::ScopeAttachmentAccess m_depthStencilAccess = RHI::ScopeAttachmentAccess::ReadWrite;

            /// Clear image requests which use render target stage.
            AZStd::vector<CommandList::ImageClearRequest> m_clearRenderTargetRequests;

            /// Clear image requests which use unordered access.
            AZStd::vector<CommandList::ImageClearRequest> m_clearImageRequests;

            /// Clear buffer requests which use unordered access.
            AZStd::vector<CommandList::BufferClearRequest> m_clearBufferRequests;

            /// Discard resource requests for transient resources.
            AZStd::vector<ID3D12Resource*> m_discardResourceRequests;

            /// The list of fences to signal before executing this scope.
            FenceValueSet m_waitFencesByQueue = { {0} };

            /// The value to signal after executing this scope.
            uint64_t m_signalFenceValue = 0;
        };
    }
}
