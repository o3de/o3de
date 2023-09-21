/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/CopyItem.h>
#include <Atom/RHI/MultiDeviceBuffer.h>
#include <Atom/RHI/MultiDeviceImage.h>
#include <Atom/RHI/MultiDeviceQueryPool.h>

namespace AZ::RHI
{
    //! A structure used to define a MultiDeviceCopyItem, copying from a MultiDeviceBuffer to a MultiDeviceBuffer
    struct MultiDeviceCopyBufferDescriptor
    {
        MultiDeviceCopyBufferDescriptor() = default;

        //! Returns the device-specific CopyBufferDescriptor for the given index
        CopyBufferDescriptor GetDeviceCopyBufferDescriptor(int deviceIndex) const
        {
            AZ_Assert(m_mdSourceBuffer, "Not initialized with source MultiDeviceBuffer\n");
            AZ_Assert(m_mdDestinationBuffer, "Not initialized with destination MultiDeviceBuffer\n");

            return CopyBufferDescriptor{ m_mdSourceBuffer ? m_mdSourceBuffer->GetDeviceBuffer(deviceIndex).get() : nullptr,
                                         m_sourceOffset,
                                         m_mdDestinationBuffer ? m_mdDestinationBuffer->GetDeviceBuffer(deviceIndex).get() : nullptr,
                                         m_destinationOffset,
                                         m_size };
        }

        const MultiDeviceBuffer* m_mdSourceBuffer = nullptr;
        uint32_t m_sourceOffset = 0;
        const MultiDeviceBuffer* m_mdDestinationBuffer = nullptr;
        uint32_t m_destinationOffset = 0;
        uint32_t m_size = 0;
    };

    //! A structure used to define a MultiDeviceCopyItem, copying from a MultiDeviceImage to a MultiDeviceImage
    struct MultiDeviceCopyImageDescriptor
    {
        MultiDeviceCopyImageDescriptor() = default;

        //! Returns the device-specific CopyImageDescriptor for the given index
        CopyImageDescriptor GetDeviceCopyImageDescriptor(int deviceIndex) const
        {
            AZ_Assert(m_mdSourceImage, "Not initialized with source MultiDeviceImage\n");
            AZ_Assert(m_mdDestinationImage, "Not initialized with destination MultiDeviceImage\n");

            return CopyImageDescriptor{ m_mdSourceImage ? m_mdSourceImage->GetDeviceImage(deviceIndex).get() : nullptr,
                                        m_sourceSubresource,
                                        m_sourceOrigin,
                                        m_sourceSize,
                                        m_mdDestinationImage ? m_mdDestinationImage->GetDeviceImage(deviceIndex).get() : nullptr,
                                        m_destinationSubresource,
                                        m_destinationOrigin };
        }

        const MultiDeviceImage* m_mdSourceImage = nullptr;
        ImageSubresource m_sourceSubresource;
        Origin m_sourceOrigin;
        Size m_sourceSize;
        const MultiDeviceImage* m_mdDestinationImage = nullptr;
        ImageSubresource m_destinationSubresource;
        Origin m_destinationOrigin;
    };

    //! A structure used to define a MultiDeviceCopyItem, copying from a MultiDeviceBuffer to a MultiDeviceImage
    struct MultiDeviceCopyBufferToImageDescriptor
    {
        MultiDeviceCopyBufferToImageDescriptor() = default;

        //! Returns the device-specific CopyBufferToImageDescriptor for the given index
        CopyBufferToImageDescriptor GetDeviceCopyBufferToImageDescriptor(int deviceIndex) const
        {
            AZ_Assert(m_mdSourceBuffer, "Not initialized with source MultiDeviceBuffer\n");
            AZ_Assert(m_mdDestinationImage, "Not initialized with destination MultiDeviceImage\n");

            return CopyBufferToImageDescriptor{ m_mdSourceBuffer ? m_mdSourceBuffer->GetDeviceBuffer(deviceIndex).get() : nullptr,
                                                m_sourceOffset,
                                                m_sourceBytesPerRow,
                                                m_sourceBytesPerImage,
                                                m_sourceSize,
                                                m_mdDestinationImage ? m_mdDestinationImage->GetDeviceImage(deviceIndex).get() : nullptr,
                                                m_destinationSubresource,
                                                m_destinationOrigin };
        }

