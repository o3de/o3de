/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceImage.h>
#include <Atom/RHI/DeviceBuffer.h>
#include <Atom/RHI/DeviceQueryPool.h>

namespace AZ::RHI
{
    struct DeviceCopyBufferDescriptor
    {
        DeviceCopyBufferDescriptor() = default;

        const DeviceBuffer* m_sourceBuffer = nullptr;
        uint32_t m_sourceOffset = 0;
        const DeviceBuffer* m_destinationBuffer = nullptr;
        uint32_t m_destinationOffset = 0;
        uint32_t m_size = 0;
    };

    struct DeviceCopyImageDescriptor
    {
        DeviceCopyImageDescriptor() = default;

        const DeviceImage* m_sourceImage = nullptr;
        ImageSubresource m_sourceSubresource;
        Origin m_sourceOrigin;
        Size m_sourceSize;
        const DeviceImage* m_destinationImage = nullptr;
        ImageSubresource m_destinationSubresource;
        Origin m_destinationOrigin;
    };

    struct DeviceCopyBufferToImageDescriptor
    {
        DeviceCopyBufferToImageDescriptor() = default;

        const DeviceBuffer* m_sourceBuffer = nullptr;
        uint32_t m_sourceOffset = 0;
        uint32_t m_sourceBytesPerRow = 0;
        uint32_t m_sourceBytesPerImage = 0;
        // The source format is usually same as m_destinationImage's format. When destination image contains more than one aspect,
        // the format should be compatiable with the aspect of the destination image's subresource
        Format m_sourceFormat = Format::Unknown;
        Size m_sourceSize;
        const DeviceImage* m_destinationImage = nullptr;
        ImageSubresource m_destinationSubresource;
        Origin m_destinationOrigin;
    };

    struct DeviceCopyImageToBufferDescriptor
    {
        DeviceCopyImageToBufferDescriptor() = default;

        const DeviceImage* m_sourceImage = nullptr;
        ImageSubresource m_sourceSubresource;
        Origin m_sourceOrigin;
        Size m_sourceSize;
        const  DeviceBuffer* m_destinationBuffer = nullptr;
        uint32_t m_destinationOffset = 0;
        uint32_t m_destinationBytesPerRow = 0;
        uint32_t m_destinationBytesPerImage = 0;
        // The destination format is usually same as m_sourceImage's format. When source image contains more than one aspect,
        // the format should be compatiable with the aspect of the source image's subresource
        Format m_destinationFormat = Format::Unknown;
    };

    struct DeviceCopyQueryToBufferDescriptor
    {
        DeviceCopyQueryToBufferDescriptor() = default;

        const DeviceQueryPool* m_sourceQueryPool = nullptr;
        QueryHandle m_firstQuery = QueryHandle(0);
        uint32_t m_queryCount = 0;
        const  DeviceBuffer* m_destinationBuffer = nullptr;
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

    struct DeviceCopyItem
    {
        DeviceCopyItem()
            : m_type{CopyItemType::Buffer}
            , m_buffer{}
        {}

        DeviceCopyItem(const DeviceCopyBufferDescriptor& descriptor)
            : m_type{CopyItemType::Buffer}
            , m_buffer{descriptor}
        {}

        DeviceCopyItem(const DeviceCopyImageDescriptor& descriptor)
            : m_type{CopyItemType::Image}
            , m_image{descriptor}
        {}

        DeviceCopyItem(const DeviceCopyBufferToImageDescriptor& descriptor)
            : m_type{CopyItemType::BufferToImage}
            , m_bufferToImage{descriptor}
        {}

        DeviceCopyItem(const DeviceCopyImageToBufferDescriptor& descriptor)
            : m_type{CopyItemType::ImageToBuffer}
            , m_imageToBuffer{descriptor}
        {}

        DeviceCopyItem(const DeviceCopyQueryToBufferDescriptor& descriptor)
            : m_type{CopyItemType::QueryToBuffer}
            , m_queryToBuffer{descriptor}
        {}

        CopyItemType m_type;
        union
        {
            DeviceCopyBufferDescriptor m_buffer;
            DeviceCopyImageDescriptor m_image;
            DeviceCopyBufferToImageDescriptor m_bufferToImage;
            DeviceCopyImageToBufferDescriptor m_imageToBuffer;
            DeviceCopyQueryToBufferDescriptor m_queryToBuffer;
        };
    };
}
