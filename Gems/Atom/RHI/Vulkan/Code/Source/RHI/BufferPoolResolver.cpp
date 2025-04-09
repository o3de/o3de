/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/Buffer.h>
#include <RHI/BufferPool.h>
#include <RHI/BufferPoolResolver.h>
#include <RHI/MemoryView.h>
#include <RHI/Conversion.h>

#include <algorithm>

namespace AZ
{
    namespace Vulkan
    {
        BufferPoolResolver::BufferPoolResolver(Device& device, [[maybe_unused]] const RHI::BufferPoolDescriptor& descriptor)
            : ResourcePoolResolver(device)
        {
        }

        void* BufferPoolResolver::MapBuffer(const RHI::DeviceBufferMapRequest& request)
        {
            AZ_Assert(request.m_byteCount > 0, "ByteCount of request is null");
            auto* buffer = static_cast<Buffer*>(request.m_buffer);
            RHI::Ptr<Buffer> stagingBuffer = m_device.AcquireStagingBuffer(request.m_byteCount, buffer->GetDescriptor().m_alignment);
            if (stagingBuffer)
            {
                BufferUploadPacket uploadRequest;
                uploadRequest.m_attachmentBuffer = buffer;
                uploadRequest.m_byteOffset = request.m_byteOffset;
                uploadRequest.m_stagingBuffer = stagingBuffer;
                uploadRequest.m_byteSize = request.m_byteCount;

                auto address = stagingBuffer->GetBufferMemoryView()->Map(RHI::HostMemoryAccess::Write);

                m_uploadPacketsLock.lock();
                m_uploadPackets.emplace_back(AZStd::move(uploadRequest));
                m_uploadPacketsLock.unlock();

                return address;
            }

            return nullptr;
        }

        void BufferPoolResolver::Compile(const RHI::HardwareQueueClass hardwareClass)
        {
            auto supportedQueuePipelineStages = m_device.GetCommandQueueContext().GetCommandQueue(hardwareClass).GetSupportedPipelineStages();

            VkBufferMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            barrier.pNext = nullptr;

            for (BufferUploadPacket& packet : m_uploadPackets)
            {
                packet.m_stagingBuffer->GetBufferMemoryView()->Unmap(RHI::HostMemoryAccess::Write);

                // Filter stages and access flags
                VkPipelineStageFlags bufferPipelineFlags = RHI::FilterBits(GetResourcePipelineStateFlags(packet.m_attachmentBuffer->GetDescriptor().m_bindFlags), supportedQueuePipelineStages);
                VkAccessFlags bufferAccessFlags = RHI::FilterBits(GetResourceAccessFlags(packet.m_attachmentBuffer->GetDescriptor().m_bindFlags), GetSupportedAccessFlags(bufferPipelineFlags));

                const BufferMemoryView* destBufferMemoryView = packet.m_attachmentBuffer->GetBufferMemoryView();

                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.buffer = destBufferMemoryView->GetNativeBuffer();
                barrier.offset = destBufferMemoryView->GetOffset() + packet.m_byteOffset;
                barrier.size = packet.m_byteSize;

                {
                    barrier.srcAccessMask = bufferAccessFlags;
                    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    m_prologueBarriers.emplace_back();
                    BarrierInfo& barrierInfo = m_prologueBarriers.back();
                    barrierInfo.m_srcStageMask = bufferPipelineFlags;
                    barrierInfo.m_dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
                    barrierInfo.m_barrier = barrier;
                }

                {
                    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    barrier.dstAccessMask = bufferAccessFlags;
                    m_epilogueBarriers.emplace_back();
                    BarrierInfo& barrierInfo = m_epilogueBarriers.back();
                    barrierInfo.m_srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
                    barrierInfo.m_dstStageMask = bufferPipelineFlags;
                    barrierInfo.m_barrier = barrier;
                }
            }
        }

