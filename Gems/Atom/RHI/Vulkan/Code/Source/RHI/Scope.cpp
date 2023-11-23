/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom_RHI_Vulkan_Platform.h>
#include <algorithm>
#include <Atom/RHI/BufferScopeAttachment.h>
#include <Atom/RHI/BufferProperty.h>
#include <Atom/RHI/ImageScopeAttachment.h>
#include <Atom/RHI/ImageFrameAttachment.h>
#include <Atom/RHI/ResolveScopeAttachment.h>
#include <Atom/RHI/SwapChainFrameAttachment.h>
#include <RHI/CommandList.h>
#include <RHI/GraphicsPipeline.h>
#include <RHI/ImageView.h>
#include <RHI/QueryPool.h>
#include <RHI/PipelineState.h>
#include <RHI/RenderPass.h>
#include <RHI/ResourcePoolResolver.h>
#include <RHI/Scope.h>
#include <RHI/SwapChain.h>
#include <RHI/Framebuffer.h>
#include <RHI/Device.h>
#include <RHI/Conversion.h>
#include <RHI/BufferView.h>

namespace AZ
{
    namespace Vulkan
    {
        Scope::Barrier::Barrier(VkMemoryBarrier barrier)
            : m_type(barrier.sType)
            , m_memoryBarrier(barrier)
        {}

        Scope::Barrier::Barrier(VkBufferMemoryBarrier barrier)
            : m_type(barrier.sType)
            , m_bufferBarrier(barrier)
        {}

        Scope::Barrier::Barrier(VkImageMemoryBarrier barrier)
            : m_type(barrier.sType)
            , m_imageBarrier(barrier)
        {}

        bool Scope::Barrier::BlocksResource(const ImageView& imageView, OverlapType overlapType) const
        {
            if (m_type != VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER ||
                m_imageBarrier.image != static_cast<const Image&>(imageView.GetImage()).GetNativeImage())
            {
                return false;
            }

            const auto& vkImageSubresourceRange = imageView.GetVkImageSubresourceRange();
            return overlapType == OverlapType::Partial ?
                SubresourceRangeOverlaps(m_imageBarrier.subresourceRange, vkImageSubresourceRange) :
                m_imageBarrier.subresourceRange == vkImageSubresourceRange;
        }

        bool Scope::Barrier::BlocksResource(const BufferView& bufferView, OverlapType overlapType) const
        {
            const BufferMemoryView* bufferMemoryView = static_cast<const Buffer&>(bufferView.GetBuffer()).GetBufferMemoryView();
            const auto& buferViewDescriptor = bufferView.GetDescriptor();
            if (m_type != VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER ||
                m_bufferBarrier.buffer != bufferMemoryView->GetNativeBuffer())
            {
                return false;
            }

            RHI::BufferSubresourceRange barrierRange(m_bufferBarrier.offset, m_bufferBarrier.size);
            RHI::BufferSubresourceRange bufferViewRange(buferViewDescriptor);
            bufferViewRange.m_byteOffset += bufferMemoryView->GetOffset();
            return overlapType == OverlapType::Partial ? SubresourceRangeOverlaps(barrierRange, bufferViewRange) : barrierRange == bufferViewRange;
        }        

        RHI::Ptr<Scope> Scope::Create()
        {
            return aznew Scope();
        }

        void Scope::Begin(CommandList& commandList) const
        {
            commandList.GetValidator().BeginScope(*this);

            for (RHI::ResourcePoolResolver* resolverBase : GetResourcePoolResolves())
            {
                auto* resolver = static_cast<ResourcePoolResolver*>(resolverBase);
                resolver->Resolve(commandList);
            }
        }

        void Scope::End(CommandList& commandList) const
        {
            commandList.GetValidator().EndScope();
        }

        template<class T>
        void AddAccessStages(T& barrier, const T& barrierToAdd)
        {
            barrier.srcAccessMask |= barrierToAdd.srcAccessMask;
            barrier.dstAccessMask |= barrierToAdd.dstAccessMask;
        }

        template<class T>
        void AddStageMasks(T& barrier, const T& barrierToAdd)
        {
            barrier.m_srcStageMask |= barrierToAdd.m_srcStageMask;
            barrier.m_dstStageMask |= barrierToAdd.m_dstStageMask;
        }

