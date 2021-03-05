/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include "Atom_RHI_Metal_precompiled.h"
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

        void* BufferPoolResolver::MapBuffer(const RHI::BufferMapRequest& request)
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
                uploadRequest.m_byteSize = request.m_byteCount;

                return stagingBuffer->GetMemoryView().GetCpuAddress();
            }

            return nullptr;
        }

        void BufferPoolResolver::Compile()
        {
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

                RHI::CopyBufferDescriptor copyDescriptor;
                copyDescriptor.m_sourceBuffer = stagingBuffer;
                copyDescriptor.m_sourceOffset = 0;
                copyDescriptor.m_destinationBuffer = destBuffer;
                copyDescriptor.m_destinationOffset = static_cast<uint32_t>(packet.m_byteOffset);
                copyDescriptor.m_size = static_cast<uint32_t>(packet.m_byteSize);

                commandList.Submit(RHI::CopyItem(copyDescriptor));
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

        void BufferPoolResolver::OnResourceShutdown(const RHI::Resource& resource)
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