        const MultiDeviceBuffer* m_mdSourceBuffer = nullptr;
        uint32_t m_sourceOffset = 0;
        uint32_t m_sourceBytesPerRow = 0;
        uint32_t m_sourceBytesPerImage = 0;
        Size m_sourceSize;
        const MultiDeviceImage* m_mdDestinationImage = nullptr;
        ImageSubresource m_destinationSubresource;
        Origin m_destinationOrigin;
    };

    //! A structure used to define a MultiDeviceCopyItem, copying from a MultiDeviceImage to a MultiDeviceBuffer
    struct MultiDeviceCopyImageToBufferDescriptor
    {
        MultiDeviceCopyImageToBufferDescriptor() = default;

        //! Returns the device-specific CopyImageToBufferDescriptor for the given index
        CopyImageToBufferDescriptor GetDeviceCopyImageToBufferDescriptor(int deviceIndex) const
        {
            AZ_Assert(m_mdSourceImage, "Not initialized with source MultiDeviceImage\n");
            AZ_Assert(m_mdDestinationBuffer, "Not initialized with destination MultiDeviceBuffer\n");

            return CopyImageToBufferDescriptor{ m_mdSourceImage ? m_mdSourceImage->GetDeviceImage(deviceIndex).get() : nullptr,
                                                m_sourceSubresource,
                                                m_sourceOrigin,
                                                m_sourceSize,
                                                m_mdDestinationBuffer ? m_mdDestinationBuffer->GetDeviceBuffer(deviceIndex).get() : nullptr,
                                                m_destinationOffset,
                                                m_destinationBytesPerRow,
                                                m_destinationBytesPerImage,
                                                m_destinationFormat };
        }

        const MultiDeviceImage* m_mdSourceImage = nullptr;
        ImageSubresource m_sourceSubresource;
        Origin m_sourceOrigin;
        Size m_sourceSize;
        const MultiDeviceBuffer* m_mdDestinationBuffer = nullptr;
        uint32_t m_destinationOffset = 0;
        uint32_t m_destinationBytesPerRow = 0;
        uint32_t m_destinationBytesPerImage = 0;
        //! The destination format is usually same as m_mdSourceImage's format. When source image contains more than one aspect,
        //! the format should be compatiable with the aspect of the source image's subresource
        Format m_destinationFormat;
    };

    //! A structure used to define a MultiDeviceCopyItem, copying from a MultiDeviceQueryPool to a MultiDeviceBuffer
    struct MultiDeviceCopyQueryToBufferDescriptor
    {
        MultiDeviceCopyQueryToBufferDescriptor() = default;

        //! Returns the device-specific CopyQueryToBufferDescriptor for the given index
        CopyQueryToBufferDescriptor GetDeviceCopyQueryToBufferDescriptor(int deviceIndex) const
        {
            AZ_Assert(m_mdSourceQueryPool, "Not initialized with source MultiDeviceQueryPool\n");
            AZ_Assert(m_mdDestinationBuffer, "Not initialized with destination MultiDeviceBuffer\n");

            return CopyQueryToBufferDescriptor{ m_mdSourceQueryPool ? m_mdSourceQueryPool->GetDeviceQueryPool(deviceIndex).get() : nullptr,
                                                m_firstQuery,
                                                m_queryCount,
                                                m_mdDestinationBuffer ? m_mdDestinationBuffer->GetDeviceBuffer(deviceIndex).get() : nullptr,
                                                m_destinationOffset,
                                                m_destinationStride };
        }

