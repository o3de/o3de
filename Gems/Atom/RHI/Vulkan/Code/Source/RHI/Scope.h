/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/FrameEventBus.h>
#include <Atom/RHI/Scope.h>
#include <Atom/RHI.Reflect/ClearValue.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <RHI/Fence.h>
#include <RHI/Framebuffer.h>
#include <RHI/QueryPool.h>
#include <RHI/Semaphore.h>

namespace AZ
{
    namespace Vulkan
    {
        class BufferView;
        class CommandList;
        class Image;
        class RenderPass;
        class RenderPassBuilder;

        class Scope final
            : public RHI::Scope
            , public RHI::FrameEventBus::Handler
        {
            using Base = RHI::Scope;
            friend class RenderPassBuilder;

        public:
            AZ_RTTI(Scope, "328CA015-A73A-4A64-8C10-798C021575B3", Base);
            AZ_CLASS_ALLOCATOR(Scope, AZ::SystemAllocator);

            enum class BarrierSlot : uint32_t
            {
                Aliasing = 0,   // First to be executed in the scope.
                Clear,
                Prologue,
                Epilogue,
                Resolve,        // Last to be executed in the scope.
                Count
            };

            enum class ResolveMode : uint32_t
            {
                None = 0,   // Does not resolve any attachments.
                RenderPass, // Resolves multisampled attachments using a renderpass (resolve attachments).
                CommandList // Resolves multisampled attachments using a command list function.
            };

            static constexpr uint32_t BarrierSlotCount = static_cast<uint32_t>(BarrierSlot::Count);

            static RHI::Ptr<Scope> Create();
            ~Scope() = default;

            void Begin(CommandList& commandList) const;
            void End(CommandList& commandList) const;

            //! Adds a barrier for a scope attachment resource to be emitted at a later time.
            template<class T>
            void QueueAttachmentBarrier(
                const RHI::ScopeAttachment& attachment,
                BarrierSlot slot,
                const VkPipelineStageFlags src,
                const VkPipelineStageFlags dst,
                const T& barrier)
            {
                QueueBarrierInternal(&attachment, slot, src, dst, barrier);
            }

            //! Adds a barrier over a resource that is not a scope attachment that will be emitted at a later time.
            template<class T>
            void QueueBarrier(
                BarrierSlot slot,
                const VkPipelineStageFlags src,
                const VkPipelineStageFlags dst,
                const T& barrier)
            {
                QueueBarrierInternal(nullptr, slot, src, dst, barrier);
            }

            //! Execute the queued barriers into the provided commandlist.
            void EmitScopeBarriers(CommandList& commandList, BarrierSlot slot) const;
            
            //! Process all clear requests needed for UAVs in the scope.
            void ProcessClearRequests(CommandList& commandList) const;

            //! Reset the RHI QueryPool in the QueryPoolAttachments.
            void ResetQueryPools(CommandList& commandList) const;

            void AddWaitSemaphore(const Semaphore::WaitSemaphore& semaphoreInfo);
            void AddSignalSemaphore(RHI::Ptr<Semaphore> semaphore);
            void AddSignalFence(RHI::Ptr<Fence> fence);

            const AZStd::vector<Semaphore::WaitSemaphore>& GetWaitSemaphores() const;
            const AZStd::vector<RHI::Ptr<Semaphore>>& GetSignalSemaphores() const;
            const AZStd::vector<RHI::Ptr<Fence>>& GetSignalFences() const;

            //! Graphics scopes that draw items use a renderpass.
            //! Compute or copy scopes do not use a renderpass.
            bool UsesRenderpass() const;

            //! Returns the mode of MSAA resolving (if any).
            ResolveMode GetResolveMode() const;

            //! Resolves multisampled attachments using a command list. ResolveMode must be ResolveMode::CommandList
            void ResolveMSAAAttachments(CommandList& commandList) const;

        private:
            enum class OverlapType
            {
                Partial = 0,
                Complete
            };

            struct Barrier
            {
                Barrier(VkMemoryBarrier barrier);
                Barrier(VkBufferMemoryBarrier barrier);
                Barrier(VkImageMemoryBarrier barrier);

                VkPipelineStageFlags m_srcStageMask = 0;
                VkPipelineStageFlags m_dstStageMask = 0;
                VkDependencyFlags m_dependencyFlags = 0;
                const RHI::ScopeAttachment* m_attachment = nullptr;

                VkStructureType m_type = static_cast<VkStructureType>(~0);
                union
                {
                    VkMemoryBarrier m_memoryBarrier;
                    VkBufferMemoryBarrier m_bufferBarrier;
                    VkImageMemoryBarrier m_imageBarrier;
                };

                // Checks if the image barrier affects the imageView
                bool BlocksResource(const ImageView& imageView, OverlapType overlapType) const;

                // Checks if the buffer barrier affects the bufferView
                bool BlocksResource(const BufferView& bufferView, OverlapType overlapType) const;
            };

            using BarrierList = AZStd::array<AZStd::vector<Barrier>, BarrierSlotCount>;

            struct QueryPoolAttachment
            {
                RHI::Ptr<QueryPool> m_pool;
                RHI::Interval m_interval;
                RHI::ScopeAttachmentAccess m_access;
            };

            Scope() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::Scope
            void ActivateInternal() override;
            void DeactivateInternal() override;
            void CompileInternal(RHI::Device& device) override;
            void AddQueryPoolUse(RHI::Ptr<RHI::SingleDeviceQueryPool> queryPool, const RHI::Interval& interval, RHI::ScopeAttachmentAccess access) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // FrameEventBus::Handler
            void OnFrameCompileEnd(RHI::FrameGraph& frameGraph) override;
            //////////////////////////////////////////////////////////////////////////

            // Returns true if a barrier can be converted to an implicit subpass barrier.
            bool CanOptimizeBarrier(const Barrier& barrier, BarrierSlot slot) const;

            template<class T>
            void QueueBarrierInternal(
                const RHI::ScopeAttachment* attachment,
                BarrierSlot slot,
                const VkPipelineStageFlags src,
                const VkPipelineStageFlags dst,
                const T& barrier);

            // Converts barriers to implicit subpass barriers if possible.
            void OptimizeBarriers();

            // List of barriers that are not yet optimized.
            BarrierList m_unoptimizedBarriers;
            // List of barriers that must be execute outside the renderpass.
            BarrierList m_scopeBarriers;
            // List of barriers that are part of the subpass.
            BarrierList m_subpassBarriers;

            AZStd::vector<Semaphore::WaitSemaphore> m_waitSemaphores;
            AZStd::vector<RHI::Ptr<Semaphore>> m_signalSemaphores;
            AZStd::vector<RHI::Ptr<Fence>> m_signalFences;
            AZStd::vector<QueryPoolAttachment> m_queryPoolAttachments;
            bool m_usesRenderpass = false;

            VkPipelineStageFlags m_deviceSupportedPipelineStageFlags = 0;

            ResolveMode m_resolveMode = ResolveMode::None;
            AZStd::vector<CommandList::ResourceClearRequest> m_imageClearRequests;
            AZStd::vector<CommandList::ResourceClearRequest> m_bufferClearRequests;
        };

        template<class T>
        void Scope::QueueBarrierInternal(
            const RHI::ScopeAttachment* attachment,
            BarrierSlot slot,
            const VkPipelineStageFlags src,
            const VkPipelineStageFlags dst,
            const T& vkBarrier)
        {
            Barrier barrier(vkBarrier);
            barrier.m_dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
            barrier.m_attachment = attachment;
            barrier.m_dstStageMask = dst;
            barrier.m_srcStageMask = src;

            m_unoptimizedBarriers[static_cast<uint32_t>(slot)].push_back(barrier);
        }

    } // namespace Vulkan
}
