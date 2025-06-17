/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/BufferFrameAttachment.h>
#include <Atom/RHI/BufferScopeAttachment.h>
#include <Atom/RHI/DeviceBufferView.h>
#include <Atom/RHI/SwapChain.h>
#include <Atom/RHI/FrameGraph.h>
#include <Atom/RHI/FrameGraphAttachmentDatabase.h>
#include <Atom/RHI/ImageScopeAttachment.h>
#include <Atom/RHI/SwapChainFrameAttachment.h>
#include <AzCore/std/smart_ptr/make_shared.h>
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

        RHI::ResultCode FrameGraphCompiler::InitInternal()
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

            CompileSemaphoreSynchronization(frameGraph);

            OptimizeBarriers(request);

            return AZ::Success();
        }

        bool NeedsClearBarrier(const RHI::ScopeAttachment& scopeAttachment)
        {
            const RHI::ScopeAttachmentDescriptor& descriptor = scopeAttachment.GetScopeAttachmentDescriptor();
            if (descriptor.m_loadStoreAction.m_loadAction != RHI::AttachmentLoadAction::Clear &&
                descriptor.m_loadStoreAction.m_loadActionStencil != RHI::AttachmentLoadAction::Clear)
            {
                return false;
            }

            // Render attachments are cleared by the renderpass, so they don't need
            // an explicit call to "clear" (and no barrier)
            switch (scopeAttachment.GetUsage())
            {
            case RHI::ScopeAttachmentUsage::RenderTarget:
            case RHI::ScopeAttachmentUsage::DepthStencil:
            case RHI::ScopeAttachmentUsage::Resolve:
            case RHI::ScopeAttachmentUsage::SubpassInput:
                return false;
            default:
                return true;
            }
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
            for (int deviceIndex{ 0 }; deviceIndex < RHI::RHISystemInterface::Get()->GetDeviceCount(); ++deviceIndex)
            {
                RHI::BufferScopeAttachment* scopeAttachment = frameGraphAttachment.GetFirstScopeAttachment(deviceIndex);

                while (scopeAttachment)
                {
                    Scope& scope = static_cast<Scope&>(scopeAttachment->GetScope());

                    auto& device = static_cast<Device&>(scope.GetDevice());
                    auto& queueContext = device.GetCommandQueueContext();

                    Buffer& buffer = static_cast<Buffer&>(*frameGraphAttachment.GetBuffer()->GetDeviceBuffer(scope.GetDeviceIndex()));

                    if (NeedsClearBarrier(*scopeAttachment))
                    {
                        // We need to add a barrier before clearing the buffer.
                        RHI::BufferScopeAttachment clearAttachment(
                            scopeAttachment->GetScope(),
                            scopeAttachment->GetFrameAttachment(),
                            RHI::ScopeAttachmentUsage::Copy,
                            RHI::ScopeAttachmentAccess::ReadWrite,
                            RHI::ScopeAttachmentStage::Copy,
                            scopeAttachment->GetDescriptor());
                        clearAttachment.SetBufferView(scopeAttachment->GetBufferView());

                        const QueueId ownerQueueId = queueContext.GetCommandQueue(scope.GetHardwareQueueClass()).GetId();
                        QueueResourceBarrier(
                            scope,
                            clearAttachment,
                            buffer,
                            GetSubresourceRange(clearAttachment),
                            Scope::BarrierSlot::Clear,
                            ownerQueueId,
                            ownerQueueId);
                    }

                    CompileScopeAttachment<RHI::BufferScopeAttachment, Buffer>(*scopeAttachment, buffer);
                    scopeAttachment = scopeAttachment->GetNext();
                }
            }
        }

        void FrameGraphCompiler::CompileImageBarriers(RHI::ImageFrameAttachment& imageFrameAttachment)
        {
            for (int deviceIndex{ 0 }; deviceIndex < RHI::RHISystemInterface::Get()->GetDeviceCount(); ++deviceIndex)
            {
                RHI::ImageScopeAttachment* scopeAttachment = imageFrameAttachment.GetFirstScopeAttachment(deviceIndex);

                if (!scopeAttachment)
                {
                    continue;
                }

                while (scopeAttachment)
                {
                    Scope& scope = static_cast<Scope&>(scopeAttachment->GetScope());

                    auto& device = static_cast<Device&>(scope.GetDevice());
                    auto& queueContext = device.GetCommandQueueContext();

                    Image& image = static_cast<Image&>(*imageFrameAttachment.GetImage()->GetDeviceImage(scope.GetDeviceIndex()));

                    const QueueId ownerQueueId = queueContext.GetCommandQueue(scope.GetHardwareQueueClass()).GetId();
                    if (NeedsClearBarrier(*scopeAttachment))
                    {
                        RHI::ImageScopeAttachment clearAttachment(
                            scopeAttachment->GetScope(),
                            scopeAttachment->GetFrameAttachment(),
                            RHI::ScopeAttachmentUsage::Copy,
                            RHI::ScopeAttachmentAccess::ReadWrite,
                            RHI::ScopeAttachmentStage::Copy,
                            scopeAttachment->GetDescriptor());
                        clearAttachment.SetImageView(scopeAttachment->GetImageView());

                        // We need to add layout transition to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL in order to
                        // clear the texture.
                        QueueResourceBarrier(
                            scope,
                            clearAttachment,
                            image,
                            GetSubresourceRange(clearAttachment),
                            Scope::BarrierSlot::Clear,
                            ownerQueueId,
                            ownerQueueId);
                    }

                    CompileScopeAttachment<RHI::ImageScopeAttachment, Image>(*scopeAttachment, image);

                    // Check if we are resolving using the command list function because we need to insert a
                    // layout transition to Transfer Source.
                    if (scopeAttachment->IsBeingResolved() && scope.GetResolveMode() == Scope::ResolveMode::CommandList)
                    {
                        // So we can reuse the same functions, we create a new image scope attachment
                        // with a copy read usage.
                        RHI::ImageScopeAttachment resolveSourceAttachment(
                            scopeAttachment->GetScope(),
                            scopeAttachment->GetFrameAttachment(),
                            RHI::ScopeAttachmentUsage::Copy,
                            RHI::ScopeAttachmentAccess::Read,
                            RHI::ScopeAttachmentStage::Copy,
                            scopeAttachment->GetDescriptor());
                        resolveSourceAttachment.SetImageView(scopeAttachment->GetImageView());

                        // We need to add layout transition to VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL in order to
                        // use the command list resolve function.
                        QueueResourceBarrier(
                            scope,
                            resolveSourceAttachment,
                            image,
                            GetSubresourceRange(resolveSourceAttachment),
                            Scope::BarrierSlot::Resolve,
                            ownerQueueId,
                            ownerQueueId);
                    }
                    scopeAttachment = scopeAttachment->GetNext();
                }

                // If this is the last usage of a swap chain, we require that it be in the common state for presentation.
                if (auto swapchainAttachment = azrtti_cast<RHI::SwapChainFrameAttachment*>(&imageFrameAttachment))
                {
                    // Skip adding synchronization constructs for XR swapchain as that is managed by OpenXr api
                    if (swapchainAttachment->GetSwapChain()->GetDescriptor().m_isXrSwapChain)
                    {
                        continue;
                    }

                    auto* lastScopeAttachment = imageFrameAttachment.GetLastScopeAttachment(deviceIndex);

                    Scope& lastScope = static_cast<Scope&>(lastScopeAttachment->GetScope());
                    SwapChain* swapChain =
                        static_cast<SwapChain*>(swapchainAttachment->GetSwapChain()->GetDeviceSwapChain(deviceIndex).get());

                    Scope& firstScope = static_cast<Scope&>(imageFrameAttachment.GetFirstScopeAttachment(deviceIndex)->GetScope());
                    const SwapChain::FrameContext& frameContext = swapChain->GetCurrentFrameContext();

                    // We need to wait until the presentation engine finish presenting the swapchain image
                    firstScope.AddWaitSemaphore(
                        AZStd::make_pair(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, frameContext.m_imageAvailableSemaphore));

                    auto& device = static_cast<Device&>(lastScope.GetDevice());
                    auto& queueContext = device.GetCommandQueueContext();

                    Image& image = static_cast<Image&>(*imageFrameAttachment.GetImage()->GetDeviceImage(deviceIndex));

                    QueueId srcQueueId = queueContext.GetCommandQueue(lastScope.GetHardwareQueueClass()).GetId();
                    QueueId dstQueueId = swapChain->GetPresentationQueue().GetId();
                    VkImageMemoryBarrier imageBarrier = {};
                    imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    imageBarrier.image = image.GetNativeImage();
                    imageBarrier.srcQueueFamilyIndex = srcQueueId.m_familyIndex;
                    imageBarrier.dstQueueFamilyIndex = dstQueueId.m_familyIndex;
                    imageBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                    imageBarrier.dstAccessMask = VK_ACCESS_NONE_KHR;
                    imageBarrier.oldLayout = GetImageAttachmentLayout(*lastScopeAttachment);
                    imageBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                    imageBarrier.subresourceRange = VkImageSubresourceRange{ image.GetImageAspectFlags(), 0, 1, 0, 1 };
                    lastScope.QueueAttachmentBarrier(
                        *lastScopeAttachment,
                        Scope::BarrierSlot::Epilogue,
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                        imageBarrier);
                    image.SetLayout(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
                    image.SetPipelineAccess({ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_NONE_KHR });

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
        }

        void FrameGraphCompiler::QueueResourceBarrier(
            Scope& scope,
            RHI::ScopeAttachment& scopeAttachment,
            Buffer& buffer,
            const RHI::BufferSubresourceRange& range,
            const Scope::BarrierSlot slot,
            const QueueId& srcQueueId,
            const QueueId& dstQueueId) const
        {
            auto& device = static_cast<Device&>(scope.GetDevice());
            auto& queueContext = device.GetCommandQueueContext();
            QueueId scopeQueueId = queueContext.GetCommandQueue(scope.GetHardwareQueueClass()).GetId();

            // We only need buffer barriers if we are doing an ownership transfer
            // or the queues are the same. In all the other cases a semaphore will
            // provide the memory and execution dependency needed.
            if (srcQueueId == dstQueueId ||
                srcQueueId.m_familyIndex != dstQueueId.m_familyIndex)
            {
                // Check if we are transferring between queues because the src and dst access flags must be 0 for the release and request barriers.
                VkPipelineStageFlags srcPipelineStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                VkPipelineStageFlags dstPipelineStageFlags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
                VkAccessFlags srcAccessFlags = 0;
                VkAccessFlags dstAccessFlags = 0;
                if (srcQueueId == scopeQueueId)
                {
                    const auto pipelineAccess = buffer.GetPipelineAccess(&range);
                    srcPipelineStageFlags = pipelineAccess.m_pipelineStage;
                    dstPipelineStageFlags = GetResourcePipelineStateFlags(scopeAttachment);
                    srcAccessFlags = pipelineAccess.m_access;
                    dstAccessFlags = GetResourceAccessFlags(scopeAttachment);
                }

                bool sameFamily =
                    srcQueueId.m_familyIndex == dstQueueId.m_familyIndex || buffer.GetSharingMode() == VK_SHARING_MODE_CONCURRENT;

                VkBufferMemoryBarrier bufferBarrier = {};
                bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                bufferBarrier.buffer = buffer.GetBufferMemoryView()->GetNativeBuffer();
                bufferBarrier.offset = buffer.GetBufferMemoryView()->GetOffset() + range.m_byteOffset;
                bufferBarrier.size = range.m_byteSize;
                bufferBarrier.srcAccessMask = srcAccessFlags;
                bufferBarrier.dstAccessMask = dstAccessFlags;
                bufferBarrier.srcQueueFamilyIndex = sameFamily ? VK_QUEUE_FAMILY_IGNORED : srcQueueId.m_familyIndex;
                bufferBarrier.dstQueueFamilyIndex = sameFamily ? VK_QUEUE_FAMILY_IGNORED : dstQueueId.m_familyIndex;
                auto insertedBarrier =
                    scope.QueueAttachmentBarrier(scopeAttachment, slot, srcPipelineStageFlags, dstPipelineStageFlags, bufferBarrier);

                // Set the ownership only when queuing the acquire barrier
                if (dstQueueId == scopeQueueId)
                {
                    RHI::BufferSubresourceRange insertedRange(insertedBarrier.m_bufferBarrier.offset, insertedBarrier.m_bufferBarrier.size); 
                    buffer.SetOwnerQueue(dstQueueId, &insertedRange);
                    buffer.SetPipelineAccess(
                        { insertedBarrier.m_dstStageMask, insertedBarrier.m_bufferBarrier.dstAccessMask }, &insertedRange);
                }
            }
        }

        void FrameGraphCompiler::QueueResourceBarrier(
            Scope& scope, 
            RHI::ScopeAttachment& scopeAttachment, 
            Image& image,
            const RHI::ImageSubresourceRange& range,
            const Scope::BarrierSlot slot,
            const QueueId& srcQueueId,
            const QueueId& dstQueueId) const
        {
            auto& device = static_cast<Device&>(scope.GetDevice());
            auto& queueContext = device.GetCommandQueueContext();

            // Check if we are transferring between queues because the src and dst access flags must be 0 for the release and request barriers.
            QueueId scopeQueueId = queueContext.GetCommandQueue(scope.GetHardwareQueueClass()).GetId();
            bool sameFamily = srcQueueId.m_familyIndex == dstQueueId.m_familyIndex || image.GetSharingMode() == VK_SHARING_MODE_CONCURRENT;
            VkImageLayout newLayout = GetImageAttachmentLayout(static_cast<const RHI::ImageScopeAttachment&>(scopeAttachment));

            VkImageMemoryBarrier imageBarrier = {};
            imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageBarrier.image = image.GetNativeImage();
            imageBarrier.newLayout = newLayout;
            imageBarrier.srcQueueFamilyIndex = sameFamily ? VK_QUEUE_FAMILY_IGNORED : srcQueueId.m_familyIndex;
            imageBarrier.dstQueueFamilyIndex = sameFamily ? VK_QUEUE_FAMILY_IGNORED : dstQueueId.m_familyIndex;

            for (const auto& subresourceLayout : image.GetLayout(&range))
            {
                const RHI::ImageSubresourceRange& subresourceRange = subresourceLayout.m_range;
                VkPipelineStageFlags srcPipelineStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                VkPipelineStageFlags dstPipelineStageFlags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
                VkAccessFlags srcAccessFlags = 0;
                VkAccessFlags dstAccessFlags = 0;
                if (srcQueueId == scopeQueueId)
                {
                    const auto pipelineAccess = image.GetPipelineAccess(&subresourceRange);
                    srcPipelineStageFlags = pipelineAccess.m_pipelineStage;
                    dstPipelineStageFlags = GetResourcePipelineStateFlags(scopeAttachment);
                    srcAccessFlags = pipelineAccess.m_access;
                    dstAccessFlags = GetResourceAccessFlags(scopeAttachment);
                }

                imageBarrier.srcAccessMask = srcAccessFlags;
                imageBarrier.dstAccessMask = dstAccessFlags;
                imageBarrier.subresourceRange.aspectMask = ConvertImageAspectFlags(subresourceRange.m_aspectFlags);
                imageBarrier.oldLayout = subresourceLayout.m_property;
                imageBarrier.subresourceRange.baseMipLevel = subresourceRange.m_mipSliceMin;
                imageBarrier.subresourceRange.levelCount = subresourceRange.m_mipSliceMax - subresourceRange.m_mipSliceMin + 1;
                imageBarrier.subresourceRange.baseArrayLayer = subresourceRange.m_arraySliceMin;
                imageBarrier.subresourceRange.layerCount = subresourceRange.m_arraySliceMax - subresourceRange.m_arraySliceMin + 1;

                // We only need a barrier if we are changing the layout, transferring ownership
                // or the destination and source queue are the same. In all the other cases a semaphore will
                // provide the memory and execution dependency needed.
                VkImageLayout finalLayout = newLayout;
                VkPipelineStageFlags finalStageFlags = dstPipelineStageFlags;
                VkAccessFlags finalAccessFlags = dstAccessFlags;
                RHI::ImageSubresourceRange finalRange = subresourceRange;
                if (imageBarrier.oldLayout != imageBarrier.newLayout ||
                    imageBarrier.srcQueueFamilyIndex != imageBarrier.dstQueueFamilyIndex ||
                    srcQueueId == dstQueueId)
                {
                    auto insertedBarrier =
                        scope.QueueAttachmentBarrier(scopeAttachment, slot, srcPipelineStageFlags, dstPipelineStageFlags, imageBarrier);
                    finalLayout = insertedBarrier.m_imageBarrier.newLayout;
                    finalStageFlags = insertedBarrier.m_dstStageMask;
                    finalAccessFlags = insertedBarrier.m_imageBarrier.dstAccessMask;
                    finalRange = ConvertSubresourceRange(insertedBarrier.m_imageBarrier.subresourceRange);
                }

                // Set the ownership and layout only when queuing the acquire barrier.
                if (dstQueueId == scopeQueueId)
                {
                    image.SetOwnerQueue(dstQueueId, &finalRange);
                    image.SetLayout(finalLayout, &finalRange);
                    image.SetPipelineAccess({ finalStageFlags, finalAccessFlags }, &finalRange);
                }
            }
        }

        void FrameGraphCompiler::CompileAsyncQueueSemaphores(const RHI::FrameGraph& frameGraph)
        {
            for (RHI::Scope* scopeBase : frameGraph.GetScopes())
            {
                Scope* scope = static_cast<Scope*>(scopeBase);
                auto& device = static_cast<Device&>(scope->GetDevice());

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
            const auto& scope = scopeAttachment.GetScope();
            auto &physicalDevice = static_cast<const PhysicalDevice&>(scope.GetDevice().GetPhysicalDevice());
            const auto* imageView = static_cast<const ImageView*>(scopeAttachment.GetImageView()->GetDeviceImageView(scope.GetDeviceIndex()).get());
            auto range = imageView->GetImageSubresourceRange();

            // If separate depth/stencil is not supported, then the barrier must ALWAYS include both image aspects (depth and stencil).
            if (!physicalDevice.IsFeatureSupported(DeviceFeature::SeparateDepthStencil) &&
                RHI::CheckBitsAll(range.m_aspectFlags, RHI::ImageAspectFlags::DepthStencil))
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

        void FrameGraphCompiler::CompileSemaphoreSynchronization(const RHI::FrameGraph& frameGraph)
        {
            // We need to synchronize submissions of binary semaphores/fences to the queues:
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkQueuePresentKHR.html#VUID-vkQueuePresentKHR-pWaitSemaphores-03268
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkQueueSubmit.html#VUID-vkQueueSubmit-pWaitSemaphores-03238
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkQueueSubmit.html#VUID-vkQueueSubmit-fence-00063
            // The wait operation of a binary semaphore/fence cannot be submitted before the respective signal operation is submitted.
            // Timeline semaphores don't have that limitation,
            //   but all dependent semaphores of a binary semaphore must be signalled before waiting (including timeline semaphore)
            // Here every semaphore (scope) gets assigned an index in a bitfield, and which other semaphores it depends on
            // There is one bit set for every scope, instead of every semaphore/fence
            //   because all semaphores of a scope are waited-for/signalled at the same time
            AZStd::shared_ptr<SignalEvent> signalEvent = AZStd::make_shared<SignalEvent>();
            int currentBitToSignal{ 0 };
            SignalEvent::BitSet currentDependencies{};
            AZStd::unordered_set<Fence*> signalledFences;

            for (RHI::Scope* scopeBase : frameGraph.GetScopes())
            {
                bool hasSemaphoreSignal = false;
                Scope* scope = static_cast<Scope*>(scopeBase);
                for (auto& fence : scope->GetWaitFences())
                {
                    fence->SetSignalEvent(signalEvent);
                    fence->SetSignalEventDependencies(currentDependencies);
                    if (!signalledFences.contains(fence.get()))
                    {
                        // We assume the fence is signalled on the CPU
                        fence->SetSignalEventBitToSignal(currentBitToSignal);
                        hasSemaphoreSignal = true;
                    }
                }
                for (auto& semaphore : scope->GetWaitSemaphores())
                {
                    semaphore.second->SetSignalEvent(signalEvent);
                    semaphore.second->SetSignalEventDependencies(currentDependencies);
                }

                for (auto& fence : scope->GetSignalFences())
                {
                    fence->SetSignalEvent(signalEvent);
                    fence->SetSignalEventBitToSignal(currentBitToSignal);
                    signalledFences.insert(fence.get());
                    // The fence wait might not be on the framegraph (wait on CPU)
                    // In this case we want wait for the current scopes dependencies (i.e. for signalling of the fence itself)
                    // If the Fence is waited-for on the framegraph at a later scope, we overwrite the dependencies later
                    SignalEvent::BitSet fenceDependencies = currentDependencies;
                    fenceDependencies.set(currentBitToSignal);
                    fence->SetSignalEventDependencies(fenceDependencies);
                    hasSemaphoreSignal = true;
                }
                for (auto& semaphore : scope->GetSignalSemaphores())
                {
                    semaphore->SetSignalEvent(signalEvent);
                    semaphore->SetSignalEventBitToSignal(currentBitToSignal);
                    hasSemaphoreSignal = true;
                }
                if (hasSemaphoreSignal)
                {
                    currentDependencies.set(currentBitToSignal);
                    currentBitToSignal++;
                }
            }
        }

        RHI::Scope* FrameGraphCompiler::FindPreviousScope(RHI::ScopeAttachment& scopeAttachment) const
        {
            for (auto* prev = scopeAttachment.GetPrevious(); prev != nullptr; prev = prev->GetPrevious())
            {
                if (prev->GetScope().GetId() != scopeAttachment.GetScope().GetId())
                {
                    return &prev->GetScope();
                }
            }
            return nullptr;
        }

        void FrameGraphCompiler::OptimizeBarriers(const RHI::FrameGraphCompileRequest& request)
        {
            RHI::FrameGraph& frameGraph = *request.m_frameGraph;
            for (RHI::Scope* scope : frameGraph.GetScopes())
            {
                static_cast<Scope*>(scope)->OptimizeBarriers();
            }            
        }
    } // namespace Vulkan
} // namespace AZ
