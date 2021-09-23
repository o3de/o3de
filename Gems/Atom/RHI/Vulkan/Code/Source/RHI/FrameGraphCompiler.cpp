/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/BufferFrameAttachment.h>
#include <Atom/RHI/BufferScopeAttachment.h>
#include <Atom/RHI/BufferView.h>
#include <Atom/RHI/FrameGraphAttachmentDatabase.h>
#include <Atom/RHI/FrameGraph.h>
#include <Atom/RHI/ImageScopeAttachment.h>
#include <Atom/RHI/SwapChainFrameAttachment.h>
#include <AzCore/Debug/EventTrace.h>
#include <RHI/Buffer.h>
#include <RHI/BufferView.h>
#include <RHI/Conversion.h>
#include <RHI/Device.h>
#include <RHI/FrameGraphCompiler.h>
#include <RHI/Image.h>
#include <RHI/ImageView.h>
#include <RHI/Scope.h>
#include <RHI/SwapChain.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::Ptr<FrameGraphCompiler> FrameGraphCompiler::Create()
        {
            return aznew FrameGraphCompiler();
        }

        RHI::ResultCode FrameGraphCompiler::InitInternal([[maybe_unused]] RHI::Device& device)
        {
            return RHI::ResultCode::Success;
        }

        void FrameGraphCompiler::ShutdownInternal()
        {
        }

        RHI::MessageOutcome FrameGraphCompiler::CompileInternal(const RHI::FrameGraphCompileRequest& request)
        {
            AZ_PROFILE_SCOPE(RHI, "FrameGraphCompiler: CompileInternal(Vulkan)");

            AZ_Assert(request.m_frameGraph, "FrameGraph is null.");
            RHI::FrameGraph& frameGraph = *request.m_frameGraph;
            
            CompileResourceBarriers(frameGraph.GetAttachmentDatabase());

            if (!RHI::CheckBitsAny(request.m_compileFlags, RHI::FrameSchedulerCompileFlags::DisableAsyncQueues))
            {
                CompileAsyncQueueSemaphores(frameGraph);
            }

            return AZ::Success();
        }

        bool NeedsClearBarrier(const RHI::ScopeAttachment& scopeAttachment, const RHI::ScopeAttachmentDescriptor& descriptor)
        {
            if (descriptor.m_loadStoreAction.m_loadAction != RHI::AttachmentLoadAction::Clear && descriptor.m_loadStoreAction.m_loadActionStencil != RHI::AttachmentLoadAction::Clear)
            {
                return false;
            }

            for (const auto& usageAndAccess : scopeAttachment.GetUsageAndAccess())
            {
                // Render attachments are cleared by the renderpass, so they don't need
                // an explicit call to "clear" (and no barrier)
                switch (usageAndAccess.m_usage)
                {
                case RHI::ScopeAttachmentUsage::RenderTarget:
                case RHI::ScopeAttachmentUsage::DepthStencil:
                case RHI::ScopeAttachmentUsage::Resolve:
                case RHI::ScopeAttachmentUsage::SubpassInput:
                    continue;
                default:
                    return true;

                }
            }

            return false;
        }

        void FrameGraphCompiler::CompileResourceBarriers(const RHI::FrameGraphAttachmentDatabase& attachmentDatabase)
        {
            AZ_PROFILE_SCOPE(RHI, "FrameGraphCompiler: CompileResourceBarriers(Vulkan)");

             for (RHI::BufferFrameAttachment* bufferFrameAttachment : attachmentDatabase.GetBufferAttachments())
             {
                 CompileBufferBarriers(*bufferFrameAttachment);
             }

             for (RHI::ImageFrameAttachment* imageFrameAttachment : attachmentDatabase.GetImageAttachments())
             {
                 CompileImageBarriers(*imageFrameAttachment);
             }
        }

        void FrameGraphCompiler::CompileBufferBarriers(RHI::BufferFrameAttachment& frameGraphAttachment)
        {
            auto& device = static_cast<Device&>(GetDevice());
            auto& queueContext = device.GetCommandQueueContext();

            RHI::BufferScopeAttachment* scopeAttachment = frameGraphAttachment.GetFirstScopeAttachment();
            Buffer& buffer = static_cast<Buffer&>(*frameGraphAttachment.GetBuffer());
            while (scopeAttachment)
            {
                Scope& scope = static_cast<Scope&>(scopeAttachment->GetScope());
                if (NeedsClearBarrier(*scopeAttachment, scopeAttachment->GetDescriptor()))
                {
                    // We need to add a barrier before clearing the buffer.
                    RHI::BufferScopeAttachment clearAttachment(
                        scopeAttachment->GetScope(),
                        scopeAttachment->GetFrameAttachment(),
                        RHI::ScopeAttachmentUsage::Copy,
                        RHI::ScopeAttachmentAccess::ReadWrite,
                        scopeAttachment->GetDescriptor());
                    clearAttachment.SetBufferView(scopeAttachment->GetBufferView());

                    const QueueId ownerQueueId = queueContext.GetCommandQueue(scope.GetHardwareQueueClass()).GetId();
                    QueueResourceBarrier(
                        scope,
                        clearAttachment,
                        buffer,
                        RHI::BufferSubresourceRange(scopeAttachment->GetBufferView()->GetDescriptor()),
                        Scope::BarrierSlot::Clear,
                        GetResourcePipelineStateFlags(*scopeAttachment),
                        GetResourceAccessFlags(*scopeAttachment),
                        ownerQueueId,
                        ownerQueueId);
                }

                CompileScopeAttachment<RHI::BufferScopeAttachment, Buffer>(*scopeAttachment, buffer);
                scopeAttachment = scopeAttachment->GetNext();
            }
        }

        void FrameGraphCompiler::CompileImageBarriers(RHI::ImageFrameAttachment& imageFrameAttachment)
        {
            auto& device = static_cast<Device&>(GetDevice());
            auto& queueContext = device.GetCommandQueueContext();

            Image& image = static_cast<Image&>(*imageFrameAttachment.GetImage());
            RHI::ImageScopeAttachment* scopeAttachment = imageFrameAttachment.GetFirstScopeAttachment();
            while (scopeAttachment)
            {
                Scope& scope = static_cast<Scope&>(scopeAttachment->GetScope());
                if (NeedsClearBarrier(*scopeAttachment, scopeAttachment->GetDescriptor()))
                {
                    RHI::ImageScopeAttachment clearAttachment(
                        scopeAttachment->GetScope(),
                        scopeAttachment->GetFrameAttachment(),
                        RHI::ScopeAttachmentUsage::Copy,
                        RHI::ScopeAttachmentAccess::ReadWrite,
                        scopeAttachment->GetDescriptor());
                    clearAttachment.SetImageView(scopeAttachment->GetImageView());

                    // We need to add layout transition to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL in order to
                    // clear the texture.
                    const QueueId ownerQueueId = queueContext.GetCommandQueue(scope.GetHardwareQueueClass()).GetId();
                    QueueResourceBarrier(
                        scope,
                        clearAttachment,
                        image,
                        static_cast<const ImageView*>(scopeAttachment->GetImageView())->GetImageSubresourceRange(),
                        Scope::BarrierSlot::Clear,
                        GetResourcePipelineStateFlags(*scopeAttachment),
                        GetResourceAccessFlags(*scopeAttachment),
                        ownerQueueId,
                        ownerQueueId);
                }

                CompileScopeAttachment<RHI::ImageScopeAttachment, Image>(*scopeAttachment, image);

                // Check if we are resolving using the command list function because we need to insert a
                // layout transition to Transfer Source.
                if (scopeAttachment->IsBeingResolved() &&
                    scope.GetResolveMode() == Scope::ResolveMode::CommandList)
                {
                    // So we can reuse the same functions, we create a new image scope attachment
                    // with a copy read usage.
                    RHI::ImageScopeAttachment resolveSourceAttachment(
                        scopeAttachment->GetScope(),
                        scopeAttachment->GetFrameAttachment(),
                        RHI::ScopeAttachmentUsage::Copy,
                        RHI::ScopeAttachmentAccess::Read,
                        scopeAttachment->GetDescriptor());
                    resolveSourceAttachment.SetImageView(scopeAttachment->GetImageView());

                    // We need to add layout transition to VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL in order to
                    // use the command list resolve function.
                    const QueueId ownerQueueId = queueContext.GetCommandQueue(scope.GetHardwareQueueClass()).GetId();
                    QueueResourceBarrier(
                        scope,
                        resolveSourceAttachment,
                        image,
                        static_cast<const ImageView*>(scopeAttachment->GetImageView())->GetImageSubresourceRange(),
                        Scope::BarrierSlot::Resolve,
                        GetResourcePipelineStateFlags(*scopeAttachment),
                        GetResourceAccessFlags(*scopeAttachment),
                        ownerQueueId,
                        ownerQueueId);
                }
                scopeAttachment = scopeAttachment->GetNext();
            }

            /**
             * If this is the last usage of a swap chain, we require that it be in the common state for presentation.
             */
            if (auto swapchainAttachment = azrtti_cast<RHI::SwapChainFrameAttachment*>(&imageFrameAttachment))
            {
                SwapChain* swapChain = static_cast<SwapChain*>(swapchainAttachment->GetSwapChain());
                const SwapChain::FrameContext& frameContext = swapChain->GetCurrentFrameContext();

                // We need to wait until the presentation engine finish presenting the swapchain image
                Scope& firstScope = static_cast<Scope&>(imageFrameAttachment.GetFirstScopeAttachment()->GetScope());
                firstScope.AddWaitSemaphore(AZStd::make_pair(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, frameContext.m_imageAvailableSemaphore));

                auto* lastScopeAttachment = imageFrameAttachment.GetLastScopeAttachment();
                Scope& lastScope = static_cast<Scope&>(lastScopeAttachment->GetScope());
                QueueId srcQueueId = queueContext.GetCommandQueue(lastScope.GetHardwareQueueClass()).GetId();
                QueueId dstQueueId = swapChain->GetPresentationQueue().GetId();
                VkImageMemoryBarrier imageBarrier = {};
                imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                imageBarrier.image = image.GetNativeImage();
                imageBarrier.srcQueueFamilyIndex = srcQueueId.m_familyIndex;
                imageBarrier.dstQueueFamilyIndex = dstQueueId.m_familyIndex;
                imageBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                imageBarrier.dstAccessMask = 0;
                imageBarrier.oldLayout = GetImageAttachmentLayout(*lastScopeAttachment);
                imageBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                imageBarrier.subresourceRange = VkImageSubresourceRange{ image.GetImageAspectFlags(), 0, 1, 0, 1 };
                
                lastScope.QueueBarrier(Scope::BarrierSlot::Epilogue, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, imageBarrier);
                image.SetLayout(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

                // If the presentation and graphic queue are in different families, then we need to transfer the
                // ownership of the swapchain image between family queues. We add a barrier to the swapchain that will be
                // executed before presenting the image.
                if (srcQueueId.m_familyIndex != dstQueueId.m_familyIndex)
                {
                    imageBarrier.srcAccessMask = 0;
                    swapChain->QueueBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, imageBarrier);
                }

                // We need a way to tell the presentation engine to wait until we finish rendering the swapchain image.
                lastScope.AddSignalSemaphore(frameContext.m_presentableSemaphore);
            }
        }

        void FrameGraphCompiler::QueueResourceBarrier(
            Scope& scope,
            RHI::ScopeAttachment& scopeAttachment,
            Buffer& buffer,
            const RHI::BufferSubresourceRange& range,
            const Scope::BarrierSlot slot, 
            VkPipelineStageFlags srcPipelineStages, 
            VkAccessFlags srcAccess, 
            const QueueId& srcQueueId,
            const QueueId& dstQueueId) const
        {
            auto& device = static_cast<Device&>(GetDevice());
            auto& queueContext = device.GetCommandQueueContext();
            QueueId scopeQueueId = queueContext.GetCommandQueue(scope.GetHardwareQueueClass()).GetId();

            // We only need buffer barriers if we are doing an ownership transfer
            // or the queues are the same. In all the other cases a semaphore will
            // provide the memory and execution dependency needed.
            if (srcQueueId == dstQueueId ||
                srcQueueId.m_familyIndex != dstQueueId.m_familyIndex)
            {
                // Check if we are transferring between queues because the src and dst access flags must be 0 for the release and request barriers.
                VkPipelineStageFlags srcPipelineStageFlags = srcQueueId == scopeQueueId ? srcPipelineStages : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                VkPipelineStageFlags dstPipelineStageFlags = dstQueueId == scopeQueueId ? GetResourcePipelineStateFlags(scopeAttachment) : VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
                VkAccessFlags srcAccessFlags = srcQueueId == scopeQueueId ? srcAccess : 0;
                VkAccessFlags dstAccessFlags = dstQueueId == scopeQueueId ? GetResourceAccessFlags(scopeAttachment) : 0;
                bool sameFamily = srcQueueId.m_familyIndex == dstQueueId.m_familyIndex;

                VkBufferMemoryBarrier bufferBarrier = {};
                bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                bufferBarrier.buffer = buffer.GetBufferMemoryView()->GetNativeBuffer();
                bufferBarrier.offset = buffer.GetBufferMemoryView()->GetOffset() + range.m_byteOffset;
                bufferBarrier.size = range.m_byteSize;
                bufferBarrier.srcAccessMask = srcAccessFlags;
                bufferBarrier.dstAccessMask = dstAccessFlags;
                bufferBarrier.srcQueueFamilyIndex = sameFamily ? VK_QUEUE_FAMILY_IGNORED : srcQueueId.m_familyIndex;
                bufferBarrier.dstQueueFamilyIndex = sameFamily ? VK_QUEUE_FAMILY_IGNORED : dstQueueId.m_familyIndex;
                scope.QueueBarrier(slot, srcPipelineStageFlags, dstPipelineStageFlags, bufferBarrier);
            }

            // Set the ownership only when queuing the acquire barrier
            if (dstQueueId == scopeQueueId)
            {
                buffer.SetOwnerQueue(dstQueueId);
            }
        }

        void FrameGraphCompiler::QueueResourceBarrier(
            Scope& scope, 
            RHI::ScopeAttachment& scopeAttachment, 
            Image& image,
            const RHI::ImageSubresourceRange& range,
            const Scope::BarrierSlot slot,
            VkPipelineStageFlags srcPipelineStages,
            VkAccessFlags srcAccess,
            const QueueId& srcQueueId,
            const QueueId& dstQueueId) const
        {
            auto& device = static_cast<Device&>(GetDevice());
            auto& queueContext = device.GetCommandQueueContext();

            // Check if we are transferring between queues because the src and dst access flags must be 0 for the release and request barriers.
            QueueId scopeQueueId = queueContext.GetCommandQueue(scope.GetHardwareQueueClass()).GetId();
            VkPipelineStageFlags srcPipelineStageFlags = srcQueueId == scopeQueueId ? srcPipelineStages : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            VkPipelineStageFlags dstPipelineStageFlags = dstQueueId == scopeQueueId ? GetResourcePipelineStateFlags(scopeAttachment) : VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            VkAccessFlags srcAccessFlags = srcQueueId == scopeQueueId ? srcAccess : 0;
            VkAccessFlags dstAccessFlags = dstQueueId == scopeQueueId ? GetResourceAccessFlags(scopeAttachment) : 0;
            bool sameFamily = srcQueueId.m_familyIndex == dstQueueId.m_familyIndex;
            VkImageLayout newLayout = GetImageAttachmentLayout(static_cast<RHI::ImageScopeAttachment&>(scopeAttachment));

            VkImageMemoryBarrier imageBarrier = {};
            imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageBarrier.image = image.GetNativeImage();
            imageBarrier.newLayout = newLayout;
            imageBarrier.srcAccessMask = srcAccessFlags;
            imageBarrier.dstAccessMask = dstAccessFlags;
            imageBarrier.srcQueueFamilyIndex = sameFamily ? VK_QUEUE_FAMILY_IGNORED : srcQueueId.m_familyIndex;
            imageBarrier.dstQueueFamilyIndex = sameFamily ? VK_QUEUE_FAMILY_IGNORED : dstQueueId.m_familyIndex;

            for (const auto& subresourceLayout : image.GetLayout(&range))
            {
                const RHI::ImageSubresourceRange& subresourceRange = subresourceLayout.m_range;
                imageBarrier.subresourceRange.aspectMask = ConvertImageAspectFlags(subresourceRange.m_aspectFlags);
                VkImageLayout vkLayout = subresourceLayout.m_property;
                imageBarrier.oldLayout = vkLayout;
                imageBarrier.subresourceRange.baseMipLevel = subresourceRange.m_mipSliceMin;
                imageBarrier.subresourceRange.levelCount = subresourceRange.m_mipSliceMax - subresourceRange.m_mipSliceMin + 1;
                imageBarrier.subresourceRange.baseArrayLayer = subresourceRange.m_arraySliceMin;
                imageBarrier.subresourceRange.layerCount = subresourceRange.m_arraySliceMax - subresourceRange.m_arraySliceMin + 1;

                // We only need a barrier if we are changing the layout, transferring ownership
                // or the destination and source queue are the same. In all the other cases a semaphore will
                // provide the memory and execution dependency needed.
                if (imageBarrier.oldLayout != imageBarrier.newLayout ||
                    imageBarrier.srcQueueFamilyIndex != imageBarrier.dstQueueFamilyIndex ||
                    srcQueueId == dstQueueId)
                {
                    scope.QueueBarrier(slot, srcPipelineStageFlags, dstPipelineStageFlags, imageBarrier);
                }
            }

            // Set the ownership and layout only when queuing the acquire barrier.
            if (dstQueueId == scopeQueueId)
            {
                image.SetLayout(imageBarrier.newLayout, &range);
            }
        }

        void FrameGraphCompiler::CompileAsyncQueueSemaphores(const RHI::FrameGraph& frameGraph)
        {
            auto& device = static_cast<Device&>(GetDevice());
            for (RHI::Scope* scopeBase : frameGraph.GetScopes())
            {
                Scope* scope = static_cast<Scope*>(scopeBase);

                for (uint32_t hardwareQueueClassIdx = 0; hardwareQueueClassIdx < RHI::HardwareQueueClassCount; ++hardwareQueueClassIdx)
                {
                    const RHI::HardwareQueueClass hardwareQueueClass = static_cast<RHI::HardwareQueueClass>(hardwareQueueClassIdx);
                    if (scope->GetHardwareQueueClass() != hardwareQueueClass)
                    {
                        if (Scope* producer = static_cast<Scope*>(scope->GetProducerByQueue(hardwareQueueClass)))
                        {
                            auto semaphore = device.GetSemaphoreAllocator().Allocate();
                            producer->AddSignalSemaphore(semaphore);
                            scope->AddWaitSemaphore(Semaphore::WaitSemaphore(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, semaphore));
                            // This will not deallocate immediately, it has a collect latency.
                            device.GetSemaphoreAllocator().DeAllocate(semaphore);
                        }
                    }
                }
            }
        }

        RHI::ImageSubresourceRange FrameGraphCompiler::GetSubresourceRange(const RHI::ImageScopeAttachment& scopeAttachment) const
        {
            auto &physicalDevice = static_cast<const PhysicalDevice&>(GetDevice().GetPhysicalDevice());
            const auto* imageView = static_cast<const ImageView*>(scopeAttachment.GetImageView());
            auto range = RHI::ImageSubresourceRange(imageView->GetDescriptor());
            RHI::ImageAspectFlags imageAspectFlags = imageView->GetImage().GetAspectFlags();
            // If separate depth/stencil is not supported, then the barrier must ALWAYS include both image aspects (depth and stencil).
            if (!physicalDevice.IsFeatureSupported(DeviceFeature::SeparateDepthStencil) &&
                RHI::CheckBitsAll(imageAspectFlags, RHI::ImageAspectFlags::DepthStencil))
            {
                range.m_aspectFlags = RHI::SetBits(range.m_aspectFlags, RHI::ImageAspectFlags::DepthStencil);
            }
            return range;
        }

        RHI::BufferSubresourceRange FrameGraphCompiler::GetSubresourceRange(const RHI::BufferScopeAttachment& scopeAttachment) const
        {
            const auto* bufferView = scopeAttachment.GetBufferView();
            return RHI::BufferSubresourceRange(bufferView->GetDescriptor());
        }
    }
}
