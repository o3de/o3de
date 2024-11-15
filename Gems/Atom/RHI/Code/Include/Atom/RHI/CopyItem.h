/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceCopyItem.h>
#include <Atom/RHI/Buffer.h>
#include <Atom/RHI/Image.h>
#include <Atom/RHI/QueryPool.h>

namespace AZ::RHI
{
    //! A structure used to define a CopyItem, copying from a Buffer to a Buffer
    struct CopyBufferDescriptor
    {
        CopyBufferDescriptor() = default;

        //! Returns the device-specific DeviceCopyBufferDescriptor for the given index
        DeviceCopyBufferDescriptor GetDeviceCopyBufferDescriptor(int deviceIndex) const
        {
            AZ_Assert(m_sourceBuffer, "Not initialized with source Buffer\n");
            AZ_Assert(m_destinationBuffer, "Not initialized with destination Buffer\n");

            return DeviceCopyBufferDescriptor{ m_sourceBuffer ? m_sourceBuffer->GetDeviceBuffer(deviceIndex).get() : nullptr,
                                         m_sourceOffset,
                                         m_destinationBuffer ? m_destinationBuffer->GetDeviceBuffer(deviceIndex).get() : nullptr,
                                         m_destinationOffset,
                                         m_size };
        }

        const Buffer* m_sourceBuffer = nullptr;
        uint32_t m_sourceOffset = 0;
        const Buffer* m_destinationBuffer = nullptr;
        uint32_t m_destinationOffset = 0;
        uint32_t m_size = 0;
    };

    //! A structure used to define a CopyItem, copying from a Image to a Image
    struct CopyImageDescriptor
    {
        CopyImageDescriptor() = default;

        //! Returns the device-specific DeviceCopyImageDescriptor for the given index
        DeviceCopyImageDescriptor GetDeviceCopyImageDescriptor(int deviceIndex) const
        {
            AZ_Assert(m_sourceImage, "Not initialized with source Image\n");
            AZ_Assert(m_destinationImage, "Not initialized with destination Image\n");

            return DeviceCopyImageDescriptor{ m_sourceImage ? m_sourceImage->GetDeviceImage(deviceIndex).get() : nullptr,
                                        m_sourceSubresource,
                                        m_sourceOrigin,
                                        m_sourceSize,
                                        m_destinationImage ? m_destinationImage->GetDeviceImage(deviceIndex).get() : nullptr,
                                        m_destinationSubresource,
                                        m_destinationOrigin };
        }

        const Image* m_sourceImage = nullptr;
        ImageSubresource m_sourceSubresource;
        Origin m_sourceOrigin;
        Size m_sourceSize;
        const Image* m_destinationImage = nullptr;
        ImageSubresource m_destinationSubresource;
        Origin m_destinationOrigin;
    };

    //! A structure used to define a CopyItem, copying from a Buffer to a Image
    struct CopyBufferToImageDescriptor
    {
        CopyBufferToImageDescriptor() = default;

        //! Returns the device-specific DeviceCopyBufferToImageDescriptor for the given index
        DeviceCopyBufferToImageDescriptor GetDeviceCopyBufferToImageDescriptor(int deviceIndex) const
        {
            AZ_Assert(m_sourceBuffer, "Not initialized with source Buffer\n");
            AZ_Assert(m_destinationImage, "Not initialized with destination Image\n");

            return DeviceCopyBufferToImageDescriptor{ m_sourceBuffer ? m_sourceBuffer->GetDeviceBuffer(deviceIndex).get() : nullptr,
                                                      m_sourceOffset,
                                                      m_sourceBytesPerRow,
                                                      m_sourceBytesPerImage,
                                                      m_sourceFormat,
                                                      m_sourceSize,
                                                      m_destinationImage ? m_destinationImage->GetDeviceImage(deviceIndex).get() : nullptr,
                                                      m_destinationSubresource,
                                                      m_destinationOrigin };
        }

        const Buffer* m_sourceBuffer = nullptr;
        uint32_t m_sourceOffset = 0;
        uint32_t m_sourceBytesPerRow = 0;
        uint32_t m_sourceBytesPerImage = 0;
        //! The source format is usually same as m_destinationImage's format. When destination image contains more than one aspect,
        //! the format should be compatiable with the aspect of the destination image's subresource
        Format m_sourceFormat = Format::Unknown;
        Size m_sourceSize;
        const Image* m_destinationImage = nullptr;
        ImageSubresource m_destinationSubresource;
        Origin m_destinationOrigin;
    };

    //! A structure used to define a CopyItem, copying from a Image to a Buffer
    struct CopyImageToBufferDescriptor
    {
        CopyImageToBufferDescriptor() = default;

        //! Returns the device-specific DeviceCopyImageToBufferDescriptor for the given index
        DeviceCopyImageToBufferDescriptor GetDeviceCopyImageToBufferDescriptor(int deviceIndex) const
        {
            AZ_Assert(m_sourceImage, "Not initialized with source Image\n");
            AZ_Assert(m_destinationBuffer, "Not initialized with destination Buffer\n");

            return DeviceCopyImageToBufferDescriptor{ m_sourceImage ? m_sourceImage->GetDeviceImage(deviceIndex).get() : nullptr,
                                                m_sourceSubresource,
                                                m_sourceOrigin,
                                                m_sourceSize,
                                                m_destinationBuffer ? m_destinationBuffer->GetDeviceBuffer(deviceIndex).get() : nullptr,
                                                m_destinationOffset,
                                                m_destinationBytesPerRow,
                                                m_destinationBytesPerImage,
                                                m_destinationFormat };
        }

