/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RHI/Buffer.h>
#include <RHI/BufferPool.h>
#include <RHI/Device.h>
#include <RHI/Image.h>
#include <RHI/ImagePool.h>
#include <RHI/ImagePoolResolver.h>

namespace AZ
{
    namespace Metal
    {
        ImagePoolResolver::ImagePoolResolver(Device& device)
            : ResourcePoolResolver(device)
        {
        }
    
        RHI::ResultCode ImagePoolResolver::UpdateImage(const RHI::DeviceImageUpdateRequest& request, size_t& bytesTransferred)
        {
            Image* image = static_cast<Image*>(request.m_image);

            const RHI::DeviceImageSubresourceLayout& sourceSubresourceLayout = request.m_sourceSubresourceLayout;

            const uint32_t stagingRowPitch = sourceSubresourceLayout.m_bytesPerRow;
            const uint32_t stagingSlicePitch = sourceSubresourceLayout.m_bytesPerImage;
            const uint32_t stagingSize = stagingSlicePitch * sourceSubresourceLayout.m_size.m_depth;
            
            RHI::Ptr<Buffer> stagingBuffer = m_device.AcquireStagingBuffer(stagingSize, RHI::BufferBindFlags::CopyRead);
            if (!stagingBuffer)
            {
                return RHI::ResultCode::OutOfMemory;
            }

            {
                AZStd::lock_guard<AZStd::mutex> lock(m_uploadPacketsLock);
                m_uploadPackets.emplace_back();
                ImageUploadPacket& uploadRequest = m_uploadPackets.back();
                uploadRequest.m_destinationImage = image;
                uploadRequest.m_subresourceLayout = request.m_sourceSubresourceLayout;
                uploadRequest.m_stagingBuffer = stagingBuffer;
                uploadRequest.m_subresource = request.m_imageSubresource;
                uploadRequest.m_offset = request.m_imageSubresourcePixelOffset;
            }

            const uint8_t* subresourceDataStart = reinterpret_cast<const uint8_t*>(request.m_sourceData);
            uint8_t* stagingDataStart = static_cast<uint8_t*>(stagingBuffer->GetMemoryView().GetCpuAddress());
            for (uint32_t depth = 0; depth < sourceSubresourceLayout.m_size.m_depth; ++depth)
            {
                uint8_t* stagingSliceDataStart = stagingDataStart + depth * stagingSlicePitch;
                const uint8_t* subresourceSliceDataStart = subresourceDataStart + depth * sourceSubresourceLayout.m_bytesPerImage;

                for (uint32_t row = 0; row < sourceSubresourceLayout.m_rowCount; ++row)
                {
                    memcpy(stagingSliceDataStart + row * stagingRowPitch,
                        subresourceSliceDataStart + row * sourceSubresourceLayout.m_bytesPerRow,
                        stagingRowPitch);
                }
            }
            
            Platform::PublishBufferCpuChangeOnGpu(stagingBuffer->GetMemoryView().GetGpuAddress<id<MTLBuffer>>(), stagingBuffer->GetMemoryView().GetOffset(), stagingSize);
            image->m_pendingResolves++;
            bytesTransferred = stagingSize;
            return RHI::ResultCode::Success;
        }
        
        void ImagePoolResolver::Compile()
        {
        }
        
        void ImagePoolResolver::Resolve(CommandList& commandList) const
        {
            auto& device = static_cast<Device&>(GetDevice());
            for (const auto& packet : m_uploadPackets)
            {
                const RHI::DeviceImageSubresourceLayout& subresourceLayout = packet.m_subresourceLayout;
                const uint32_t stagingRowPitch = subresourceLayout.m_bytesPerRow;
                const uint32_t stagingSlicePitch = subresourceLayout.m_rowCount * stagingRowPitch;

                RHI::DeviceCopyBufferToImageDescriptor copyDescriptor;
                copyDescriptor.m_sourceBuffer = packet.m_stagingBuffer.get();
                copyDescriptor.m_sourceOffset = 0;
                copyDescriptor.m_sourceBytesPerRow = stagingRowPitch;
                copyDescriptor.m_sourceBytesPerImage = stagingSlicePitch;
                copyDescriptor.m_sourceSize = subresourceLayout.m_size;
                copyDescriptor.m_destinationImage = packet.m_destinationImage;
                copyDescriptor.m_destinationSubresource.m_mipSlice = packet.m_subresource.m_mipSlice;
                copyDescriptor.m_destinationSubresource.m_arraySlice = packet.m_subresource.m_arraySlice;
                copyDescriptor.m_destinationOrigin.m_left = packet.m_offset.m_left;
                copyDescriptor.m_destinationOrigin.m_top = packet.m_offset.m_top;
                copyDescriptor.m_destinationOrigin.m_front = packet.m_offset.m_front;

                commandList.Submit(RHI::DeviceCopyItem(copyDescriptor));
                device.QueueForRelease(packet.m_stagingBuffer->GetMemoryView());
            }
        }
        
        void ImagePoolResolver::Deactivate()
        {
            AZStd::for_each(m_uploadPackets.begin(), m_uploadPackets.end(), [](auto& packet)
            {
                AZ_Assert(packet.m_destinationImage->m_pendingResolves, "There's no pending resolves for image %s", packet.m_destinationImage->GetName().GetCStr());
                packet.m_destinationImage->m_pendingResolves--;
            });

            m_uploadPackets.clear();
        }
    
        void ImagePoolResolver::OnResourceShutdown(const RHI::DeviceResource& resource)
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_uploadPacketsLock);
            const Image* image = static_cast<const Image*>(&resource);
            auto predicatePacket = [&image](const ImageUploadPacket& packet)
            {
                return packet.m_destinationImage == image;
            };

            AZStd::remove_if(m_uploadPackets.begin(), m_uploadPackets.end(), predicatePacket);
         }

    }
}
