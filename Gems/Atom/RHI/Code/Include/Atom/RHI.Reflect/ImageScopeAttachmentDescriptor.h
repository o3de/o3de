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
#include <Atom/RHI.Reflect/ScopeId.h> // GALIB

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

        //! GALIB
        //! This is an optional parameter that becomes useful to connect the current Scope
        //! to a previous scope so the current scope becomes a Subpass. Typically when
        //! frameGraph.UseSubpassInputAttachment(descriptor); is called, then the Subpass connection
        //! is made automatically. But for cases like
        //! frameGraph.UseColorAttachment(descriptor); it is not clear that there is a an opportunity of making
        //! the current scope a subpass of the previous scope and then this parameter comes to the rescue
        //! to highlight an explicit connection.
        ScopeId m_subpassScopeId;
    };
}
