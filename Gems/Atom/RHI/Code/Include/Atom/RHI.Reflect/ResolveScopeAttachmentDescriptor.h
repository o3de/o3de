/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/ImageScopeAttachmentDescriptor.h>

namespace AZ::RHI
{
    //! Describes the binding of a resolve image attachment to a scope.
    struct ResolveScopeAttachmentDescriptor
        : public ImageScopeAttachmentDescriptor
    {
        AZ_TYPE_INFO(ResolveScopeAttachmentDescriptor, "{8ADC8E2B-2221-487B-A13F-E14218292E39}");

        static void Reflect(AZ::ReflectContext* context);

        ResolveScopeAttachmentDescriptor() = default;

        explicit ResolveScopeAttachmentDescriptor(
            const AttachmentId& attachmentId,
            const AttachmentId& resolveAttachmentId,
            const ImageViewDescriptor& imageViewDescriptor = ImageViewDescriptor(),
            const AttachmentLoadStoreAction& loadStoreAction = AttachmentLoadStoreAction());

        /// The attachment id associated with the attachment to be resolved.
        AttachmentId m_resolveAttachmentId;
    };
}
