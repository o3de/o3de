/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
            AZ_CLASS_ALLOCATOR(Scope, AZ::SystemAllocator);

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

            //! Adds a transition barrier that will be emitted at the beginning of the scope.
            //! Can specify a state that the command list need to be before emitting the barrier.
            //! A null state means that it doesn't matter in which state the command list is.
            //! Returns the barrier inserted. This barrier may have been merged with a previously inserted barrier.
            const D3D12_RESOURCE_TRANSITION_BARRIER& QueuePrologueTransition(
                const D3D12_RESOURCE_TRANSITION_BARRIER& transitionBarrier,
                const BarrierOp::CommandListState* state = nullptr);
            //! Adds a transition barrier that will be emitted at the end of the scope.
            //! Can specify a state that the command list need to be before emitting the barrier.
            //! A null state means that it doesn't matter in which state the command list is.
            //! Returns the barrier inserted. This barrier may have been merged with a previously inserted barrier.
            const D3D12_RESOURCE_TRANSITION_BARRIER& QueueEpilogueTransition(
                const D3D12_RESOURCE_TRANSITION_BARRIER& transitionBarrier,
                const BarrierOp::CommandListState* state = nullptr);
            //! Adds an aliasing barrier that will be emitted at the beginning of the scope.
            //! Can specify a state that the command list need to be before emitting the barrier.
            //! A null state means that it doesn't matter in which state the command list is.
            void QueueAliasingBarrier(
                const D3D12_RESOURCE_ALIASING_BARRIER& aliasingBarrier,
                const BarrierOp::CommandListState* state = nullptr);
            //! Adds a transition barrier that will be emitted at the end of the scope before resolving.
            //! Can specify a state that the command list need to be before emitting the barrier.
            //! A null state means that it doesn't matter in which state the command list is.
            //! Returns the barrier inserted. This barrier may have been merged with a previously inserted barrier.
            const D3D12_RESOURCE_TRANSITION_BARRIER& QueueResolveTransition(
                const D3D12_RESOURCE_TRANSITION_BARRIER& transitionBarrier,
                const BarrierOp::CommandListState* state = nullptr);
            //! Adds a transition barrier that will be emitted at the beginning of the scope before discarting resources.
            //! Can specify a state that the command list need to be before emitting the barrier.
            //! A null state means that it doesn't matter in which state the command list is.
            void QueuePreDiscardTransition(
                const D3D12_RESOURCE_TRANSITION_BARRIER& transitionBarrier,
                const BarrierOp::CommandListState* state = nullptr);

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
            void CompileInternal() override;
            //////////////////////////////////////////////////////////////////////////

            void CompileAttachmentInternal(
                bool isFullResourceClear,
                const RHI::ScopeAttachment& binding,
                ID3D12Resource* resource);

            // Returns true if nativeResource is present within m_discardResourceRequests
            bool IsInDiscardResourceRequests(ID3D12Resource* nativeResource) const;

            const D3D12_RESOURCE_TRANSITION_BARRIER& QueueTransitionInternal(
                AZStd::vector<BarrierOp>& barriers, const BarrierOp& barrierToAdd);
            
            /// A set of transition barriers for both before and after the scope.
            AZStd::vector<BarrierOp> m_prologueTransitionBarrierRequests;
            AZStd::vector<BarrierOp> m_epilogueTransitionBarrierRequests;
            AZStd::vector<BarrierOp> m_preDiscardTransitionBarrierRequests;

            /// A set of transition barriers for resolving a multisample image.
            AZStd::vector<BarrierOp> m_resolveTransitionBarrierRequests;

            /// Aliasing barrier requests for transient resources.
            AZStd::vector<BarrierOp> m_aliasingBarriers;

            /// Array of color attachments, bound by index.
            AZStd::vector<const ImageView*> m_colorAttachments;

            /// [optional] Depth stencil attachment.
            const ImageView* m_depthStencilAttachment = nullptr;

            /// [optional] Shading rate attachment.
            const ImageView* m_shadingRateAttachment = nullptr;

            /// Depth stencil attachment access.
            RHI::ScopeAttachmentAccess m_depthStencilAccess = RHI::ScopeAttachmentAccess::Unknown;

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

            /// Holds a view with both depth and stencil aspects. Used when
            /// merging a depth only attachment with a depth stencil attachment.
            RHI::ConstPtr<ImageView> m_fullDepthStencilView;
        };
    }
}
