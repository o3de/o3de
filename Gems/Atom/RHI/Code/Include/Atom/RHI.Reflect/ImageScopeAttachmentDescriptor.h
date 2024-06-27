/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/ImageViewDescriptor.h>
#include <Atom/RHI.Reflect/ScopeAttachmentDescriptor.h>

namespace AZ::RHI
{
    //! Describes the binding of an image attachment to a scope.
    struct ImageScopeAttachmentDescriptor
        : public ScopeAttachmentDescriptor
    {
        AZ_TYPE_INFO(ImageScopeAttachmentDescriptor, "{66523EB6-9D3A-4633-A708-ADD57FDD5CE2}");

        static void Reflect(AZ::ReflectContext* context);

        ImageScopeAttachmentDescriptor() = default;

        explicit ImageScopeAttachmentDescriptor(
            const AttachmentId& attachmentId,
            const ImageViewDescriptor& imageViewDescriptor = ImageViewDescriptor(),
            const AttachmentLoadStoreAction& imageScopeLoadStoreAction = AttachmentLoadStoreAction());

        /// Returns the descriptor for the ImageView
        const ImageViewDescriptor& GetViewDescriptor() const;

        /// The image view associated with the binding.
        ImageViewDescriptor m_imageViewDescriptor;
    };
}