        template<class T>
        void FilterAccessStages(T& barrier, VkAccessFlags srcSupportedFlags, VkAccessFlags dstSupportedFlags)
        {
            barrier.srcAccessMask = RHI::FilterBits(barrier.srcAccessMask, srcSupportedFlags);
            barrier.dstAccessMask = RHI::FilterBits(barrier.dstAccessMask, dstSupportedFlags);
        }

        void Scope::EmitScopeBarriers(CommandList& commandList, BarrierSlot slot) const
        {
            switch (slot)
            {
            case BarrierSlot::Prologue:
                for (RHI::ResourcePoolResolver* resolvePolicyBase : GetResourcePoolResolves())
                {
                    static_cast<ResourcePoolResolver*>(resolvePolicyBase)->QueuePrologueTransitionBarriers(commandList);
                }
                break;
            case BarrierSlot::Epilogue:
                for (RHI::ResourcePoolResolver* resolvePolicyBase : GetResourcePoolResolves())
                {
                    static_cast<ResourcePoolResolver*>(resolvePolicyBase)->QueueEpilogueTransitionBarriers(commandList);
                }
                break;
            default:
                break;
            }

            for (const auto& barrier : m_scopeBarriers[static_cast<uint32_t>(slot)])
            {
                const VkMemoryBarrier* memoryBarriers = nullptr;
                const VkBufferMemoryBarrier* bufferBarriers = nullptr;
                const VkImageMemoryBarrier* imageBarriers = nullptr;
                switch (barrier.m_type)
                {
                case VK_STRUCTURE_TYPE_MEMORY_BARRIER:
                    memoryBarriers = &barrier.m_memoryBarrier;
                    break;
                case VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER:
                    bufferBarriers = &barrier.m_bufferBarrier;
                    break;
                case VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER:
                    imageBarriers = &barrier.m_imageBarrier;
                    break;
                default:
                    AZ_Assert(false, "Invalid memory barrier type.");
                    break;
                }

                static_cast<Device&>(commandList.GetDevice())
                    .GetContext()
                    .CmdPipelineBarrier(
                        commandList.GetNativeCommandBuffer(),
                        barrier.m_srcStageMask,
                        barrier.m_dstStageMask,
                        barrier.m_dependencyFlags,
                        memoryBarriers ? 1 : 0,
                        memoryBarriers,
                        bufferBarriers ? 1 : 0,
                        bufferBarriers,
                        imageBarriers ? 1 : 0,
                        imageBarriers);
            }
        }

        void Scope::ProcessClearRequests(CommandList& commandList) const
        {
            EmitScopeBarriers(commandList, BarrierSlot::Clear);
            for (const auto& clearRequest : m_bufferClearRequests)
            {
                commandList.ClearBuffer(clearRequest);
            }

            for (const auto& clearRequest : m_imageClearRequests)
            {
                commandList.ClearImage(clearRequest);
            }
        }

        void Scope::ResetQueryPools(CommandList& commandList) const
        {
            // Reset the Query Pools that will be used before starting the renderpass.
            for (const auto& queryPoolAttachment : m_queryPoolAttachments)
            {
                if (!RHI::CheckBitsAll(queryPoolAttachment.m_access, RHI::ScopeAttachmentAccess::Write))
                {
                    continue;
                }

                queryPoolAttachment.m_pool->ResetQueries(commandList, queryPoolAttachment.m_interval);
            }
        }

        void Scope::AddWaitSemaphore(const Semaphore::WaitSemaphore& semaphoreInfo)
        {
            m_waitSemaphores.push_back(semaphoreInfo);
        }

        void Scope::AddSignalSemaphore(RHI::Ptr<Semaphore> semaphore)
        {
            m_signalSemaphores.push_back(semaphore);
        }

        void Scope::AddSignalFence(RHI::Ptr<Fence> fence)
        {
            m_signalFences.push_back(fence);
        }

        const AZStd::vector<RHI::Ptr<Semaphore>>& Scope::GetSignalSemaphores() const
        {
            return m_signalSemaphores;
        }

        const AZStd::vector<RHI::Ptr<Fence>>& Scope::GetSignalFences() const
        {
            return m_signalFences;
        }

