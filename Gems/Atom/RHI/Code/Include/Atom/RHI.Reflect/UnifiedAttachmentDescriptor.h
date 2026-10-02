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
    struct ATOM_RHI_REFLECT_API UnifiedAttachmentDescriptor
    {
        UnifiedAttachmentDescriptor();
        UnifiedAttachmentDescriptor(const BufferDescriptor& bufferDescriptor);
        UnifiedAttachmentDescriptor(const ImageDescriptor& imageDescriptor);
        UnifiedAttachmentDescriptor(const BufferDescriptor& bufferDescriptor, const BufferViewDescriptor& bufferViewDescriptor);
        UnifiedAttachmentDescriptor(const ImageDescriptor& imageDescriptor, const ImageViewDescriptor& imageViewDescriptor);

        HashValue64 GetHash(HashValue64 seed = HashValue64{ 0 }) const;

        /// Differentiates between an image, buffer and resolve attachment
        AttachmentType m_type = AttachmentType::Uninitialized;

        union Storage {
            struct BufferAttachment
            {
                BufferDescriptor m_buffer;
                BufferViewDescriptor m_bufferView;
            } buffer;
            struct ImageAttachment
            {
                ImageDescriptor m_image;
                ImageViewDescriptor m_imageView;
            } image;
            Storage() : buffer{} {}
            Storage(const BufferDescriptor& bufferDescriptor)
                : buffer{ bufferDescriptor } {}
            Storage(const BufferDescriptor& bufferDescriptor, const BufferViewDescriptor& bufferViewDescriptor)
                : buffer{ bufferDescriptor, bufferViewDescriptor } {}
            Storage(const ImageDescriptor& imageDescriptor)
                : image{ imageDescriptor } {}
            Storage(const ImageDescriptor& imageDescriptor, const ImageViewDescriptor& imageViewDescriptor)
                : image{ imageDescriptor, imageViewDescriptor } {}
        } m_storage{};

        // The following parts of the interface shall be removed once an
        // API breaking release is coming up:
        BufferDescriptor& m_buffer{ m_storage.buffer.m_buffer };
        BufferViewDescriptor& m_bufferView{ m_storage.buffer.m_bufferView };
        ImageDescriptor& m_image{ m_storage.image.m_image };
        ImageViewDescriptor& m_imageView{ m_storage.image.m_imageView };

        UnifiedAttachmentDescriptor& operator=(const BufferDescriptor& bufferDescriptor);
        UnifiedAttachmentDescriptor& operator=(const ImageDescriptor& imageDescriptor);
        UnifiedAttachmentDescriptor& operator=(const UnifiedAttachmentDescriptor& other);
    };
} // namespace AZ::RHI
