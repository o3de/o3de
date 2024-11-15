/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/BufferProperty.h>
#include <Atom/RHI/BufferScopeAttachment.h>
#include <Atom/RHI/FrameGraphExecuteContext.h>
#include <Atom/RHI/ImageFrameAttachment.h>
#include <Atom/RHI/ImageScopeAttachment.h>
#include <Atom/RHI/ResolveScopeAttachment.h>
#include <Atom/RHI/SwapChainFrameAttachment.h>
#include <Atom_RHI_Vulkan_Platform.h>
#include <AzCore/Math/MathIntrinsics.h>
#include <RHI/BufferView.h>
#include <RHI/CommandList.h>
#include <RHI/Conversion.h>
#include <RHI/Device.h>
#include <RHI/Framebuffer.h>
#include <RHI/GraphicsPipeline.h>
#include <RHI/ImageView.h>
#include <RHI/PipelineState.h>
#include <RHI/QueryPool.h>
#include <RHI/RenderPass.h>
#include <RHI/ResourcePoolResolver.h>
#include <RHI/Scope.h>
#include <RHI/SwapChain.h>
#include <algorithm>

namespace AZ
{
    namespace Vulkan
    {
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
        void CombineFamilyQueues(T& barrier, const T& barrierToAdd)
        {
            AZ_Assert(
                barrier.srcQueueFamilyIndex == barrierToAdd.srcQueueFamilyIndex,
                "Different source queue family when combining family queues, %d and %d",
                barrier.srcQueueFamilyIndex,
                barrierToAdd.srcQueueFamilyIndex);
            barrier.dstQueueFamilyIndex = barrierToAdd.dstQueueFamilyIndex;
        }

        void AddSubresources(VkBufferMemoryBarrier& barrier, const VkBufferMemoryBarrier& barrierToAdd)
        {
            VkDeviceSize absoluteOffset = AZStd::max(barrier.offset + barrier.size, barrierToAdd.offset + barrierToAdd.size);
            barrier.offset = AZStd::min(barrier.offset, barrierToAdd.offset);
            barrier.size = absoluteOffset - barrier.offset;
        }
        
        void AddSubresources(VkImageMemoryBarrier& barrier, const VkImageMemoryBarrier& barrierToAdd)
        {
            barrier.subresourceRange.aspectMask |= barrierToAdd.subresourceRange.aspectMask;
            barrier.subresourceRange.baseMipLevel =
                AZStd::min(barrier.subresourceRange.baseMipLevel, barrierToAdd.subresourceRange.baseMipLevel);
            barrier.subresourceRange.baseArrayLayer =
                AZStd::min(barrier.subresourceRange.baseArrayLayer, barrierToAdd.subresourceRange.baseArrayLayer);
            barrier.subresourceRange.levelCount = AZStd::max(barrier.subresourceRange.levelCount, barrierToAdd.subresourceRange.levelCount);
            barrier.subresourceRange.layerCount = AZStd::max(barrier.subresourceRange.layerCount, barrierToAdd.subresourceRange.layerCount);
        }

        template<class T>
        void FilterAccessStages(T& barrier, VkAccessFlags srcSupportedFlags, VkAccessFlags dstSupportedFlags)
        {
            barrier.srcAccessMask = RHI::FilterBits(barrier.srcAccessMask, srcSupportedFlags);
            barrier.dstAccessMask = RHI::FilterBits(barrier.dstAccessMask, dstSupportedFlags);
        }

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

        bool Scope::Barrier::Overlaps(const ImageView& imageView, OverlapType overlapType) const
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

        bool Scope::Barrier::Overlaps(const BufferView& bufferView, OverlapType overlapType) const
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

