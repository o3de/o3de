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
#include <RHI/Conversions.h>

namespace AZ
{
    namespace Metal
    {
        BufferPoolResolver::BufferPoolResolver(Device& device, const RHI::BufferPoolDescriptor& descriptor)
            : ResourcePoolResolver(device)
        {
        }

        void* BufferPoolResolver::MapBuffer(const RHI::DeviceBufferMapRequest& request)
        {
            AZ_Assert(request.m_byteCount > 0, "ByteCount of request is null");
            auto* buffer = static_cast<Buffer*>(request.m_buffer);
            RHI::Ptr<Buffer> stagingBuffer = m_device.AcquireStagingBuffer(request.m_byteCount, RHI::BufferBindFlags::CopyRead);
            if (stagingBuffer)
            {
                m_uploadPacketsLock.lock();
                m_uploadPackets.emplace_back();
                BufferUploadPacket& uploadRequest = m_uploadPackets.back();
                m_uploadPacketsLock.unlock();

                buffer->m_pendingResolves++;

                uploadRequest.m_attachmentBuffer = buffer;
                uploadRequest.m_byteOffset = request.m_byteOffset;
                uploadRequest.m_stagingBuffer = stagingBuffer;

                return stagingBuffer->GetMemoryView().GetCpuAddress();
            }

            return nullptr;
        }

        void BufferPoolResolver::Compile()
        {
            for (BufferUploadPacket& packet : m_uploadPackets)
            {
                Buffer* stagingBuffer = packet.m_stagingBuffer.get();
                //Inform the GPU that the CPU has modified the staging buffer.
                Platform::PublishBufferCpuChangeOnGpu(stagingBuffer->GetMemoryView().GetGpuAddress<id<MTLBuffer>>(), stagingBuffer->GetMemoryView().GetOffset(), stagingBuffer->GetMemoryView().GetSize());
            }
        }

        void BufferPoolResolver::Resolve(CommandList& commandList) const
        {
            auto& device = static_cast<Device&>(GetDevice());
            for (const BufferUploadPacket& packet : m_uploadPackets)
            {
                Buffer* stagingBuffer = packet.m_stagingBuffer.get();
                Buffer* destBuffer = packet.m_attachmentBuffer;
                AZ_Assert(stagingBuffer, "Staging Buffer is null.");
                AZ_Assert(destBuffer, "Attachment Buffer is null.");
                
                RHI::DeviceCopyBufferDescriptor copyDescriptor;
                copyDescriptor.m_sourceBuffer = stagingBuffer;
                copyDescriptor.m_sourceOffset = static_cast<uint32_t>(stagingBuffer->GetMemoryView().GetOffset());
                copyDescriptor.m_destinationBuffer = destBuffer;
                copyDescriptor.m_destinationOffset = static_cast<uint32_t>(destBuffer->GetMemoryView().GetOffset() + packet.m_byteOffset);
                copyDescriptor.m_size = static_cast<uint32_t>(stagingBuffer->GetMemoryView().GetSize());

                commandList.Submit(RHI::DeviceCopyItem(copyDescriptor));
                device.QueueForRelease(stagingBuffer->GetMemoryView());
            }
        }

        void BufferPoolResolver::Deactivate()
        {
            AZStd::for_each(m_uploadPackets.begin(), m_uploadPackets.end(), [](auto& packet)
            {
                AZ_Assert(packet.m_attachmentBuffer->m_pendingResolves, "There's no pending resolves for buffer %s", packet.m_attachmentBuffer->GetName().GetCStr());
                packet.m_attachmentBuffer->m_pendingResolves--;
            });

            m_uploadPackets.clear();
        }

        void BufferPoolResolver::OnResourceShutdown(const RHI::DeviceResource& resource)
        {
            const Buffer& buffer = static_cast<const Buffer&>(resource);
            if (!buffer.m_pendingResolves)
            {
                return;
            }

            AZStd::lock_guard<AZStd::mutex> lock(m_uploadPacketsLock);
            auto predicatePackets = [&buffer](const BufferUploadPacket& packet)
            {
                return packet.m_attachmentBuffer == &buffer;
            };
           
            const auto uploadPacketsEnd = AZStd::remove_if(m_uploadPackets.begin(), m_uploadPackets.end(), predicatePackets);
            m_uploadPackets.erase(uploadPacketsEnd, m_uploadPackets.end());
        }
        
    }
}