        void BufferPoolResolver::Resolve(CommandList& commandList)
        {
            auto& device = static_cast<Device&>(commandList.GetDevice());
            for (const BufferUploadPacket& packet : m_uploadPackets)
            {
                Buffer* stagingBuffer = packet.m_stagingBuffer.get();
                Buffer* destBuffer = packet.m_attachmentBuffer;
                AZ_Assert(stagingBuffer, "Staging Buffer is null.");
                AZ_Assert(destBuffer, "Attachment Buffer is null.");

                RHI::DeviceCopyBufferDescriptor copyDescriptor;
                copyDescriptor.m_sourceBuffer = stagingBuffer;
                copyDescriptor.m_sourceOffset = 0;
                copyDescriptor.m_destinationBuffer = destBuffer;
                copyDescriptor.m_destinationOffset = static_cast<uint32_t>(packet.m_byteOffset);
                copyDescriptor.m_size = static_cast<uint32_t>(packet.m_byteSize);

                commandList.Submit(RHI::DeviceCopyItem(copyDescriptor));
                device.QueueForRelease(stagingBuffer);                
            }
        }

        void BufferPoolResolver::Deactivate()
        {
            m_uploadPackets.clear();
            m_epilogueBarriers.clear();
            m_prologueBarriers.clear();
        }

        template<class T, class Predicate>
        void EraseResourceFromList(AZStd::vector<T>& list, const Predicate& predicate)
        {
            list.erase(AZStd::remove_if(list.begin(), list.end(), predicate), list.end());
        }

        void BufferPoolResolver::OnResourceShutdown(const RHI::DeviceResource& resource)
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_uploadPacketsLock);
            const Buffer* buffer = static_cast<const Buffer*>(&resource);

            auto eraseBeginIt = std::stable_partition(
                m_uploadPackets.begin(),
                m_uploadPackets.end(),
                [&buffer](const BufferUploadPacket& packet)
                {
                    // We want the elements to erase at the back of the vector
                    // so we can easily erase them by resizing the vector.
                    return packet.m_attachmentBuffer != buffer;
                }
            );
            for (auto it = eraseBeginIt; it != m_uploadPackets.end(); ++it)
            {
                it->m_stagingBuffer->GetBufferMemoryView()->Unmap(RHI::HostMemoryAccess::Write);
            }
            m_uploadPackets.resize(AZStd::distance(m_uploadPackets.begin(), eraseBeginIt));

            auto predicateBarriers = [&buffer](const BarrierInfo& barrierInfo)
            {
                const BufferMemoryView* bufferView = buffer->GetBufferMemoryView();
                if (barrierInfo.m_barrier.buffer != bufferView->GetNativeBuffer())
                {
                    return false;
                }

                VkDeviceSize barrierBegin = barrierInfo.m_barrier.offset;
                VkDeviceSize barrierEnd = barrierBegin + barrierInfo.m_barrier.size;
                VkDeviceSize bufferBegin = bufferView->GetOffset();
                VkDeviceSize bufferEnd = bufferBegin + bufferView->GetSize();

                return barrierBegin < bufferEnd && bufferBegin < barrierEnd;
            };
            EraseResourceFromList(m_prologueBarriers, predicateBarriers);
            EraseResourceFromList(m_epilogueBarriers, predicateBarriers);
        }

        void BufferPoolResolver::QueuePrologueTransitionBarriers(CommandList& commandList, BarrierTypeFlags mask)
        {
            EmmitBarriers(commandList, m_prologueBarriers, mask);
        }

        void BufferPoolResolver::QueueEpilogueTransitionBarriers(CommandList& commandList, BarrierTypeFlags mask)
        {
            EmmitBarriers(commandList, m_epilogueBarriers, mask);
        }

        void BufferPoolResolver::EmmitBarriers(
            CommandList& commandList, const AZStd::vector<BarrierInfo>& barriers, BarrierTypeFlags mask) const
        {
            for (const BarrierInfo& barrierInfo : barriers)
            {
                if (!RHI::CheckBitsAll(mask, ConvertBarrierType(VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER)))
                {
                    continue;
                }

                m_device.GetContext().CmdPipelineBarrier(
                    commandList.GetNativeCommandBuffer(),
                    barrierInfo.m_srcStageMask,
                    barrierInfo.m_dstStageMask,
                    0,
                    0,
                    nullptr,
                    1,
                    &barrierInfo.m_barrier,
                    0,
                    nullptr);
            }
        }
    }
}
