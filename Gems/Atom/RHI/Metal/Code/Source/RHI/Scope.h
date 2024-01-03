/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <Atom/RHI/Scope.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <RHI/CommandList.h>
#include <RHI/Image.h>
#include <RHI/QueryPool.h>
#include <RHI/SwapChain.h>

namespace AZ
{
    namespace Metal
    {
        class SwapChain;
        class Image;
        class Buffer;
        
        class Scope final
        : public RHI::Scope
        {
            using Base = RHI::Scope;
        public:
            AZ_RTTI(Scope, "{FDACECE6-322E-480C-9331-DC639C320882}", Base);
            AZ_CLASS_ALLOCATOR(Scope, AZ::SystemAllocator);

            //Used for aliased memory
            enum class ResourceFenceAction : uint32_t
            {
                Wait = 0,
                Signal,
                Count
            };
            
            static RHI::Ptr<Scope> Create();

            void Begin(
                CommandList& commandList,
                AZ::u32 commandListIndex,
                AZ::u32 commandListCount) const;

            void End(
                CommandList& commandList,
                AZ::u32 commandListIndex,
                AZ::u32 commandListCount) const;
            
            MTLRenderPassDescriptor* GetRenderPassDescriptor() const;
            
            bool HasSignalFence() const;
            bool HasWaitFences() const;
            void SetSignalFenceValue(uint64_t fenceValue);
            uint64_t GetSignalFenceValue() const;

            void SetWaitFenceValueByQueue(RHI::HardwareQueueClass hardwareQueueClass, uint64_t fenceValue);
            uint64_t GetWaitFenceValueByQueue(RHI::HardwareQueueClass hardwareQueueClass) const;
            const FenceValueSet& GetWaitFences() const;

            //!Queue a fence related to the transient resource for this scope
            void QueueResourceFence(ResourceFenceAction fenceAction, Fence& fence);
            
            //! Signal all the transient resource fences associated with this scope
            void SignalAllResourceFences(CommandList& commandList) const;
            void SignalAllResourceFences(id <MTLCommandBuffer> mtlCommandBuffer) const;
            
            //! Wait on all the transient resource fences associated with this scope
            void WaitOnAllResourceFences(CommandList& commandList) const;
            void WaitOnAllResourceFences(id <MTLCommandBuffer> mtlCommandBuffer) const;
        private:
            
            struct QueryPoolAttachment
            {
                RHI::Ptr<QueryPool> m_pool;
                RHI::Interval m_interval;
                RHI::ScopeAttachmentAccess m_access;
            };
            
            Scope() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::Scope
            void InitInternal() override;
            void DeactivateInternal() override;
            void CompileInternal(RHI::Device& device) override;
            void AddQueryPoolUse(RHI::Ptr<RHI::SingleDeviceQueryPool> queryPool, const RHI::Interval& interval, RHI::ScopeAttachmentAccess access) override;
            //////////////////////////////////////////////////////////////////////////
            
            //! Cache the multisample state and at the same time hook up the custom sample msaa positions for the render pass.
            void ApplyMSAACustomPositions(const ImageView* imageView);

            /// Render pass descriptor needed for RenderCommandEncoder or ParallelCommandEncoder
            MTLRenderPassDescriptor*    m_renderPassDescriptor = nil;
                        
            /// Cache MultisampleState for the scope. It is passed on to the command list for validation check against the MultisampleState passed through the PipelineState
            RHI::MultisampleState m_scopeMultisampleState;
                        
            ///Used to check if the scope needs to do a msaa resolve
            bool m_isResolveNeeded = false;
            
            /// Used to check if we need to clear a render target thorugh load action.
            bool m_isClearNeeded = false;
            
            //! Used to store resolve related attachment information
            struct ResolveAttachmentData
            {
                int m_colorAttachmentIndex = 0;
                MTLRenderPassDescriptor* m_renderPassDesc = nil;
                RHI::ScopeAttachmentUsage m_attachmentUsage = RHI::ScopeAttachmentUsage::Uninitialized;
                bool m_isStoreAction = false;
            };
            
            /// The list of fences to wait before executing this scope.
            FenceValueSet m_waitFencesByQueue = { {0} };

            /// The value to signal after executing this scope.
            uint64_t m_signalFenceValue = 0;
                        
            /// Aliasing fence requests for transient resources.
            AZStd::vector<Fence> m_resourceFences[static_cast<uint32_t>(ResourceFenceAction::Count)];

            AZStd::vector<QueryPoolAttachment> m_queryPoolAttachments;
            
            /// Used to check if the current scope is writing to a swapchain texture
            bool m_isWritingToSwapChainScope = false;
            
            /// Used to check if the current scope is a swapchain scope and the next scope will be used to capture the current frame
            bool m_isSwapChainAndFrameCaptureEnabled = false;
            
            /// Used to request the drawable from the driver in the Execute phase (Scope::Begin)
            const RHI::ImageScopeAttachment* m_swapChainAttachment = nullptr;
            
            /// Track all the heaps that will need too be made resident for this scope
            AZStd::set<id<MTLHeap>> m_residentHeaps;

            /// Cache marker name which will be used for labelling.
            Name m_markerName;
        };
    }
}