        bool Scope::Barrier::Overlaps(const Barrier& barrier, OverlapType overlapType) const
        {
            if (m_type != barrier.m_type)
            {
                return false;
            }

            switch (m_type)
            {
            case VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER:
                return m_imageBarrier.image == barrier.m_imageBarrier.image &&
                    (overlapType == OverlapType::Partial
                         ? SubresourceRangeOverlaps(m_imageBarrier.subresourceRange, barrier.m_imageBarrier.subresourceRange)
                         : m_imageBarrier.subresourceRange == barrier.m_imageBarrier.subresourceRange);
            case VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER:
            {
                RHI::BufferSubresourceRange barrierRange(m_bufferBarrier.offset, m_bufferBarrier.size);
                RHI::BufferSubresourceRange barrierRangeOther(barrier.m_bufferBarrier.offset, barrier.m_bufferBarrier.size);
                return m_bufferBarrier.buffer == barrier.m_bufferBarrier.buffer &&
                    (overlapType == OverlapType::Partial
                        ? SubresourceRangeOverlaps(barrierRange, barrierRangeOther)
                        : barrierRange == barrierRangeOther);
            }
            default:
                return false;
            }
        }

        void Scope::Barrier::Combine(const Barrier& other)
        {
            AddStageMasks(*this, other);
            VkAccessFlags srcSupportedAccess = GetSupportedAccessFlags(m_srcStageMask);
            VkAccessFlags dstSupportedAccess = GetSupportedAccessFlags(m_dstStageMask);
            switch (m_type)
            {
            case VK_STRUCTURE_TYPE_MEMORY_BARRIER:
                AddAccessStages(m_memoryBarrier, other.m_memoryBarrier);
                FilterAccessStages(m_memoryBarrier, srcSupportedAccess, dstSupportedAccess);
                break;
            case VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER:
                AddAccessStages(m_bufferBarrier, other.m_bufferBarrier);
                AddSubresources(m_bufferBarrier, other.m_bufferBarrier);
                CombineFamilyQueues(m_bufferBarrier, other.m_bufferBarrier);
                FilterAccessStages(m_bufferBarrier, srcSupportedAccess, dstSupportedAccess);
                break;
            case VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER:
                AddAccessStages(m_imageBarrier, other.m_imageBarrier);
                AddSubresources(m_imageBarrier, other.m_imageBarrier);
                CombineFamilyQueues(m_imageBarrier, other.m_imageBarrier);
                FilterAccessStages(m_imageBarrier, srcSupportedAccess, dstSupportedAccess);
                m_imageBarrier.newLayout = other.m_imageBarrier.newLayout;
                break;
            default:
                break;
            }
        }

        bool Scope::Barrier::IsNeeded() const
        {
            if (!RHI::CheckBitsAll(GetBarrierOptimizationFlags(), BarrierOptimizationFlags::RemoveReadAfterRead))
            {
                return true;
            }

            // Read after read access don't need synchronization.
            switch (m_type)
            {
            case VK_STRUCTURE_TYPE_MEMORY_BARRIER:
                return !(IsReadOnlyAccess(m_memoryBarrier.srcAccessMask) && IsReadOnlyAccess(m_memoryBarrier.dstAccessMask));
            case VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER:
                // Ownership transfer need barriers.
                return !(m_bufferBarrier.srcQueueFamilyIndex == m_bufferBarrier.dstQueueFamilyIndex &&
                    IsReadOnlyAccess(m_bufferBarrier.srcAccessMask) && IsReadOnlyAccess(m_bufferBarrier.dstAccessMask));
            case VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER:
                // Ownership transfer and layout transitions need barriers.
                return !(m_imageBarrier.srcQueueFamilyIndex == m_imageBarrier.dstQueueFamilyIndex &&
                    m_imageBarrier.oldLayout == m_imageBarrier.newLayout &&
                    IsReadOnlyAccess(m_imageBarrier.srcAccessMask) && IsReadOnlyAccess(m_imageBarrier.dstAccessMask));
            default:
                return false;
            }
        }