        const Image* m_sourceImage = nullptr;
        ImageSubresource m_sourceSubresource;
        Origin m_sourceOrigin;
        Size m_sourceSize;
        const Buffer* m_destinationBuffer = nullptr;
        uint32_t m_destinationOffset = 0;
        uint32_t m_destinationBytesPerRow = 0;
        uint32_t m_destinationBytesPerImage = 0;
        //! The destination format is usually same as m_sourceImage's format. When source image contains more than one aspect,
        //! the format should be compatiable with the aspect of the source image's subresource
        Format m_destinationFormat = Format::Unknown;
    };

    //! A structure used to define a CopyItem, copying from a QueryPool to a Buffer
    struct CopyQueryToBufferDescriptor
    {
        CopyQueryToBufferDescriptor() = default;

        //! Returns the device-specific DeviceCopyQueryToBufferDescriptor for the given index
        DeviceCopyQueryToBufferDescriptor GetDeviceCopyQueryToBufferDescriptor(int deviceIndex) const
        {
            AZ_Assert(m_sourceQueryPool, "Not initialized with source QueryPool\n");
            AZ_Assert(m_destinationBuffer, "Not initialized with destination Buffer\n");

            return DeviceCopyQueryToBufferDescriptor{ m_sourceQueryPool ? m_sourceQueryPool->GetDeviceQueryPool(deviceIndex).get() : nullptr,
                                                m_firstQuery,
                                                m_queryCount,
                                                m_destinationBuffer ? m_destinationBuffer->GetDeviceBuffer(deviceIndex).get() : nullptr,
                                                m_destinationOffset,
                                                m_destinationStride };
        }

        const QueryPool* m_sourceQueryPool = nullptr;
        QueryHandle m_firstQuery = QueryHandle(0);
        uint32_t m_queryCount = 0;
        const Buffer* m_destinationBuffer = nullptr;
        uint32_t m_destinationOffset = 0;
        uint32_t m_destinationStride = 0;
    };

    struct CopyItem
    {
        CopyItem()
            : m_type{ CopyItemType::Buffer }
            , m_buffer{}
        {
        }

        CopyItem(
            const CopyBufferDescriptor& descriptor, RHI::MultiDevice::DeviceMask mask = RHI::MultiDevice::AllDevices)
            : m_type{ CopyItemType::Buffer }
            , m_buffer{ descriptor }
            , m_deviceMask{ mask }
        {
        }

        CopyItem(
            const CopyImageDescriptor& descriptor, RHI::MultiDevice::DeviceMask mask = RHI::MultiDevice::AllDevices)
            : m_type{ CopyItemType::Image }
            , m_image{ descriptor }
            , m_deviceMask{ mask }
        {
        }

        CopyItem(
            const CopyBufferToImageDescriptor& descriptor, RHI::MultiDevice::DeviceMask mask = RHI::MultiDevice::AllDevices)
            : m_type{ CopyItemType::BufferToImage }
            , m_bufferToImage{ descriptor }
            , m_deviceMask{ mask }
        {
        }

        CopyItem(
            const CopyImageToBufferDescriptor& descriptor, RHI::MultiDevice::DeviceMask mask = RHI::MultiDevice::AllDevices)
            : m_type{ CopyItemType::ImageToBuffer }
            , m_imageToBuffer{ descriptor }
            , m_deviceMask{ mask }
        {
        }

        CopyItem(
            const CopyQueryToBufferDescriptor& descriptor, RHI::MultiDevice::DeviceMask mask = RHI::MultiDevice::AllDevices)
            : m_type{ CopyItemType::QueryToBuffer }
            , m_queryToBuffer{ descriptor }
            , m_deviceMask{ mask }
        {
        }

        //! Returns the device-specific DeviceCopyItem for the given index
        DeviceCopyItem GetDeviceCopyItem(int deviceIndex) const
        {
            switch (m_type)
            {
            case CopyItemType::Buffer:
                return DeviceCopyItem(m_buffer.GetDeviceCopyBufferDescriptor(deviceIndex));
            case CopyItemType::Image:
                return DeviceCopyItem(m_image.GetDeviceCopyImageDescriptor(deviceIndex));
            case CopyItemType::BufferToImage:
                return DeviceCopyItem(m_bufferToImage.GetDeviceCopyBufferToImageDescriptor(deviceIndex));
            case CopyItemType::ImageToBuffer:
                return DeviceCopyItem(m_imageToBuffer.GetDeviceCopyImageToBufferDescriptor(deviceIndex));
            case CopyItemType::QueryToBuffer:
                return DeviceCopyItem(m_queryToBuffer.GetDeviceCopyQueryToBufferDescriptor(deviceIndex));
            default:
                return DeviceCopyItem();
            }
        }

        CopyItemType m_type;
        union
        {
            CopyBufferDescriptor m_buffer;
            CopyImageDescriptor m_image;
            CopyBufferToImageDescriptor m_bufferToImage;
            CopyImageToBufferDescriptor m_imageToBuffer;
            CopyQueryToBufferDescriptor m_queryToBuffer;
        };
        //! A DeviceMask to denote on which devices an operation should take place
        RHI::MultiDevice::DeviceMask m_deviceMask;
    };
} // namespace AZ::RHI
