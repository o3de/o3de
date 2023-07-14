/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/ImageViewDescriptor.h>
#include <Atom/RHI.Reflect/ResolveScopeAttachmentDescriptor.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ::RHI
{
    void ResolveScopeAttachmentDescriptor::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<ResolveScopeAttachmentDescriptor, ImageScopeAttachmentDescriptor>()
                ->Version(2)
                ->Field("ResolveAttachmentId", &ResolveScopeAttachmentDescriptor::m_resolveAttachmentId)
                ;
        }
    }

    ResolveScopeAttachmentDescriptor::ResolveScopeAttachmentDescriptor(
        const AttachmentId& attachmentId,
        const AttachmentId& resolveAttachmentId,
        const ImageViewDescriptor& imageViewDescriptor,
        const AttachmentLoadStoreAction& loadStoreAction)
        : ImageScopeAttachmentDescriptor(attachmentId, imageViewDescriptor, loadStoreAction)
        , m_resolveAttachmentId{ resolveAttachmentId }
    {}
}
