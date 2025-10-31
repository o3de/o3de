/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

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
    namespace RHI
    {
        class FrameGraphExecuteContext;
    }

    namespace Vulkan
    {
        class BufferView;
        class CommandList;
        class Image;
        class RenderPass;
        class RenderPassBuilder;

        class Scope final
            : public RHI::Scope
        {
            using Base = RHI::Scope;
            friend class RenderPassBuilder;

        public:
            struct Barrier;

            AZ_RTTI(Scope, "328CA015-A73A-4A64-8C10-798C021575B3", Base);
            AZ_CLASS_ALLOCATOR(Scope, AZ::SystemAllocator);

            enum class BarrierSlot : uint32_t
            {
                Clear = 0,      // First to be executed in the scope.
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

            void Begin(CommandList& commandList, const RHI::FrameGraphExecuteContext& context) const;
            void End(CommandList& commandList, const RHI::FrameGraphExecuteContext& context) const;

            //! Adds a barrier for a scope attachment resource to be emitted at a later time.
            //! Returns the barrier inserted. This barrier may have been merged with a previously inserted barrier.
            template<class T>
            const Barrier QueueAttachmentBarrier(
                RHI::ScopeAttachment& attachment,
                BarrierSlot slot,
                const VkPipelineStageFlags src,
                const VkPipelineStageFlags dst,
                const T& barrier)
            {
                return QueueBarrierInternal(&attachment, slot, src, dst, barrier);
            }

            // Collects all the memory barriers used by the scope in the provided BarrierSlot.
            VkMemoryBarrier2 CollectMemoryBarriers(BarrierSlot slot) const;

            //! Execute the queued barriers into the provided commandlist.
            void EmitScopeBarriers(CommandList& commandList, BarrierSlot slot, BarrierTypeFlags mask = BarrierTypeFlags::All) const;
            
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
            const AZStd::vector<RHI::Ptr<Fence>>& GetWaitFences() const;

            //! Graphics scopes that draw items use a renderpass.
            //! Compute or copy scopes do not use a renderpass.
            bool UsesRenderpass() const;

            //! Returns the mode of MSAA resolving (if any).
            ResolveMode GetResolveMode() const;

            //! Resolves multisampled attachments using a command list. ResolveMode must be ResolveMode::CommandList
            void ResolveMSAAAttachments(CommandList& commandList) const;

            void SetDepthStencilFullView(RHI::ConstPtr<ImageView> view);
            const ImageView* GetDepthStencilFullView() const;

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
                RHI::ScopeAttachment* m_attachment = nullptr;

                VkStructureType m_type = static_cast<VkStructureType>(~0);
                union
                {
                    VkMemoryBarrier m_memoryBarrier;
                    VkBufferMemoryBarrier m_bufferBarrier;
                    VkImageMemoryBarrier m_imageBarrier;
                };

                // Checks if the image barrier affects the imageView
                bool Overlaps(const ImageView& imageView, OverlapType overlapType) const;

                // Checks if the buffer barrier affects the bufferView
                bool Overlaps(const BufferView& bufferView, OverlapType overlapType) const;

                bool Overlaps(const Barrier& barrier, OverlapType overlapType) const;

                void Combine(const Barrier& rhs);

                // Returns false if the barrier is not needed because is a read after read access with no
                // ownership transfer or layout transition.
                bool IsNeeded() const;

                bool operator==(const Barrier& other) const;
            };

            using BarrierList = AZStd::array<AZStd::vector<Barrier>, BarrierSlotCount>;

            // Converts barriers to implicit subpass barriers if possible.
            void OptimizeBarriers();

        private:
            struct QueryPoolAttachment
            {
                RHI::Ptr<RHI::QueryPool> m_pool;
                RHI::Interval m_interval;
                RHI::ScopeAttachmentAccess m_access;
            };

            Scope() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::Scope
            void ActivateInternal() override;
            void DeactivateInternal() override;
            void CompileInternal() override;
            void AddQueryPoolUse(RHI::Ptr<RHI::QueryPool> queryPool, const RHI::Interval& interval, RHI::ScopeAttachmentAccess access) override;
            //////////////////////////////////////////////////////////////////////////

            // Returns true if a barrier can be converted to an implicit subpass barrier.
            bool CanOptimizeBarrier(const Barrier& barrier, BarrierSlot slot) const;

            // Returns true if the barriers in the scope have layout transitions that are incompatible with automatic layout transition of
            // a renderpass and thus cannot be optimized.
            bool AreDepthStencilLayoutsCompatible(const Barrier& barrier, BarrierSlot slot) const;

            template<class T>
            const Barrier QueueBarrierInternal(
                RHI::ScopeAttachment* attachment,
                BarrierSlot slot,
                const VkPipelineStageFlags src,
                const VkPipelineStageFlags dst,
                const T& barrier);

            void OptimizeBarrier(const Barrier& unoptimizedBarrier, BarrierSlot slot);
            void BuildGlobalBarriers(BarrierSlot slot);

            // Returns true if it's the first usage of the scope in the graph group.
            bool IsFirstUsage(const RHI::ScopeAttachment* scopeAttachment) const;
            // Returns true if it's the last usage of the scope in the graph group.
            bool IsLastUsage(const RHI::ScopeAttachment* scopeAttachment) const;
            // Returns true if the previous/next usage of the ScopeAttachment is outside of the graph group.
            bool HasExternalConnection(const RHI::ScopeAttachment* scopeAttachment, BarrierSlot slot) const;

            // List of barriers that are not yet optimized.
            BarrierList m_unoptimizedBarriers;
            // List of barriers that must be execute outside the renderpass.
            BarrierList m_scopeBarriers;
            // List of barriers that are merged as a global memory barrier.
            BarrierList m_globalBarriers;

            AZStd::vector<Semaphore::WaitSemaphore> m_waitSemaphores;
            AZStd::vector<RHI::Ptr<Semaphore>> m_signalSemaphores;
            AZStd::vector<RHI::Ptr<Fence>> m_signalFences;
            AZStd::vector<RHI::Ptr<Fence>> m_waitFences;
            AZStd::vector<QueryPoolAttachment> m_queryPoolAttachments;
            bool m_usesRenderpass = false;

            VkPipelineStageFlags m_deviceSupportedPipelineStageFlags = 0;

            ResolveMode m_resolveMode = ResolveMode::None;
            AZStd::vector<CommandList::ResourceClearRequest> m_imageClearRequests;
            AZStd::vector<CommandList::ResourceClearRequest> m_bufferClearRequests;

            // Used to hold a view to the full depth and stencil when we need to merge two
            // ScopeAttachment views, one that is only depth with one that is only stencil.
            RHI::ConstPtr<ImageView> m_depthStencilFullView;
        };

        template<class T>
        const Scope::Barrier Scope::QueueBarrierInternal(
            RHI::ScopeAttachment* attachment,
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

            if (!barrier.IsNeeded())
            {
                // If the barrier is not needed, we add the source access to the destination access,
                // so future barriers can synchronize properly. E.g. a read after read barrier needs
                // future barriers to synchronize with both read accesses (the first and the second read accesses). 
                barrier.m_dstStageMask |= barrier.m_srcStageMask;
                switch (barrier.m_type)
                {
                case VK_STRUCTURE_TYPE_MEMORY_BARRIER:
                    barrier.m_memoryBarrier.dstAccessMask |= barrier.m_memoryBarrier.srcAccessMask;
                    break;
                case VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER:
                    barrier.m_bufferBarrier.dstAccessMask |= barrier.m_bufferBarrier.srcAccessMask;
                    break;
                case VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER:
                    barrier.m_imageBarrier.dstAccessMask |= barrier.m_imageBarrier.srcAccessMask;
                    break;
                default:
                    break;
                }
                return barrier;
            }

            auto& unoptimizedBarriers = m_unoptimizedBarriers[static_cast<uint32_t>(slot)];
            auto findIt = AZStd::find_if(
                unoptimizedBarriers.begin(),
                unoptimizedBarriers.end(),
                [&](const Barrier& element)
                {
                    return element.Overlaps(barrier, OverlapType::Partial);
                });

            if (findIt != unoptimizedBarriers.end())
            {
                Barrier& mergedBarrier = *findIt;
                mergedBarrier.Combine(barrier);
                return mergedBarrier;
            }
            else
            {
                unoptimizedBarriers.push_back(barrier);
                return unoptimizedBarriers.back();
            }
        }
    } // namespace Vulkan
}