        const MultiDeviceQueryPool* m_mdSourceQueryPool = nullptr;
        QueryHandle m_firstQuery = QueryHandle(0);
        uint32_t m_queryCount = 0;
        const MultiDeviceBuffer* m_mdDestinationBuffer = nullptr;
        uint32_t m_destinationOffset = 0;
        uint32_t m_destinationStride = 0;
    };

    struct MultiDeviceCopyItem
    {
        MultiDeviceCopyItem()
            : m_type{ CopyItemType::Buffer }
            , m_mdBuffer{}
        {
        }

        MultiDeviceCopyItem(
            const MultiDeviceCopyBufferDescriptor& descriptor, RHI::MultiDevice::DeviceMask mask = RHI::MultiDevice::AllDevices)
            : m_type{ CopyItemType::Buffer }
            , m_mdBuffer{ descriptor }
            , m_deviceMask{ mask }
        {
        }

        MultiDeviceCopyItem(
            const MultiDeviceCopyImageDescriptor& descriptor, RHI::MultiDevice::DeviceMask mask = RHI::MultiDevice::AllDevices)
            : m_type{ CopyItemType::Image }
            , m_mdImage{ descriptor }
            , m_deviceMask{ mask }
        {
        }

        MultiDeviceCopyItem(
            const MultiDeviceCopyBufferToImageDescriptor& descriptor, RHI::MultiDevice::DeviceMask mask = RHI::MultiDevice::AllDevices)
            : m_type{ CopyItemType::BufferToImage }
            , m_mdBufferToImage{ descriptor }
            , m_deviceMask{ mask }
        {
        }

        MultiDeviceCopyItem(
            const MultiDeviceCopyImageToBufferDescriptor& descriptor, RHI::MultiDevice::DeviceMask mask = RHI::MultiDevice::AllDevices)
            : m_type{ CopyItemType::ImageToBuffer }
            , m_mdImageToBuffer{ descriptor }
            , m_deviceMask{ mask }
        {
        }

        MultiDeviceCopyItem(
            const MultiDeviceCopyQueryToBufferDescriptor& descriptor, RHI::MultiDevice::DeviceMask mask = RHI::MultiDevice::AllDevices)
            : m_type{ CopyItemType::QueryToBuffer }
            , m_mdQueryToBuffer{ descriptor }
            , m_deviceMask{ mask }
        {
        }

        //! Returns the device-specific CopyItem for the given index
        CopyItem GetDeviceCopyItem(int deviceIndex) const
        {
            switch (m_type)
            {
            case CopyItemType::Buffer:
                return CopyItem(m_mdBuffer.GetDeviceCopyBufferDescriptor(deviceIndex));
            case CopyItemType::Image:
                return CopyItem(m_mdImage.GetDeviceCopyImageDescriptor(deviceIndex));
            case CopyItemType::BufferToImage:
                return CopyItem(m_mdBufferToImage.GetDeviceCopyBufferToImageDescriptor(deviceIndex));
            case CopyItemType::ImageToBuffer:
                return CopyItem(m_mdImageToBuffer.GetDeviceCopyImageToBufferDescriptor(deviceIndex));
            case CopyItemType::QueryToBuffer:
                return CopyItem(m_mdQueryToBuffer.GetDeviceCopyQueryToBufferDescriptor(deviceIndex));
            default:
                return CopyItem();
            }
        }

        CopyItemType m_type;
        union {
            MultiDeviceCopyBufferDescriptor m_mdBuffer;
            MultiDeviceCopyImageDescriptor m_mdImage;
            MultiDeviceCopyBufferToImageDescriptor m_mdBufferToImage;
            MultiDeviceCopyImageToBufferDescriptor m_mdImageToBuffer;
            MultiDeviceCopyQueryToBufferDescriptor m_mdQueryToBuffer;
        };
        //! A DeviceMask to denote on which devices an operation should take place
        RHI::MultiDevice::DeviceMask m_deviceMask;
    };
} // namespace AZ::RHI
