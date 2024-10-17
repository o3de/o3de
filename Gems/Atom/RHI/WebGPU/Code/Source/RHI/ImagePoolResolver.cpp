/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/WebGPU.h>
#include <RHI/Buffer.h>
#include <RHI/Image.h>
#include <RHI/ImagePool.h>
#include <RHI/ImagePoolResolver.h>
#include <Atom/RHI.Reflect/Bits.h>

namespace AZ::WebGPU
{
    ImagePoolResolver::ImagePoolResolver(Device& device)
        : ResourcePoolResolver(device)
    {
    }

    RHI::ResultCode ImagePoolResolver::UpdateImage(const RHI::DeviceImageUpdateRequest& request, size_t& bytesTransferred)
    {
        auto* image = static_cast<Image*>(request.m_image);
        const auto& subresourceLayout = request.m_sourceSubresourceLayout;

        const uint32_t stagingRowPitch = subresourceLayout.m_bytesPerRow;
        const uint32_t stagingSlicePitch = subresourceLayout.m_bytesPerImage;
        const uint32_t stagingSize = stagingSlicePitch * subresourceLayout.m_size.m_depth;

        bytesTransferred = 0;

        RHI::Ptr<Buffer> stagingBuffer = m_device.AcquireStagingBuffer(stagingSize);
        if (!stagingBuffer)
        {
            return RHI::ResultCode::OutOfMemory;
        }

        m_uploadPacketsLock.lock();
        m_uploadPackets.emplace_back();
        ImageUploadPacket& uploadRequest = m_uploadPackets.back();
        m_uploadPacketsLock.unlock();

        uploadRequest.m_destinationImage = image;
        uploadRequest.m_subresourceLayout = request.m_sourceSubresourceLayout;
        uploadRequest.m_stagingBuffer = stagingBuffer;
        uploadRequest.m_subresource = request.m_imageSubresource;
        uploadRequest.m_offset = request.m_imageSubresourcePixelOffset;

        const uint8_t* subresourceDataStart = reinterpret_cast<const uint8_t*>(request.m_sourceData);
        // Buffer was mapped at creation, so we can just request the range
        uint8_t* stagingDataStart = reinterpret_cast<uint8_t*>(stagingBuffer->GetNativeBuffer().GetMappedRange(0, stagingSize));
        AZ_Assert(stagingDataStart, "Failed to map staging buffer for updating image");
        for (uint32_t depth = 0; depth < subresourceLayout.m_size.m_depth; ++depth)
        {
            uint8_t* stagingSliceDataStart = stagingDataStart + depth * stagingSlicePitch;
            const uint8_t* subresourceSliceDataStart = subresourceDataStart + depth * subresourceLayout.m_bytesPerImage;

            for (uint32_t row = 0; row < subresourceLayout.m_rowCount; ++row)
            {
                ::memcpy(stagingSliceDataStart + row * stagingRowPitch,
                    subresourceSliceDataStart + row * subresourceLayout.m_bytesPerRow,
                    stagingRowPitch);
            }
        }
        stagingBuffer->GetNativeBuffer().Unmap();
        bytesTransferred = stagingSize;
        return RHI::ResultCode::Success;
    }

    void ImagePoolResolver::Resolve(CommandList& commandList)
    {
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
        }
    }

    void ImagePoolResolver::Deactivate()
    {
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

        m_uploadPackets.erase(AZStd::remove_if(m_uploadPackets.begin(), m_uploadPackets.end(), predicatePacket), m_uploadPackets.end());
    }
}
