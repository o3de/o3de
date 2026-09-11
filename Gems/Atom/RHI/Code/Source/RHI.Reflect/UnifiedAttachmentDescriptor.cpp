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
    {
    }

    UnifiedAttachmentDescriptor::UnifiedAttachmentDescriptor(const BufferDescriptor& bufferDescriptor)
        : Base{ BufferAttachment{ bufferDescriptor } }
    {
    }

    UnifiedAttachmentDescriptor::UnifiedAttachmentDescriptor(const ImageDescriptor& imageDescriptor)
        : Base{ ImageAttachment{ imageDescriptor } }
    {
    }

    UnifiedAttachmentDescriptor::UnifiedAttachmentDescriptor(
        const BufferDescriptor& bufferDescriptor, const BufferViewDescriptor& bufferViewDescriptor)
        : Base{ BufferAttachment{ bufferDescriptor, bufferViewDescriptor } }
    {
    }

    UnifiedAttachmentDescriptor::UnifiedAttachmentDescriptor(
        const ImageDescriptor& imageDescriptor, const ImageViewDescriptor& imageViewDescriptor)
        : Base{ ImageAttachment{ imageDescriptor, imageViewDescriptor } }
    {
    }

    HashValue64 UnifiedAttachmentDescriptor::GetHash(HashValue64 seed /* = 0 */) const
    {
        return TypeHash64(*this, seed);
    }
} // namespace AZ::RHI
