/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/MultiDeviceBuffer.h>
#include <Atom/RHI/MultiDeviceImage.h>
#include <Atom/RHI/MultiDeviceQueryPool.h>
#include <Atom/RHI/CopyItem.h>

namespace AZ
{
    namespace RHI
    {
        struct MultiDeviceCopyBufferDescriptor
        {
            MultiDeviceCopyBufferDescriptor() = default;

            CopyBufferDescriptor GetDeviceCopyBufferDescriptor(int deviceIndex) const
            {
                AZ_Assert(m_sourceBuffer, "Not initialized with source MultiDeviceBuffer\n");
                AZ_Assert(m_destinationBuffer, "Not initialized with destination MultiDeviceBuffer\n");

                return CopyBufferDescriptor{ m_sourceBuffer ? m_sourceBuffer->GetDeviceBuffer(deviceIndex).get() : nullptr,
                                                         m_sourceOffset,
                                                         m_destinationBuffer ? m_destinationBuffer->GetDeviceBuffer(deviceIndex).get()
                                                                             : nullptr,
                                                         m_destinationOffset,
                                                         m_size };
            }

            const MultiDeviceBuffer* m_sourceBuffer = nullptr;
            uint32_t m_sourceOffset = 0;
            const MultiDeviceBuffer* m_destinationBuffer = nullptr;
            uint32_t m_destinationOffset = 0;
            uint32_t m_size = 0;
        };

        struct MultiDeviceCopyImageDescriptor
        {
            MultiDeviceCopyImageDescriptor() = default;

            CopyImageDescriptor GetDeviceCopyImageDescriptor(int deviceIndex) const
            {
                AZ_Assert(m_sourceImage, "Not initialized with source MultiDeviceImage\n");
                AZ_Assert(m_destinationImage, "Not initialized with destination MultiDeviceImage\n");

                return CopyImageDescriptor{ m_sourceImage ? m_sourceImage->GetDeviceImage(deviceIndex).get() : nullptr,
                                                        m_sourceSubresource,
                                                        m_sourceOrigin,
                                                        m_sourceSize,
                                                        m_destinationImage ? m_destinationImage->GetDeviceImage(deviceIndex).get()
                                                                           : nullptr,
                                                        m_destinationSubresource,
                                                        m_destinationOrigin };
            }

            const MultiDeviceImage* m_sourceImage = nullptr;
            ImageSubresource m_sourceSubresource;
            Origin m_sourceOrigin;
            Size m_sourceSize;
            const MultiDeviceImage* m_destinationImage = nullptr;
            ImageSubresource m_destinationSubresource;
            Origin m_destinationOrigin;
        };

        struct MultiDeviceCopyBufferToImageDescriptor
        {
            MultiDeviceCopyBufferToImageDescriptor() = default;

            CopyBufferToImageDescriptor GetDeviceCopyBufferToImageDescriptor(int deviceIndex) const
            {
                AZ_Assert(m_sourceBuffer, "Not initialized with source MultiDeviceBuffer\n");
                AZ_Assert(m_destinationImage, "Not initialized with destination MultiDeviceImage\n");

                return CopyBufferToImageDescriptor{
                    m_sourceBuffer ? m_sourceBuffer->GetDeviceBuffer(deviceIndex).get() : nullptr,
                    m_sourceOffset,
                    m_sourceBytesPerRow,
                    m_sourceBytesPerImage,
                    m_sourceSize,
                    m_destinationImage ? m_destinationImage->GetDeviceImage(deviceIndex).get() : nullptr,
                    m_destinationSubresource,
                    m_destinationOrigin
                };
            }

            const MultiDeviceBuffer* m_sourceBuffer = nullptr;
            uint32_t m_sourceOffset = 0;
            uint32_t m_sourceBytesPerRow = 0;
            uint32_t m_sourceBytesPerImage = 0;
            Size m_sourceSize;
            const MultiDeviceImage* m_destinationImage = nullptr;
            ImageSubresource m_destinationSubresource;
            Origin m_destinationOrigin;
        };

        struct MultiDeviceCopyImageToBufferDescriptor
        {
            MultiDeviceCopyImageToBufferDescriptor() = default;

            CopyImageToBufferDescriptor GetDeviceCopyImageToBufferDescriptor(int deviceIndex) const
            {
                AZ_Assert(m_sourceImage, "Not initialized with source MultiDeviceImage\n");
                AZ_Assert(m_destinationBuffer, "Not initialized with destination MultiDeviceBuffer\n");

                return CopyImageToBufferDescriptor{ m_sourceImage ? m_sourceImage->GetDeviceImage(deviceIndex).get() : nullptr,
                                                                m_sourceSubresource,
                                                                m_sourceOrigin,
                                                                m_sourceSize,
                                                                m_destinationBuffer
                                                                    ? m_destinationBuffer->GetDeviceBuffer(deviceIndex).get()
                                                                    : nullptr,
                                                                m_destinationOffset,
                                                                m_destinationBytesPerRow,
                                                                m_destinationBytesPerImage,
                                                                m_destinationFormat };
            }