        const AZStd::vector<Semaphore::WaitSemaphore>& Scope::GetWaitSemaphores() const
        {
            return m_waitSemaphores;
        }

        bool Scope::UsesRenderpass() const
        {
            return m_usesRenderpass;
        }

        void Scope::ActivateInternal()
        {
            bool haveRenderAttachments = false;
            bool haveClearLoadOps = false;
            const auto& imageAttachments = GetImageAttachments();
            for (auto attachment : imageAttachments)
            {
                haveRenderAttachments |=
                    attachment->HasUsage(RHI::ScopeAttachmentUsage::RenderTarget) ||
                    attachment->HasUsage(RHI::ScopeAttachmentUsage::DepthStencil) ||
                    attachment->HasUsage(RHI::ScopeAttachmentUsage::Resolve);

                haveClearLoadOps |=
                    attachment->GetDescriptor().m_loadStoreAction.m_loadAction == RHI::AttachmentLoadAction::Clear ||
                    attachment->GetDescriptor().m_loadStoreAction.m_loadActionStencil == RHI::AttachmentLoadAction::Clear;
            }

            if (haveRenderAttachments &&
                (haveClearLoadOps || GetEstimatedItemCount() > 0))
            {
                m_usesRenderpass = true;
            }

            if (GetResolveAttachments().empty())
            {
                m_resolveMode = ResolveMode::None;                
            }
            else
            {
                m_resolveMode = m_usesRenderpass ? ResolveMode::RenderPass : ResolveMode::CommandList;
            }
        }

        void Scope::DeactivateInternal()
        {
            for (RHI::ResourcePoolResolver* resolvePolicyBase : GetResourcePoolResolves())
            {
                static_cast<ResourcePoolResolver*>(resolvePolicyBase)->Deactivate();
            }

            for (const auto& fence : m_signalFences)
            {
                static_cast<Device&>(fence->GetDevice()).QueueForRelease(fence);
            }

            m_waitSemaphores.clear();
            m_signalSemaphores.clear();
            m_signalFences.clear();
            for (size_t i = 0; i < BarrierSlotCount; ++i)
            {
                m_unoptimizedBarriers[i].clear();
                m_scopeBarriers[i].clear();
                m_subpassBarriers[i].clear();
            }

            m_usesRenderpass = false;
            m_queryPoolAttachments.clear();

            m_deviceSupportedPipelineStageFlags = 0;

            m_imageClearRequests.clear();
            m_bufferClearRequests.clear();

            RHI::FrameEventBus::Handler::BusDisconnect();
        }

        template<typename T>
        void CollectClearActions(const AZStd::vector<T*>& attachments, AZStd::vector<CommandList::ResourceClearRequest>& clearRequests)
        {
            for (const auto* scopeAttachment : attachments)
            {
                const auto& bindingDescriptor = scopeAttachment->GetDescriptor();
                if (HasExplicitClear(*scopeAttachment, bindingDescriptor))
                {
                    clearRequests.push_back({ bindingDescriptor.m_loadStoreAction.m_clearValue, scopeAttachment->GetResourceView() });
                }
            }
        }

        void Scope::CompileInternal(RHI::Device& deviceBase)
        {
            for (RHI::ResourcePoolResolver* resolvePolicyBase : GetResourcePoolResolves())
            {
                static_cast<ResourcePoolResolver*>(resolvePolicyBase)->Compile(GetHardwareQueueClass());
            }

            CollectClearActions(GetImageAttachments(), m_imageClearRequests);
            CollectClearActions(GetBufferAttachments(), m_bufferClearRequests);

            const auto& fences = GetFencesToSignal();
            m_signalFences.reserve(fences.size());
            for (const auto& fence : fences)
            {
                m_signalFences.push_back(AZStd::static_pointer_cast<Fence>(fence));
            }

            Device& device = static_cast<Device&>(deviceBase);
            m_deviceSupportedPipelineStageFlags = device.GetCommandQueueContext().GetCommandQueue(GetHardwareQueueClass()).GetSupportedPipelineStages();

            RHI::FrameEventBus::Handler::BusConnect(&deviceBase);
        }

