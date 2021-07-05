/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/Image.h>
#include <Atom/RHI/Buffer.h>
#include <Atom/RHI/QueryPool.h>

namespace AZ
{
    namespace RHI
    {
        struct CopyBufferDescriptor
        {
            CopyBufferDescriptor() = default;

            const Buffer* m_sourceBuffer = nullptr;
            uint32_t m_sourceOffset = 0;
            const Buffer* m_destinationBuffer = nullptr;
            uint32_t m_destinationOffset = 0;
            uint32_t m_size = 0;
        };

        struct CopyImageDescriptor
        {
            CopyImageDescriptor() = default;

            const Image* m_sourceImage = nullptr;
            ImageSubresource m_sourceSubresource;
            Origin m_sourceOrigin;
            Size m_sourceSize;
            const Image* m_destinationImage = nullptr;
            ImageSubresource m_destinationSubresource;
            Origin m_destinationOrigin;
        };

        struct CopyBufferToImageDescriptor
        {
            CopyBufferToImageDescriptor() = default;

            const Buffer* m_sourceBuffer = nullptr;
            uint32_t m_sourceOffset = 0;
            uint32_t m_sourceBytesPerRow = 0;
            uint32_t m_sourceBytesPerImage = 0;
            Size m_sourceSize;
            const Image* m_destinationImage = nullptr;
            ImageSubresource m_destinationSubresource;
            Origin m_destinationOrigin;
        };

        struct CopyImageToBufferDescriptor
        {
            CopyImageToBufferDescriptor() = default;

            const Image* m_sourceImage = nullptr;
            ImageSubresource m_sourceSubresource;
            Origin m_sourceOrigin;
            Size m_sourceSize;
            const  Buffer* m_destinationBuffer = nullptr;
            uint32_t m_destinationOffset = 0;
            uint32_t m_destinationBytesPerRow = 0;
            uint32_t m_destinationBytesPerImage = 0;
            // The destination format is usually same as m_sourceImage's format. When source image contains more than one aspect,
            // the format should be compatiable with the aspect of the source image's subresource
            Format m_destinationFormat; 
        };

        struct CopyQueryToBufferDescriptor
        {
            CopyQueryToBufferDescriptor() = default;

            const QueryPool* m_sourceQueryPool = nullptr;
            QueryHandle m_firstQuery = QueryHandle(0);
            uint32_t m_queryCount = 0;
            const  Buffer* m_destinationBuffer = nullptr;
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

        struct CopyItem
        {
            CopyItem()
                : m_type{CopyItemType::Buffer}
                , m_buffer{}
            {}

            CopyItem(const CopyBufferDescriptor& descriptor)
                : m_type{CopyItemType::Buffer}
                , m_buffer{descriptor}
            {}

            CopyItem(const CopyImageDescriptor& descriptor)
                : m_type{CopyItemType::Image}
                , m_image{descriptor}
            {}

            CopyItem(const CopyBufferToImageDescriptor& descriptor)
                : m_type{CopyItemType::BufferToImage}
                , m_bufferToImage{descriptor}
            {}

            CopyItem(const CopyImageToBufferDescriptor& descriptor)
                : m_type{CopyItemType::ImageToBuffer}
                , m_imageToBuffer{descriptor}
            {}

            CopyItem(const CopyQueryToBufferDescriptor& descriptor)
                : m_type{CopyItemType::QueryToBuffer}
                , m_queryToBuffer{descriptor}
            {}

            CopyItemType m_type;
            union
            {
                CopyBufferDescriptor m_buffer;
                CopyImageDescriptor m_image;
                CopyBufferToImageDescriptor m_bufferToImage;
                CopyImageToBufferDescriptor m_imageToBuffer;
                CopyQueryToBufferDescriptor m_queryToBuffer;
            };
        };
    }
}
