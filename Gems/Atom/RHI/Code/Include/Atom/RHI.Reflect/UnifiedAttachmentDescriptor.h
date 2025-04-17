/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <Atom/RHI.Reflect/BufferDescriptor.h>
#include <Atom/RHI.Reflect/BufferViewDescriptor.h>
#include <Atom/RHI.Reflect/ImageDescriptor.h>
#include <Atom/RHI.Reflect/ImageViewDescriptor.h>

namespace AZ::RHI
{
    //! A unified attachment descriptor used to store either an image or a buffer descriptor.
    //! Used primarily to simplify pass attachment logic while supporting both attachment types.
    struct UnifiedAttachmentDescriptor
    {
        UnifiedAttachmentDescriptor();
        UnifiedAttachmentDescriptor(const BufferDescriptor& bufferDescriptor);
        UnifiedAttachmentDescriptor(const ImageDescriptor& imageDescriptor);
        UnifiedAttachmentDescriptor(const BufferDescriptor& bufferDescriptor, const BufferViewDescriptor& bufferViewDescriptor);
        UnifiedAttachmentDescriptor(const ImageDescriptor& imageDescriptor, const ImageViewDescriptor& imageViewDescriptor);

        HashValue64 GetHash(HashValue64 seed = HashValue64{ 0 }) const;

        /// Differentiates between an image, buffer and resolve attachment
        AttachmentType m_type = AttachmentType::Uninitialized;

        union {
            struct
            {
                BufferDescriptor m_buffer;
                BufferViewDescriptor m_bufferView;
            };
            struct
            {
                ImageDescriptor m_image;
                ImageViewDescriptor m_imageView;
            };
        };
    };
} // namespace AZ::RHI