        void Scope::AddQueryPoolUse(RHI::Ptr<RHI::SingleDeviceQueryPool> queryPool, const RHI::Interval& interval, RHI::ScopeAttachmentAccess access)
        {
            m_queryPoolAttachments.push_back({ AZStd::static_pointer_cast<QueryPool>(queryPool), interval, access });
        }

        bool Scope::CanOptimizeBarrier(const Barrier& barrier, BarrierSlot slot) const
        {
            if (!UsesRenderpass() || !barrier.m_attachment)
            {
                return false;
            }

            // Check that the destination pipeline stages are supported by the Pipeline
            VkPipelineStageFlags filteredDstStages = RHI::FilterBits(barrier.m_dstStageMask, GetSupportedPipelineStages(RHI::PipelineStateType::Draw));
            if (filteredDstStages == VkPipelineStageFlags(0))
            {
                return false;
            }

            const RHI::ScopeAttachment* scopeAttachment =
                slot == BarrierSlot::Prologue ? barrier.m_attachment->GetPrevious() : barrier.m_attachment->GetNext();

            // We don't optimize external barriers because they cause renderpass incompatiblity with the pipeline renderpass.
            // The subpass dependencies added by the optimization causes the incompatibility.
            if (!scopeAttachment || scopeAttachment->GetScope().GetFrameGraphGroupId() != GetFrameGraphGroupId())
            {
                return false;
            }

            switch (barrier.m_type)
            {
            case VK_STRUCTURE_TYPE_MEMORY_BARRIER:
                return true;
            case VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER:
                return barrier.m_bufferBarrier.srcQueueFamilyIndex == barrier.m_bufferBarrier.dstQueueFamilyIndex;
            case VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER:
                if (barrier.m_imageBarrier.srcQueueFamilyIndex == barrier.m_imageBarrier.dstQueueFamilyIndex)
                {
                    // Check if the barrier is a renderpass image attachment (to replace it with a subpass dependency and an automatic subpass layout transition)
                    const AZStd::vector<RHI::ScopeAttachmentUsageAndAccess>& usagesAndAccesses = scopeAttachment->GetUsageAndAccess();
                    for (const RHI::ScopeAttachmentUsageAndAccess& usageAndAccess : usagesAndAccesses)
                    {
                        if (IsRenderAttachmentUsage(usageAndAccess.m_usage))
                        {
                            // Since we need to preserve the order of the barriers, and scope barriers are execute before
                            // subpass barriers, we need to check that there's no barrier for the same resource already in the
                            // scope barrier list.
                            const auto& scopeBarriers = m_scopeBarriers[static_cast<uint32_t>(slot)];
                            auto findIt = AZStd::find_if(scopeBarriers.begin(), scopeBarriers.end(), [&barrier](const auto& scopeBarrier)
                            {
                                return
                                    scopeBarrier.m_type == VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER &&
                                    scopeBarrier.m_imageBarrier.image == barrier.m_imageBarrier.image &&
                                    SubresourceRangeOverlaps(scopeBarrier.m_imageBarrier.subresourceRange, barrier.m_imageBarrier.subresourceRange);
                            });

                            if (findIt == scopeBarriers.end())
                            {
                                return true;
                            }
                        }
                    }
                }
                return false;
            default:
                AZ_Assert(false, "Invalid memory barrier type.");
                return false;
            }
        }

