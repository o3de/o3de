/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Utils/TypeHash.h>

#include <Atom/RHI.Reflect/UnifiedAttachmentDescriptor.h>

namespace AZ
{
    namespace RHI
    {
        UnifiedAttachmentDescriptor::UnifiedAttachmentDescriptor()
        { }

        UnifiedAttachmentDescriptor::UnifiedAttachmentDescriptor(const BufferDescriptor& bufferDescriptor)
            : m_buffer(bufferDescriptor)
            , m_type(AttachmentType::Buffer)
        { }

        UnifiedAttachmentDescriptor::UnifiedAttachmentDescriptor(const ImageDescriptor& imageDescriptor)
            : m_image(imageDescriptor)
            , m_type(AttachmentType::Image)
        { }

        HashValue64 UnifiedAttachmentDescriptor::GetHash(HashValue64 seed /* = 0 */) const
        {
            return TypeHash64(*this, seed);
        }
    }
}