        bool Scope::Barrier::operator==(const Barrier& other) const
        {
            if (!(m_type == other.m_type && m_srcStageMask == other.m_srcStageMask && m_dstStageMask == other.m_dstStageMask &&
                m_dependencyFlags == other.m_dependencyFlags && m_attachment == other.m_attachment))
            {
                return false;
            }

            switch (m_type)
            {
            case VK_STRUCTURE_TYPE_MEMORY_BARRIER:
                return m_memoryBarrier == other.m_memoryBarrier;
            case VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER:
                return m_bufferBarrier == other.m_bufferBarrier;
            case VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER:
                return m_imageBarrier == other.m_imageBarrier;
            default:
                return false;
            }
        }

        RHI::Ptr<Scope> Scope::Create()
        {
            return aznew Scope();
        }

        void Scope::Begin(CommandList& commandList, const RHI::FrameGraphExecuteContext& context) const
        {
            const bool isPrologue = context.GetCommandListIndex() == 0;
            commandList.GetValidator().BeginScope(*this);

            if (isPrologue)
            {
                for (RHI::ResourcePoolResolver* resolverBase : GetResourcePoolResolves())
                {
                    auto* resolver = static_cast<ResourcePoolResolver*>(resolverBase);
                    resolver->Resolve(commandList);
                }

                // Attachments in a subpass (except for first usage) have to use the vkCmdClearAttachments
                // instead of VK_ATTACHMENT_LOAD_OP_CLEAR.
                if (RHI::CheckBitsAll(GetActivationFlags(), RHI::Scope::ActivationFlags::Subpass))
                {
                    const Framebuffer* framebuffer = commandList.GetActiveFramebuffer();
                    if (framebuffer)
                    {
                        AZStd::vector<VkClearAttachment> clearAttachments;
                        AZStd::vector<VkClearRect> clearRects;
                        for (auto imageAttachment : GetImageAttachments())
                        {
                            const auto& loadStoreAction = imageAttachment->GetScopeAttachmentDescriptor().m_loadStoreAction;
                            bool clear = loadStoreAction.m_loadAction == RHI::AttachmentLoadAction::Clear;
                            bool clearStencil = loadStoreAction.m_loadActionStencil == RHI::AttachmentLoadAction::Clear;
                            if (IsRenderAttachmentUsage(imageAttachment->GetUsage()) && (clear || clearStencil))
                            {
                                // Check that if it's the first usage of the attachment in the renderpass.
                                // On first usage we can use VK_ATTACHMENT_LOAD_OP_CLEAR.
                                if (!IsFirstUsage(imageAttachment))
                                {
                                    const auto& descriptor = imageAttachment->GetDescriptor();
                                    RHI::ImageAspectFlags aspectFlags = descriptor.GetViewDescriptor().m_aspectFlags;
                                    if (!clear)
                                    {
                                        aspectFlags =
                                            RHI::ResetBits(aspectFlags, RHI::ImageAspectFlags::Color | RHI::ImageAspectFlags::Depth);
                                    }
                                    if (!clearStencil)
                                    {
                                        aspectFlags = RHI::ResetBits(aspectFlags, RHI::ImageAspectFlags::Stencil);
                                    }
                                    // Find the index of the attachment in the framebuffer.
                                    auto attachmentIndex = framebuffer->FindImageViewIndex(*imageAttachment);
                                    AZ_Assert(
                                        attachmentIndex || !RHI::CheckBitsAll(aspectFlags, RHI::ImageAspectFlags::Color),
                                        "Failed to find attachment index for attachment %s",
                                        imageAttachment->GetScopeAttachmentDescriptor().m_attachmentId.GetCStr());

                                    VkClearAttachment& clearAttachment = clearAttachments.emplace_back();
                                    clearAttachment.aspectMask = ConvertImageAspectFlags(aspectFlags);
                                    clearAttachment.colorAttachment = attachmentIndex.value_or(0);
                                    FillClearValue(loadStoreAction.m_clearValue, clearAttachment.clearValue);

                                    VkClearRect& clearRect = clearRects.emplace_back();
                                    clearRect.baseArrayLayer = 0;
                                    clearRect.layerCount = framebuffer->GetSize().m_depth;
                                    clearRect.rect.offset = {};
                                    clearRect.rect.extent.width = framebuffer->GetSize().m_width;
                                    clearRect.rect.extent.height = framebuffer->GetSize().m_height;
                                }
                            }
                        }

                        if (!clearAttachments.empty())
                        {
                            static_cast<Device&>(commandList.GetDevice())
                                .GetContext()
                                .CmdClearAttachments(
                                    commandList.GetNativeCommandBuffer(),
                                    aznumeric_caster(clearAttachments.size()),
                                    clearAttachments.data(),
                                    aznumeric_caster(clearRects.size()),
                                    clearRects.data());
                        }
                    }
                }
            }
        }

