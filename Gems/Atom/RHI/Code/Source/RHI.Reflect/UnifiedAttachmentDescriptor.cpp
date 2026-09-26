/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Utils/TypeHash.h>

#include <Atom/RHI.Reflect/UnifiedAttachmentDescriptor.h>

namespace AZ::RHI
{
    UnifiedAttachmentDescriptor::UnifiedAttachmentDescriptor()
        : m_storage{}
    {
    }

    UnifiedAttachmentDescriptor::UnifiedAttachmentDescriptor(const BufferDescriptor& bufferDescriptor)
        : m_type{ AttachmentType::Buffer }
        , m_storage{ bufferDescriptor }
    {
    }

    UnifiedAttachmentDescriptor::UnifiedAttachmentDescriptor(const ImageDescriptor& imageDescriptor)
        : m_type{ AttachmentType::Image }
        , m_storage{ imageDescriptor }
    {
    }

    UnifiedAttachmentDescriptor::UnifiedAttachmentDescriptor(
        const BufferDescriptor& bufferDescriptor, const BufferViewDescriptor& bufferViewDescriptor)
        : m_type{ AttachmentType::Buffer }
        , m_storage{ bufferDescriptor, bufferViewDescriptor }
    {
    }

    UnifiedAttachmentDescriptor::UnifiedAttachmentDescriptor(
        const ImageDescriptor& imageDescriptor, const ImageViewDescriptor& imageViewDescriptor)
        : m_type{ AttachmentType::Image }
        , m_storage{ imageDescriptor, imageViewDescriptor }
    {
    }

    HashValue64 UnifiedAttachmentDescriptor::GetHash(HashValue64 seed /* = 0 */) const
    {
        return TypeHash64(*this, seed);
    }

    UnifiedAttachmentDescriptor& UnifiedAttachmentDescriptor::operator=(const BufferDescriptor& bufferDescriptor)
    {
        m_type = AttachmentType::Buffer;
        m_buffer = bufferDescriptor;
        return *this;
    }

    UnifiedAttachmentDescriptor& UnifiedAttachmentDescriptor::operator=(const ImageDescriptor& imageDescriptor)
    {
        m_type = AttachmentType::Image;
        m_image = imageDescriptor;
        return *this;
    }

    UnifiedAttachmentDescriptor& UnifiedAttachmentDescriptor::operator=(const UnifiedAttachmentDescriptor& other)
    {
        m_type = other.m_type;
        if(m_type == AttachmentType::Image)
            m_storage.image = other.m_storage.image;
        else if(m_type == AttachmentType::Buffer)
            m_storage.buffer = other.m_storage.buffer;
        return *this;
    }
} // namespace AZ::RHI