            const MultiDeviceImage* m_sourceImage = nullptr;
            ImageSubresource m_sourceSubresource;
            Origin m_sourceOrigin;
            Size m_sourceSize;
            const MultiDeviceBuffer* m_destinationBuffer = nullptr;
            uint32_t m_destinationOffset = 0;
            uint32_t m_destinationBytesPerRow = 0;
            uint32_t m_destinationBytesPerImage = 0;
            // The destination format is usually same as m_sourceImage's format. When source image contains more than one aspect,
            // the format should be compatiable with the aspect of the source image's subresource
            Format m_destinationFormat;
        };

        struct MultiDeviceCopyQueryToBufferDescriptor
        {
            MultiDeviceCopyQueryToBufferDescriptor() = default;

            CopyQueryToBufferDescriptor GetDeviceCopyQueryToBufferDescriptor(int deviceIndex) const
            {
                AZ_Assert(m_sourceQueryPool, "Not initialized with source MultiDeviceQueryPool\n");
                AZ_Assert(m_destinationBuffer, "Not initialized with destination MultiDeviceBuffer\n");

                return CopyQueryToBufferDescriptor{
                    m_sourceQueryPool ? m_sourceQueryPool->GetDeviceQueryPool(deviceIndex).get() : nullptr,
                    m_firstQuery,
                    m_queryCount,
                    m_destinationBuffer ? m_destinationBuffer->GetDeviceBuffer(deviceIndex).get() : nullptr,
                    m_destinationOffset,
                    m_destinationStride
                };
            }

            const MultiDeviceQueryPool* m_sourceQueryPool = nullptr;
            QueryHandle m_firstQuery = QueryHandle(0);
            uint32_t m_queryCount = 0;
            const MultiDeviceBuffer* m_destinationBuffer = nullptr;
            uint32_t m_destinationOffset = 0;
            uint32_t m_destinationStride = 0;
        };

        struct MultiDeviceCopyItem
        {
            MultiDeviceCopyItem()
                : m_type{ CopyItemType::Buffer }
                , m_buffer{}
            {
            }

            MultiDeviceCopyItem(const MultiDeviceCopyBufferDescriptor& descriptor)
                : m_type{ CopyItemType::Buffer }
                , m_buffer{ descriptor }
            {
            }

            MultiDeviceCopyItem(const MultiDeviceCopyImageDescriptor& descriptor)
                : m_type{ CopyItemType::Image }
                , m_image{ descriptor }
            {
            }

            MultiDeviceCopyItem(const MultiDeviceCopyBufferToImageDescriptor& descriptor)
                : m_type{ CopyItemType::BufferToImage }
                , m_bufferToImage{ descriptor }
            {
            }

            MultiDeviceCopyItem(const MultiDeviceCopyImageToBufferDescriptor& descriptor)
                : m_type{ CopyItemType::ImageToBuffer }
                , m_imageToBuffer{ descriptor }
            {
            }

            MultiDeviceCopyItem(const MultiDeviceCopyQueryToBufferDescriptor& descriptor)
                : m_type{ CopyItemType::QueryToBuffer }
                , m_queryToBuffer{ descriptor }
            {
            }

            CopyItem GetDeviceCopyItem(int deviceIndex) const
            {
                switch (m_type)
                {
                case CopyItemType::Buffer:
                    return CopyItem(m_buffer.GetDeviceCopyBufferDescriptor(deviceIndex));
                case CopyItemType::Image:
                    return CopyItem(m_image.GetDeviceCopyImageDescriptor(deviceIndex));
                case CopyItemType::BufferToImage:
                    return CopyItem(m_bufferToImage.GetDeviceCopyBufferToImageDescriptor(deviceIndex));
                case CopyItemType::ImageToBuffer:
                    return CopyItem(m_imageToBuffer.GetDeviceCopyImageToBufferDescriptor(deviceIndex));
                case CopyItemType::QueryToBuffer:
                    return CopyItem(m_queryToBuffer.GetDeviceCopyQueryToBufferDescriptor(deviceIndex));
                default:
                    return CopyItem();
                }
            }

            CopyItemType m_type;
            union {
                MultiDeviceCopyBufferDescriptor m_buffer;
                MultiDeviceCopyImageDescriptor m_image;
                MultiDeviceCopyBufferToImageDescriptor m_bufferToImage;
                MultiDeviceCopyImageToBufferDescriptor m_imageToBuffer;
                MultiDeviceCopyQueryToBufferDescriptor m_queryToBuffer;
            };
        };
    } // namespace RHI
} // namespace AZ