        void Scope::OptimizeBarriers()
        {
            auto optimizeBarrierFunc = [this](const Barrier& unoptimizedBarrier, BarrierSlot slot)
            {
                uint32_t slotIndex = aznumeric_caster(slot);
                AZStd::vector<Barrier>& barriers = CanOptimizeBarrier(unoptimizedBarrier, slot) ? m_subpassBarriers[slotIndex] : m_scopeBarriers[slotIndex];
                auto filteredBarrier = unoptimizedBarrier;
                filteredBarrier.m_dstStageMask = RHI::FilterBits(unoptimizedBarrier.m_dstStageMask, m_deviceSupportedPipelineStageFlags);
                filteredBarrier.m_srcStageMask = RHI::FilterBits(unoptimizedBarrier.m_srcStageMask, m_deviceSupportedPipelineStageFlags);

                switch (filteredBarrier.m_type)
                {
                case VK_STRUCTURE_TYPE_MEMORY_BARRIER:
                {
                    // All memory barriers can be group into one.
                    auto findIt = AZStd::find_if(barriers.begin(), barriers.end(), [](const Barrier& element) {return element.m_type == VK_STRUCTURE_TYPE_MEMORY_BARRIER; });
                    if (findIt != barriers.end())
                    {
                        Barrier& foundBarrier = *findIt;
                        AddStageMasks(foundBarrier, filteredBarrier);
                        AddAccessStages(foundBarrier.m_memoryBarrier, filteredBarrier.m_memoryBarrier);
                        return;
                    }
                    break;
                }
                case VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER:
                {
                    // Merge all buffer barriers that affect the same buffer region into one.
                    auto findIt = AZStd::find_if(barriers.begin(), barriers.end(), [&filteredBarrier](const Barrier& element)
                    {
                        return element.m_type == VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER &&
                            element.m_bufferBarrier.buffer == filteredBarrier.m_bufferBarrier.buffer &&
                            element.m_bufferBarrier.offset == filteredBarrier.m_bufferBarrier.offset &&
                            element.m_bufferBarrier.size == filteredBarrier.m_bufferBarrier.size &&
                            element.m_bufferBarrier.srcQueueFamilyIndex == filteredBarrier.m_bufferBarrier.srcQueueFamilyIndex &&
                            element.m_bufferBarrier.dstQueueFamilyIndex == filteredBarrier.m_bufferBarrier.dstQueueFamilyIndex;
                    });


                    if (findIt != barriers.end())
                    {
                        // Merge the stage and access flags of the barriers.
                        Barrier& foundBarrier = *findIt;
                        AddStageMasks(foundBarrier, filteredBarrier);
                        AddAccessStages(foundBarrier.m_bufferBarrier, filteredBarrier.m_bufferBarrier);
                        return;
                    }
                    break;
                }
                case VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER:
                {
                    // Merge all image barriers that affect the same image region into one.
                    auto findIt = AZStd::find_if(barriers.begin(), barriers.end(), [&filteredBarrier](const Barrier& element)
                    {
                        return element.m_type == VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER &&
                            element.m_imageBarrier.image == filteredBarrier.m_imageBarrier.image &&
                            element.m_imageBarrier.subresourceRange == filteredBarrier.m_imageBarrier.subresourceRange &&
                            element.m_imageBarrier.srcQueueFamilyIndex == filteredBarrier.m_imageBarrier.srcQueueFamilyIndex &&
                            element.m_imageBarrier.dstQueueFamilyIndex == filteredBarrier.m_imageBarrier.dstQueueFamilyIndex;
                    });

                    if (findIt != barriers.end())
                    {
                        // Merge the stage and access flags of the barriers.
                        Barrier& foundBarrier = *findIt;
                        AddStageMasks(foundBarrier, filteredBarrier);
                        AddAccessStages(foundBarrier.m_imageBarrier, filteredBarrier.m_imageBarrier);
                        // The new layout will be equal to the new layout of the new barrier since this was added
                        // after the found barrier.
                        foundBarrier.m_imageBarrier.oldLayout = slot == BarrierSlot::Prologue ? filteredBarrier.m_imageBarrier.oldLayout : foundBarrier.m_imageBarrier.oldLayout;
                        foundBarrier.m_imageBarrier.newLayout = slot == BarrierSlot::Prologue ? foundBarrier.m_imageBarrier.newLayout : filteredBarrier.m_imageBarrier.newLayout;
                        return;
                    }
                    break;
                }
                default:
                    AZ_Assert(false, "Invalid memory barrier type.");
                    return;
                }

                switch (slot)
                {
                case BarrierSlot::Prologue:
                    barriers.insert(barriers.begin(), filteredBarrier);
                    break;
                case BarrierSlot::Epilogue:
                    barriers.push_back(filteredBarrier);
                    break;
                default:
                    AZ_Assert(false, "Invalid slot type %d", slot);
                    break;
                }                
            };

            uint32_t prologueIndex = aznumeric_caster(BarrierSlot::Prologue);
            uint32_t epilogueIndex = aznumeric_caster(BarrierSlot::Epilogue);
            // Traverse in reverse order because we can only optimize a barrier if we know that
            // all subsequent barriers over the same resource can also be optimize (so the order is preserve).
            // This is because optimized barriers (i.e. subpass implicit barriers) are execute AFTER scope barriers.
            // By traversing in reverse order we can easily check if all subsequent barriers are also optimized.
            for (auto it = m_unoptimizedBarriers[prologueIndex].rbegin(); it != m_unoptimizedBarriers[prologueIndex].rend(); ++it)
            {
                optimizeBarrierFunc(*it, BarrierSlot::Prologue);
            }

            for (auto it = m_unoptimizedBarriers[epilogueIndex].begin(); it != m_unoptimizedBarriers[epilogueIndex].end(); ++it)
            {
                optimizeBarrierFunc(*it, BarrierSlot::Epilogue);
            }

            const uint32_t resolveIndex = aznumeric_caster(BarrierSlot::Resolve);
            const uint32_t clearIndex = aznumeric_caster(BarrierSlot::Clear);
            const uint32_t aliasingIndex = aznumeric_caster(BarrierSlot::Aliasing);
            m_scopeBarriers[resolveIndex] = m_unoptimizedBarriers[resolveIndex];
            m_scopeBarriers[clearIndex] = m_unoptimizedBarriers[clearIndex];
            m_scopeBarriers[aliasingIndex] = m_unoptimizedBarriers[aliasingIndex];
        }

