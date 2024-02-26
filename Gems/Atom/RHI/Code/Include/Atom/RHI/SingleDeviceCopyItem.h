/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/SingleDeviceImage.h>
#include <Atom/RHI/SingleDeviceBuffer.h>
#include <Atom/RHI/SingleDeviceQueryPool.h>

namespace AZ::RHI
{
    struct SingleDeviceCopyBufferDescriptor
    {
        SingleDeviceCopyBufferDescriptor() = default;

        const SingleDeviceBuffer* m_sourceBuffer = nullptr;
        uint32_t m_sourceOffset = 0;
        const SingleDeviceBuffer* m_destinationBuffer = nullptr;
        uint32_t m_destinationOffset = 0;
        uint32_t m_size = 0;
    };

    struct SingleDeviceCopyImageDescriptor
    {
        SingleDeviceCopyImageDescriptor() = default;

        const SingleDeviceImage* m_sourceImage = nullptr;
        ImageSubresource m_sourceSubresource;
        Origin m_sourceOrigin;
        Size m_sourceSize;
        const SingleDeviceImage* m_destinationImage = nullptr;
        ImageSubresource m_destinationSubresource;
        Origin m_destinationOrigin;
    };

    struct SingleDeviceCopyBufferToImageDescriptor
    {
        SingleDeviceCopyBufferToImageDescriptor() = default;

        const SingleDeviceBuffer* m_sourceBuffer = nullptr;
        uint32_t m_sourceOffset = 0;
        uint32_t m_sourceBytesPerRow = 0;
        uint32_t m_sourceBytesPerImage = 0;
        Size m_sourceSize;
        const SingleDeviceImage* m_destinationImage = nullptr;
        ImageSubresource m_destinationSubresource;
        Origin m_destinationOrigin;
    };

    struct SingleDeviceCopyImageToBufferDescriptor
    {
        SingleDeviceCopyImageToBufferDescriptor() = default;

        const SingleDeviceImage* m_sourceImage = nullptr;
        ImageSubresource m_sourceSubresource;
        Origin m_sourceOrigin;
        Size m_sourceSize;
        const  SingleDeviceBuffer* m_destinationBuffer = nullptr;
        uint32_t m_destinationOffset = 0;
        uint32_t m_destinationBytesPerRow = 0;
        uint32_t m_destinationBytesPerImage = 0;
        // The destination format is usually same as m_sourceImage's format. When source image contains more than one aspect,
        // the format should be compatiable with the aspect of the source image's subresource
        Format m_destinationFormat; 
    };

    struct SingleDeviceCopyQueryToBufferDescriptor
    {
        SingleDeviceCopyQueryToBufferDescriptor() = default;

        const SingleDeviceQueryPool* m_sourceQueryPool = nullptr;
        QueryHandle m_firstQuery = QueryHandle(0);
        uint32_t m_queryCount = 0;
        const  SingleDeviceBuffer* m_destinationBuffer = nullptr;
        uint32_t m_destinationOffset = 0;
        uint32_t m_destinationStride = 0;
    };

    enum class CopyItemType : uint32_t
    {
        Buffer = 0,
        Image,
        BufferToImage,
        ImageToBuffer,
        QueryToBuffer,
        Invalid
    };

    struct SingleDeviceCopyItem
    {
        SingleDeviceCopyItem()
            : m_type{CopyItemType::Buffer}
            , m_buffer{}
        {}

        SingleDeviceCopyItem(const SingleDeviceCopyBufferDescriptor& descriptor)
            : m_type{CopyItemType::Buffer}
            , m_buffer{descriptor}
        {}

        SingleDeviceCopyItem(const SingleDeviceCopyImageDescriptor& descriptor)
            : m_type{CopyItemType::Image}
            , m_image{descriptor}
        {}

        SingleDeviceCopyItem(const SingleDeviceCopyBufferToImageDescriptor& descriptor)
            : m_type{CopyItemType::BufferToImage}
            , m_bufferToImage{descriptor}
        {}

        SingleDeviceCopyItem(const SingleDeviceCopyImageToBufferDescriptor& descriptor)
            : m_type{CopyItemType::ImageToBuffer}
            , m_imageToBuffer{descriptor}
        {}

        SingleDeviceCopyItem(const SingleDeviceCopyQueryToBufferDescriptor& descriptor)
            : m_type{CopyItemType::QueryToBuffer}
            , m_queryToBuffer{descriptor}
        {}

        CopyItemType m_type;
        union
        {
            SingleDeviceCopyBufferDescriptor m_buffer;
            SingleDeviceCopyImageDescriptor m_image;
            SingleDeviceCopyBufferToImageDescriptor m_bufferToImage;
            SingleDeviceCopyImageToBufferDescriptor m_imageToBuffer;
            SingleDeviceCopyQueryToBufferDescriptor m_queryToBuffer;
        };
    };
}
