/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/WebGPU.h>
#include <RHI/Device.h>
#include <RHI/Buffer.h>
#include <RHI/BufferPool.h>
#include <RHI/BufferPoolResolver.h>

#include <algorithm>

namespace AZ::WebGPU
{
    BufferPoolResolver::BufferPoolResolver(Device& device, [[maybe_unused]] const RHI::BufferPoolDescriptor& descriptor)
        : ResourcePoolResolver(device)
    {
    }

    void* BufferPoolResolver::MapBuffer(const RHI::DeviceBufferMapRequest& request)
    {
        AZ_Assert(request.m_byteCount > 0, "ByteCount of request is null");
        auto* buffer = static_cast<Buffer*>(request.m_buffer);
        size_t alignedSize = RHI::AlignUp(request.m_byteCount, MapSizeAligment);
        RHI::Ptr<Buffer> stagingBuffer = m_device.AcquireStagingBuffer(alignedSize, buffer->GetDescriptor().m_alignment);
        if (stagingBuffer)
        {
            BufferUploadPacket uploadRequest;
            uploadRequest.m_attachmentBuffer = buffer;
            uploadRequest.m_byteOffset = request.m_byteOffset;
            uploadRequest.m_stagingBuffer = stagingBuffer;
            uploadRequest.m_byteSize = alignedSize;

            // Buffer was mapped at creation, so we can just request the range
            auto address = stagingBuffer->GetNativeBuffer().GetMappedRange(0, alignedSize);

            m_uploadPacketsLock.lock();
            m_uploadPackets.emplace_back(AZStd::move(uploadRequest));
            m_uploadPacketsLock.unlock();

            return address;
        }

        return nullptr;
    }

    void BufferPoolResolver::Compile([[maybe_unused]] const RHI::HardwareQueueClass hardwareClass)
    {
        for (BufferUploadPacket& packet : m_uploadPackets)
        {
            packet.m_stagingBuffer->GetNativeBuffer().Unmap();
        }
    }

    void BufferPoolResolver::Resolve(CommandList& commandList)
    {
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
            stagingBuffer = nullptr;                
        }
    }

    void BufferPoolResolver::Deactivate()
    {
        m_uploadPackets.clear();
    }

    void BufferPoolResolver::OnResourceShutdown(const RHI::DeviceResource& resource)
    {
        AZStd::lock_guard<WebGPU::mutex> lock(m_uploadPacketsLock);
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
            it->m_stagingBuffer->GetNativeBuffer().Unmap();
        }
        m_uploadPackets.resize(AZStd::distance(m_uploadPackets.begin(), eraseBeginIt));
    }  
}