        void Scope::End(CommandList& commandList, [[maybe_unused]] const RHI::FrameGraphExecuteContext& context) const
        {
            commandList.GetValidator().EndScope();
        }

        VkMemoryBarrier2 Scope::CollectMemoryBarriers(BarrierSlot slot) const
        {
            VkMemoryBarrier2 memoryBarrier = {};
            for (const auto& barrier : m_scopeBarriers[static_cast<uint32_t>(slot)])
            {
                if (barrier.m_type == VK_STRUCTURE_TYPE_MEMORY_BARRIER)
                {
                    memoryBarrier.srcStageMask |= barrier.m_srcStageMask;
                    memoryBarrier.dstStageMask |= barrier.m_dstStageMask;
                    memoryBarrier.srcAccessMask |= barrier.m_memoryBarrier.srcAccessMask;
                    memoryBarrier.dstAccessMask |= barrier.m_memoryBarrier.dstAccessMask;
                }
            }
            return memoryBarrier;
        }

        void Scope::EmitScopeBarriers(CommandList& commandList, BarrierSlot slot, BarrierTypeFlags mask) const
        {
            switch (slot)
            {
            case BarrierSlot::Prologue:
                for (RHI::ResourcePoolResolver* resolvePolicyBase : GetResourcePoolResolves())
                {
                    static_cast<ResourcePoolResolver*>(resolvePolicyBase)->QueuePrologueTransitionBarriers(commandList, mask);
                }
                break;
            case BarrierSlot::Epilogue:
                for (RHI::ResourcePoolResolver* resolvePolicyBase : GetResourcePoolResolves())
                {
                    static_cast<ResourcePoolResolver*>(resolvePolicyBase)->QueueEpilogueTransitionBarriers(commandList, mask);
                }
                break;
            default:
                break;
            }

            for (const auto& barrier : m_scopeBarriers[static_cast<uint32_t>(slot)])
            {
                if (!RHI::CheckBitsAll(mask, ConvertBarrierType(barrier.m_type)))
                {
                    continue;
                }

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

                AZStd::static_pointer_cast<QueryPool>(queryPoolAttachment.m_pool->GetDeviceQueryPool(GetDeviceIndex()))->ResetQueries(commandList, queryPoolAttachment.m_interval);
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

        const AZStd::vector<RHI::Ptr<Fence>>& Scope::GetWaitFences() const
        {
            return m_waitFences;
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
                    attachment->GetUsage() == RHI::ScopeAttachmentUsage::RenderTarget ||
                    attachment->GetUsage() == RHI::ScopeAttachmentUsage::DepthStencil ||
                    attachment->GetUsage() == RHI::ScopeAttachmentUsage::SubpassInput ||
                    attachment->GetUsage() == RHI::ScopeAttachmentUsage::ShadingRate ||
                    attachment->GetUsage() == RHI::ScopeAttachmentUsage::Resolve;

                haveClearLoadOps |=
                    attachment->GetDescriptor().m_loadStoreAction.m_loadAction == RHI::AttachmentLoadAction::Clear ||
                    attachment->GetDescriptor().m_loadStoreAction.m_loadActionStencil == RHI::AttachmentLoadAction::Clear;
            }

            if (haveRenderAttachments &&
                (   RHI::CheckBitsAll(GetActivationFlags(), RHI::Scope::ActivationFlags::Subpass) ||
                    haveClearLoadOps ||
                    GetEstimatedItemCount() > 0))
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

            for (const auto& fence : m_waitFences)
            {
                static_cast<Device&>(fence->GetDevice()).QueueForRelease(fence);
            }

            m_waitSemaphores.clear();
            m_signalSemaphores.clear();
            m_signalFences.clear();
            m_waitFences.clear();
            for (size_t i = 0; i < BarrierSlotCount; ++i)
            {
                m_unoptimizedBarriers[i].clear();
                m_scopeBarriers[i].clear();
                m_globalBarriers[i].clear();
            }

            m_usesRenderpass = false;
            m_queryPoolAttachments.clear();

            m_deviceSupportedPipelineStageFlags = 0;

            m_imageClearRequests.clear();
            m_bufferClearRequests.clear();
        }

        template<typename T>
        void CollectClearActions(const AZStd::vector<T*>& attachments, AZStd::vector<CommandList::ResourceClearRequest>& clearRequests)
        {
            for (const auto* scopeAttachment : attachments)
            {
                const auto& bindingDescriptor = scopeAttachment->GetDescriptor();
                if (HasExplicitClear(*scopeAttachment))
                {
                    clearRequests.push_back({ bindingDescriptor.m_loadStoreAction.m_clearValue, scopeAttachment->GetResourceView() });
                }
            }
        }

        void Scope::CompileInternal()
        {
            for (RHI::ResourcePoolResolver* resolvePolicyBase : GetResourcePoolResolves())
            {
                static_cast<ResourcePoolResolver*>(resolvePolicyBase)->Compile(GetHardwareQueueClass());
            }

            CollectClearActions(GetImageAttachments(), m_imageClearRequests);
            CollectClearActions(GetBufferAttachments(), m_bufferClearRequests);

            const auto& signalFences = GetFencesToSignal();
            m_signalFences.reserve(signalFences.size());
            for (const auto& fence : signalFences)
            {
                m_signalFences.push_back(AZStd::static_pointer_cast<Fence>(fence->GetDeviceFence(GetDeviceIndex())));
            }

            const auto& waitFences = GetFencesToWaitFor();
            m_waitFences.reserve(waitFences.size());
            for (const auto& fence : waitFences)
            {
                m_waitFences.push_back(AZStd::static_pointer_cast<Fence>(fence->GetDeviceFence(GetDeviceIndex())));
            }

            RHI::Device& deviceBase = GetDevice();
            Device& device = static_cast<Device&>(deviceBase);
            m_deviceSupportedPipelineStageFlags = device.GetCommandQueueContext().GetCommandQueue(GetHardwareQueueClass()).GetSupportedPipelineStages();
        }

        void Scope::AddQueryPoolUse(RHI::Ptr<RHI::QueryPool> queryPool, const RHI::Interval& interval, RHI::ScopeAttachmentAccess access)
        {
            m_queryPoolAttachments.push_back({ queryPool, interval, access });
        }

        bool Scope::CanOptimizeBarrier([[maybe_unused]] const Barrier& barrier, [[maybe_unused]] BarrierSlot slot) const
        {
            // Check that the destination pipeline stages are supported by the Pipeline
            VkPipelineStageFlags filteredDstStages = RHI::FilterBits(barrier.m_dstStageMask, GetSupportedPipelineStages(RHI::PipelineStateType::Draw));
            if (filteredDstStages == VkPipelineStageFlags(0))
            {
                return false;
            }

            BarrierOptimizationFlags optimizationFlags = GetBarrierOptimizationFlags();
            bool globalBarrierOptimizationEnabled = RHI::CheckBitsAll(optimizationFlags, BarrierOptimizationFlags::UseGlobal);
            switch (barrier.m_type)
            {
            case VK_STRUCTURE_TYPE_MEMORY_BARRIER:
                return globalBarrierOptimizationEnabled;
            case VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER:
                return globalBarrierOptimizationEnabled &&
                    barrier.m_bufferBarrier.srcQueueFamilyIndex == barrier.m_bufferBarrier.dstQueueFamilyIndex;
            case VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER:
                if (barrier.m_imageBarrier.srcQueueFamilyIndex == barrier.m_imageBarrier.dstQueueFamilyIndex)
                {
                    // Barriers between subpasses have to be "optimized" as subpass dependencies and subpass layout transitions (for
                    // pipeline compatibilty) Barriers to external scopes (outside of the group) can be optimized if we are using a
                    // renderpass and the optimization option is enabled.
                    bool useRenderpassBarrier = UsesRenderpass() && IsRenderAttachmentUsage(barrier.m_attachment->GetUsage());
                    bool renderpassLayoutOptimizationEnabled =
                        RHI::CheckBitsAll(optimizationFlags, BarrierOptimizationFlags::UseRenderpassLayout);
                    useRenderpassBarrier &= (renderpassLayoutOptimizationEnabled || !HasExternalConnection(barrier.m_attachment, slot)) &&
                        AreDepthStencilLayoutsCompatible(barrier, slot);
                    if (useRenderpassBarrier ||
                        (globalBarrierOptimizationEnabled &&
                         barrier.m_imageBarrier.oldLayout == barrier.m_imageBarrier.newLayout)) // If the layout doesn't change we can just
                                                                                                // add it to the global barrier (if enabled)
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
                return false;
            default:
                AZ_Assert(false, "Invalid memory barrier type.");
                return false;
            }
        }

        bool Scope::AreDepthStencilLayoutsCompatible(const Barrier& barrier, BarrierSlot slot) const
        {
            // The two aspects of a depth stencil image could be in incompatible layouts so that an automatic layout transition
            // by the renderpass doesn't work. In that case we will have two barriers each transitioning one aspect and we
            // cannot optimize them. First we check if the usage is DepthStencil, there is only a single aspect and the layout
            // changes.
            if (barrier.m_attachment->GetUsage() == RHI::ScopeAttachmentUsage::DepthStencil &&
                az_popcnt_u32(barrier.m_imageBarrier.subresourceRange.aspectMask) < 2 &&
                barrier.m_imageBarrier.oldLayout != barrier.m_imageBarrier.newLayout)
            {
                const Image& image = static_cast<const Image&>(
                    barrier.m_attachment->GetResourceView()->GetDeviceResourceView(GetDeviceIndex())->GetResource());

                // Next, we check if the image actually has both a depth and a stencil aspect.
                if (az_popcnt_u32(AZStd::to_underlying(image.GetAspectFlags())) == 2)
                {
                    VkImageLayout layout{ VK_IMAGE_LAYOUT_UNDEFINED };
                    VkImageAspectFlags aspectFlags{ VK_IMAGE_ASPECT_NONE };

                    // Finally, we need to go through all unoptimizedBarriers and for the barriers handling the same image, we
                    // need to check if the layouts are compatible. E.g., DEPTH_READ_ONLY_OPTIMAL is compatible with
                    // STENCIL_ATTACHMENT_OPTIMAL, but TRANSFER_DEST is not compatible with either of them.
                    for (const Barrier& otherBarrier : m_unoptimizedBarriers[AZStd::to_underlying(slot)])
                    {
                        if (otherBarrier.m_type == VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER &&
                            otherBarrier.m_imageBarrier.srcQueueFamilyIndex == otherBarrier.m_imageBarrier.dstQueueFamilyIndex &&
                            otherBarrier.m_imageBarrier.image == barrier.m_imageBarrier.image &&
                            otherBarrier.m_attachment->GetUsage() == RHI::ScopeAttachmentUsage::DepthStencil &&
                            az_popcnt_u32(otherBarrier.m_imageBarrier.subresourceRange.aspectMask) == 1)
                        {
                            // First found barrier is used to initialize aspectFlags and layout
                            if (aspectFlags == VK_IMAGE_ASPECT_NONE)
                            {
                                aspectFlags = otherBarrier.m_imageBarrier.subresourceRange.aspectMask;
                                layout = FilterImageLayout(otherBarrier.m_imageBarrier.oldLayout, ConvertImageAspectFlags(aspectFlags));
                            }
                            // The second found barrier should not overlap with the aspects
                            else if (!AZ::RHI::CheckBitsAll(aspectFlags, otherBarrier.m_imageBarrier.subresourceRange.aspectMask))
                            {
                                auto newLayout = CombineImageLayout(
                                    FilterImageLayout(layout, ConvertImageAspectFlags(aspectFlags)),
                                    FilterImageLayout(
                                        otherBarrier.m_imageBarrier.oldLayout,
                                        ConvertImageAspectFlags(otherBarrier.m_imageBarrier.subresourceRange.aspectMask)));

                                // CombineImageLayout returns the first parameter if the layouts are incompatible
                                if (newLayout == layout)
                                {
                                    return false;
                                }

                                layout = newLayout;
                                aspectFlags |= otherBarrier.m_imageBarrier.subresourceRange.aspectMask;
                            }
                        }
                    }
                }
            }

            return true;
        }

        void Scope::OptimizeBarrier(const Barrier& unoptimizedBarrier, BarrierSlot slot)
        {
            uint32_t slotIndex = aznumeric_caster(slot);
            bool canOptimize = CanOptimizeBarrier(unoptimizedBarrier, slot);
            auto filteredBarrier = unoptimizedBarrier;
            filteredBarrier.m_dstStageMask = RHI::FilterBits(unoptimizedBarrier.m_dstStageMask, m_deviceSupportedPipelineStageFlags);
            filteredBarrier.m_srcStageMask = RHI::FilterBits(unoptimizedBarrier.m_srcStageMask, m_deviceSupportedPipelineStageFlags);

            AZStd::vector<Barrier>& barriers = canOptimize ? m_globalBarriers[slotIndex] : m_scopeBarriers[slotIndex];

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
        }

        void Scope::OptimizeBarriers()
        {
            // Group resource barriers into a global barrier when possible. This seems to be more efficient than having multiple barriers
            // for each resource. Ownership transfers and layout transitions for non render attachments cannot be group into a global barrier.
            uint32_t prologueIndex = aznumeric_caster(BarrierSlot::Prologue);
            uint32_t epilogueIndex = aznumeric_caster(BarrierSlot::Epilogue);
            // Traverse in reverse order because we can only optimize a barrier if we know that
            // all subsequent barriers over the same resource can also be optimize (so the order is preserve).
            // This is because the global barrier is executed at the end of the prologue scope barriers.
            // By traversing in reverse order we can easily check if all subsequent barriers are also optimized.
            for (auto it = m_unoptimizedBarriers[prologueIndex].rbegin(); it != m_unoptimizedBarriers[prologueIndex].rend(); ++it)
            {
                OptimizeBarrier(*it, BarrierSlot::Prologue);
            }

            for (auto it = m_unoptimizedBarriers[epilogueIndex].begin(); it != m_unoptimizedBarriers[epilogueIndex].end(); ++it)
            {
                OptimizeBarrier(*it, BarrierSlot::Epilogue);
            }

            // Merge all the optimizable barriers into one global barrier.
            BuildGlobalBarriers(BarrierSlot::Prologue);
            BuildGlobalBarriers(BarrierSlot::Epilogue);

            // Other stages just get copied.
            const uint32_t resolveIndex = aznumeric_caster(BarrierSlot::Resolve);
            const uint32_t clearIndex = aznumeric_caster(BarrierSlot::Clear);
            m_scopeBarriers[resolveIndex] = m_unoptimizedBarriers[resolveIndex];
            m_scopeBarriers[clearIndex] = m_unoptimizedBarriers[clearIndex];
        }

        void Scope::BuildGlobalBarriers(BarrierSlot slot)
        {
            auto& subpassBarriers = m_globalBarriers[aznumeric_caster(slot)];
            if (subpassBarriers.empty())
            {
                return;
            }

            VkPipelineStageFlags srcStageMask = 0;
            VkPipelineStageFlags dstStageMask = 0;
            VkMemoryBarrier vkMemoryBarrier{};
            vkMemoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
            for (auto it : subpassBarriers)
            {
                // Check if we can move the barrier as a subpass dependency.
                // We don't do this for external subpass dependencies because they cause incompatiblity with
                // the renderpass used for building a pipeline state.
                if (!HasExternalConnection(it.m_attachment, slot))
                {
                    continue;
                }

                srcStageMask |= it.m_srcStageMask;
                dstStageMask |= it.m_dstStageMask;
                switch (it.m_type)
                {
                case VK_STRUCTURE_TYPE_MEMORY_BARRIER:
                    vkMemoryBarrier.srcAccessMask |= it.m_memoryBarrier.srcAccessMask;
                    vkMemoryBarrier.dstAccessMask |= it.m_memoryBarrier.dstAccessMask;
                    break;
                case VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER:
                    vkMemoryBarrier.srcAccessMask |= it.m_bufferBarrier.srcAccessMask;
                    vkMemoryBarrier.dstAccessMask |= it.m_bufferBarrier.dstAccessMask;
                    break;
                case VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER:
                    vkMemoryBarrier.srcAccessMask |= it.m_bufferBarrier.srcAccessMask;
                    vkMemoryBarrier.dstAccessMask |= it.m_bufferBarrier.dstAccessMask;
                    break;
                default:
                    AZ_Assert(false, "Invalid barrier type %s", it.m_type);
                    break;
                }
            }

            // Check for an empty memory barrier
            if (vkMemoryBarrier.srcAccessMask == 0 && vkMemoryBarrier.dstAccessMask == 0)
            {
                return;
            }

            // Add the global memory barrier.
            Barrier memoryBarrier(vkMemoryBarrier);
            memoryBarrier.m_srcStageMask = srcStageMask;
            memoryBarrier.m_dstStageMask = dstStageMask;
            memoryBarrier.m_dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
            auto& barriers = m_scopeBarriers[aznumeric_caster(slot)];
            if (slot == BarrierSlot::Epilogue)
            {
                barriers.push_back(AZStd::move(memoryBarrier));
            }
            else
            {
                barriers.insert(barriers.begin(), AZStd::move(memoryBarrier));
            }
        }

        bool Scope::IsFirstUsage(const RHI::ScopeAttachment* scopeAttachment) const
        {
            return HasExternalConnection(scopeAttachment, BarrierSlot::Prologue);
        }

        bool Scope::IsLastUsage(const RHI::ScopeAttachment* scopeAttachment) const
        {
            return HasExternalConnection(scopeAttachment, BarrierSlot::Epilogue);
        }

        bool Scope::HasExternalConnection(const RHI::ScopeAttachment* scopeAttachment, BarrierSlot slot) const
        {
            bool goForward = slot == BarrierSlot::Epilogue;
            while (scopeAttachment != nullptr && scopeAttachment->GetScope().GetId() == GetId())
            {
                scopeAttachment = goForward ? scopeAttachment->GetNext() : scopeAttachment->GetPrevious();
            }

            if (!scopeAttachment || scopeAttachment->GetScope().GetFrameGraphGroupId() != GetFrameGraphGroupId())
            {
                return true;
            }
            return false;
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
                            auto srcImageView = static_cast<const ImageView*>(imageAttachment->GetImageView()->GetDeviceImageView(GetDeviceIndex()).get());
                            auto dstImageView = static_cast<const ImageView*>(resolveAttachment->GetImageView()->GetDeviceImageView(GetDeviceIndex()).get());
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

        void Scope::SetDepthStencilFullView(RHI::ConstPtr<ImageView> view)
        {
            m_depthStencilFullView = view;
        }

        const ImageView *Scope::GetDepthStencilFullView() const
        {
            return m_depthStencilFullView.get();
        }

        Scope::ResolveMode Scope::GetResolveMode() const
        {
            return m_resolveMode;            
        }
    }
}