        void Scope::OnFrameCompileEnd(RHI::FrameGraph& frameGraph)
        {
            if (GetFrameGraph() != &frameGraph)
            {
                return;
            }

            // All barriers are already queued so we can optimize them.
            OptimizeBarriers();
        }

        void Scope::ResolveMSAAAttachments(CommandList& commandList) const
        {
            if (GetResolveMode() == ResolveMode::CommandList)
            {
                EmitScopeBarriers(commandList, BarrierSlot::Resolve);
                for (RHI::ResolveScopeAttachment* resolveAttachment : GetResolveAttachments())
                {
                    for (RHI::ImageScopeAttachment* imageAttachment : GetImageAttachments())
                    {
                        if (imageAttachment->GetDescriptor().m_attachmentId == resolveAttachment->GetDescriptor().m_resolveAttachmentId)
                        {
                            auto srcImageView = static_cast<const ImageView*>(imageAttachment->GetImageView());
                            auto dstImageView = static_cast<const ImageView*>(resolveAttachment->GetImageView());
                            auto srcImageSubresourceRange = srcImageView->GetVkImageSubresourceRange();
                            auto dstImageSubresourceRange = dstImageView->GetVkImageSubresourceRange();
                            auto& srcImage = static_cast<const Image&>(srcImageView->GetImage());
                            auto& dstImage = static_cast<const Image&>(dstImageView->GetImage());

                            VkImageResolve region = {};
                            region.extent = ConvertToExtent3D(srcImage.GetDescriptor().m_size);

                            region.srcSubresource.aspectMask = srcImageSubresourceRange.aspectMask;
                            region.srcSubresource.baseArrayLayer = srcImageSubresourceRange.baseArrayLayer;
                            region.srcSubresource.layerCount = srcImageSubresourceRange.layerCount;
                            region.srcSubresource.mipLevel = srcImageSubresourceRange.baseMipLevel;

                            region.dstSubresource.aspectMask = dstImageSubresourceRange.aspectMask;
                            region.dstSubresource.baseArrayLayer = dstImageSubresourceRange.baseArrayLayer;
                            region.dstSubresource.layerCount = dstImageSubresourceRange.layerCount;
                            region.dstSubresource.mipLevel = dstImageSubresourceRange.baseMipLevel;

                            static_cast<Device&>(commandList.GetDevice())
                                .GetContext()
                                .CmdResolveImage(
                                    commandList.GetNativeCommandBuffer(),
                                    srcImage.GetNativeImage(),
                                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                    dstImage.GetNativeImage(),
                                    GetImageAttachmentLayout(*resolveAttachment),
                                    1,
                                    &region);

                            break;
                        }
                    }
                }
            }
        }

        Scope::ResolveMode Scope::GetResolveMode() const
        {
            return m_resolveMode;            
        }
    }
}
