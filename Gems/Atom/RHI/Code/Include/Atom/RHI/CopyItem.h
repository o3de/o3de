/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/Buffer.h>
#include <Atom/RHI/DeviceCopyItem.h>
#include <Atom/RHI/Image.h>
#include <Atom/RHI/QueryPool.h>

namespace AZ
{
    namespace RHI
    {
        struct CopyBufferDescriptor
        {
            CopyBufferDescriptor() = default;

            DeviceCopyBufferDescriptor GetDeviceCopyBufferDescriptor(int deviceIndex) const
            {
                return DeviceCopyBufferDescriptor{ m_sourceBuffer->GetDeviceBuffer(deviceIndex).get(),
                                                   m_sourceOffset,
                                                   m_destinationBuffer->GetDeviceBuffer(deviceIndex).get(),
                                                   m_destinationOffset,
                                                   m_size };
            }

            const Buffer* m_sourceBuffer = nullptr;
            uint32_t m_sourceOffset = 0;
            const Buffer* m_destinationBuffer = nullptr;
            uint32_t m_destinationOffset = 0;
            uint32_t m_size = 0;
        };

        struct CopyImageDescriptor
        {
            CopyImageDescriptor() = default;

            DeviceCopyImageDescriptor GetDeviceCopyImageDescriptor(int deviceIndex) const
            {
                return DeviceCopyImageDescriptor{
                    m_sourceImage->GetDeviceImage(deviceIndex).get(),      m_sourceSubresource,      m_sourceOrigin,     m_sourceSize,
                    m_destinationImage->GetDeviceImage(deviceIndex).get(), m_destinationSubresource, m_destinationOrigin
                };
            }

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

            DeviceCopyBufferToImageDescriptor GetDeviceCopyBufferToImageDescriptor(int deviceIndex) const
            {
                return DeviceCopyBufferToImageDescriptor{ m_sourceBuffer->GetDeviceBuffer(deviceIndex).get(),
                                                          m_sourceOffset,
                                                          m_sourceBytesPerRow,
                                                          m_sourceBytesPerImage,
                                                          m_sourceSize,
                                                          m_destinationImage->GetDeviceImage(deviceIndex).get(),
                                                          m_destinationSubresource,
                                                          m_destinationOrigin };
            }

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

            DeviceCopyImageToBufferDescriptor GetDeviceCopyImageToBufferDescriptor(int deviceIndex) const
            {
                return DeviceCopyImageToBufferDescriptor{ m_sourceImage->GetDeviceImage(deviceIndex).get(),
                                                          m_sourceSubresource,
                                                          m_sourceOrigin,
                                                          m_sourceSize,
                                                          m_destinationBuffer->GetDeviceBuffer(deviceIndex).get(),
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
            // The destination format is usually same as m_sourceImage's format. When source image contains more than one aspect,
            // the format should be compatiable with the aspect of the source image's subresource
            Format m_destinationFormat; 
        };

        struct CopyQueryToBufferDescriptor
        {
            CopyQueryToBufferDescriptor() = default;

            DeviceCopyQueryToBufferDescriptor GetDeviceCopyQueryToBufferDescriptor(int deviceIndex) const
            {
                return DeviceCopyQueryToBufferDescriptor{
                    m_sourceQueryPool->GetDeviceQueryPool(deviceIndex).get(), m_firstQuery,        m_queryCount,
                    m_destinationBuffer->GetDeviceBuffer(deviceIndex).get(),  m_destinationOffset, m_destinationStride
                };
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
            {}

            CopyItem(const CopyBufferDescriptor& descriptor)
                : m_type{ CopyItemType::Buffer }
                , m_buffer{ descriptor }
            {}

            CopyItem(const CopyImageDescriptor& descriptor)
                : m_type{ CopyItemType::Image }
                , m_image{ descriptor }
            {}

            CopyItem(const CopyBufferToImageDescriptor& descriptor)
                : m_type{ CopyItemType::BufferToImage }
                , m_bufferToImage{ descriptor }
            {}

            CopyItem(const CopyImageToBufferDescriptor& descriptor)
                : m_type{ CopyItemType::ImageToBuffer }
                , m_imageToBuffer{ descriptor }
            {}

            CopyItem(const CopyQueryToBufferDescriptor& descriptor)
                : m_type{ CopyItemType::QueryToBuffer }
                , m_queryToBuffer{ descriptor }
            {
            }

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
        };
    }
}
