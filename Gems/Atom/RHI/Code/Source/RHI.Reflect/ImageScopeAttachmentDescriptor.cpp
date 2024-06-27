/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/ImageScopeAttachmentDescriptor.h>

#include <AzCore/Serialization/SerializeContext.h>

namespace AZ::RHI
{
    void ImageScopeAttachmentDescriptor::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<ImageScopeAttachmentDescriptor, ScopeAttachmentDescriptor>()
                ->Version(3)
                ->Field("ImageViewDescriptor", &ImageScopeAttachmentDescriptor::m_imageViewDescriptor)
                ;
        }
    }

    ImageScopeAttachmentDescriptor::ImageScopeAttachmentDescriptor(
        const AttachmentId& attachmentId,
        const ImageViewDescriptor& imageViewDescriptor,
        const AttachmentLoadStoreAction& imageScopeLoadStoreAction)
        : ScopeAttachmentDescriptor(attachmentId, imageScopeLoadStoreAction)
        , m_imageViewDescriptor{ imageViewDescriptor }
    {}

    const ImageViewDescriptor& ImageScopeAttachmentDescriptor::GetViewDescriptor() const
    {
        return m_imageViewDescriptor;
    }
}
