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

        /// The image view associated with the binding.
        ImageViewDescriptor m_imageViewDescriptor;

        //! Index of the Subpass that owns this scope attachment.
        //! When this index is greater than 0, the frameGraph will make a "SameGroup"
        //! connection between the previous and the current subpass.
        //! This will guarantee that the Topological Sort of the framegraph
        //! will group consecutive scopes as Subpasses.
        uint32_t m_subpassIndex = 0;
    };
}
