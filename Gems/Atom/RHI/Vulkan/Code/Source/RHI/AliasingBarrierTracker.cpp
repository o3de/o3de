/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/AliasingBarrierTracker.h>
#include <RHI/Buffer.h>
#include <RHI/Image.h>
#include <RHI/Scope.h>
#include <Atom/RHI.Reflect/Vulkan/Conversion.h>
#include <RHI/Device.h>

namespace AZ
{
    namespace Vulkan
    {
        AliasingBarrierTracker::AliasingBarrierTracker(Device& device)
            :m_device(device)
        {
        }

        void AliasingBarrierTracker::AppendBarrierInternal(const RHI::AliasedResource& beforeResource, const RHI::AliasedResource& afterResource)
        {
            // We only need a barrier if the old or new resource writes to memory.
            bool needsBarrier = false;
            VkPipelineStageFlags srcPipelineFlags = {};
            VkPipelineStageFlags dstPipelineFlags = {};
            VkAccessFlags srcAccessFlags = {};
            VkAccessFlags dstAccessFlags = {};
            const RHI::ImageBindFlags writeImageFlags = RHI::ImageBindFlags::ShaderWrite | RHI::ImageBindFlags::Color | RHI::ImageBindFlags::DepthStencil | RHI::ImageBindFlags::CopyWrite;
            const RHI::BufferBindFlags writeBufferFlags = RHI::BufferBindFlags::ShaderWrite | RHI::BufferBindFlags::CopyWrite;

            // Get the srcPipeline and srcAccessFlags from the resource (image or buffer)
            if (beforeResource.m_type == RHI::AliasedResourceType::Image)
            {
                const Image* image = static_cast<const Image*>(beforeResource.m_resource);
                const auto& bindFlags = image->GetDescriptor().m_bindFlags;
                needsBarrier |= RHI::CheckBitsAny(bindFlags, writeImageFlags);
                srcPipelineFlags = GetResourcePipelineStateFlags(bindFlags);
                srcAccessFlags = GetResourceAccessFlags(bindFlags);
            }
            else  // Buffer
            {
                const Buffer* buffer = static_cast<const Buffer*>(beforeResource.m_resource);
                const auto& bindFlags = buffer->GetDescriptor().m_bindFlags;
                needsBarrier |= RHI::CheckBitsAny(bindFlags, writeBufferFlags);
                srcPipelineFlags = GetResourcePipelineStateFlags(bindFlags);
                srcAccessFlags = GetResourceAccessFlags(bindFlags);
            }

            const auto& queueContext = m_device.GetCommandQueueContext();
            const QueueId& oldQueueId = queueContext.GetCommandQueue(beforeResource.m_endScope->GetHardwareQueueClass()).GetId();
            const QueueId& newQueueId = queueContext.GetCommandQueue(afterResource.m_beginScope->GetHardwareQueueClass()).GetId();

            if (afterResource.m_type == RHI::AliasedResourceType::Image)
            {
                Image* image = static_cast<Image*>(afterResource.m_resource);
                const auto& bindFlags = image->GetDescriptor().m_bindFlags;
                needsBarrier |= RHI::CheckBitsAny(bindFlags, writeImageFlags);
                dstPipelineFlags = GetResourcePipelineStateFlags(bindFlags);
                dstAccessFlags = GetResourceAccessFlags(bindFlags);

                if (!needsBarrier)
                {
                    return;
                }

                // If the old and new queues are different, we will add a semaphore that
                // serves as the memory dependency.
                if (oldQueueId == newQueueId)
                {
                    VkImageMemoryBarrier barrier = {};
                    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    barrier.pNext = nullptr;
                    barrier.srcAccessMask = srcAccessFlags;
                    barrier.dstAccessMask = dstAccessFlags;
                    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
                    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.image = image->GetNativeImage();
                    barrier.subresourceRange.aspectMask = image->GetImageAspectFlags();
                    barrier.subresourceRange.baseMipLevel = 0;
                    barrier.subresourceRange.levelCount = image->GetDescriptor().m_mipLevels;
                    barrier.subresourceRange.baseArrayLayer = 0;
                    barrier.subresourceRange.layerCount = image->GetDescriptor().m_arraySize;
                    static_cast<Scope*>(afterResource.m_beginScope)->QueueBarrier(Scope::BarrierSlot::Aliasing, srcPipelineFlags, dstPipelineFlags, barrier);

                    image->SetLayout(barrier.newLayout);
                    image->SetOwnerQueue(newQueueId);
                }
            }
            else // Buffer
            {
                Buffer* buffer = static_cast<Buffer*>(afterResource.m_resource);
                const auto& bindFlags = buffer->GetDescriptor().m_bindFlags;
                needsBarrier |= RHI::CheckBitsAny(bindFlags, writeBufferFlags);
                dstPipelineFlags = GetResourcePipelineStateFlags(bindFlags);
                dstAccessFlags = GetResourceAccessFlags(bindFlags);

                if (!needsBarrier)
                {
                    return;
                }

                // If the old and new queues are different, we will add a semaphore that
                // serves as the memory dependency.
                if (oldQueueId == newQueueId)
                {
                    const auto* bufferMemory = buffer->GetBufferMemoryView();
                    VkBufferMemoryBarrier barrier{};
                    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                    barrier.pNext = nullptr;
                    barrier.srcAccessMask = srcAccessFlags;
                    barrier.dstAccessMask = dstAccessFlags;
                    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.buffer = bufferMemory->GetNativeBuffer();
                    barrier.offset = bufferMemory->GetOffset() + afterResource.m_byteOffsetMin;
                    barrier.size = afterResource.m_byteOffsetMax - afterResource.m_byteOffsetMin + 1;
                    static_cast<Scope*>(afterResource.m_beginScope)->QueueBarrier(Scope::BarrierSlot::Aliasing, srcPipelineFlags, dstPipelineFlags, barrier);

                    buffer->SetOwnerQueue(newQueueId);
                }
            }

            // If resources are in different queues then we need to add an execution dependency so
            // we don't start using the aliased resource in the new scope before the old scope finishes.
            // The framegraph does not handle this case because we have to remember that from its perspective
            // these are two different resources that don't need a barrier/semaphore.
            if (oldQueueId != newQueueId)
            {
                auto semaphore = m_device.GetSemaphoreAllocator().Allocate();
                static_cast<Scope*>(beforeResource.m_endScope)->AddSignalSemaphore(semaphore);
                static_cast<Scope*>(afterResource.m_beginScope)->AddWaitSemaphore(AZStd::make_pair(dstPipelineFlags, semaphore));
                // This will not deallocate immediately.
                m_barrierSemaphores.push_back(semaphore);
            }
        }

        void AliasingBarrierTracker::EndInternal()
        {
            m_device.GetSemaphoreAllocator().DeAllocate(m_barrierSemaphores.data(), m_barrierSemaphores.size());
            m_barrierSemaphores.clear();
        }
    }
}
