/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <Atom/RHI.Reflect/AttachmentId.h>
#include <Atom/RHI.Reflect/AttachmentLoadStoreAction.h>
#include <Atom/RHI.Reflect/BufferScopeAttachmentDescriptor.h>
#include <Atom/RHI.Reflect/BufferViewDescriptor.h>
#include <Atom/RHI.Reflect/ClearValue.h>
#include <Atom/RHI.Reflect/ImageScopeAttachmentDescriptor.h>
#include <Atom/RHI.Reflect/ImageViewDescriptor.h>
#include <Atom/RHI.Reflect/ResolveScopeAttachmentDescriptor.h>
#include <Atom/RHI.Reflect/ScopeAttachmentDescriptor.h>

namespace AZ::RHI
{
    //! A unified descriptor of the binding of an attachment to a scope.
    //! Essentially a union of all possible ScopeAttachment types.
    struct UnifiedScopeAttachmentDescriptor final
        : public ScopeAttachmentDescriptor
    {
        UnifiedScopeAttachmentDescriptor() { };
        ~UnifiedScopeAttachmentDescriptor() { };

        /// Constructor for buffers, type will be set automatically
        explicit UnifiedScopeAttachmentDescriptor(
            const AttachmentId& attachmentId,
            const BufferViewDescriptor& bufferViewDescriptor,
            const AttachmentLoadStoreAction& loadStoreAction = AttachmentLoadStoreAction());

        /// Constructor for image, type will be set automatically
        explicit UnifiedScopeAttachmentDescriptor(
            const AttachmentId& attachmentId,
            const ImageViewDescriptor& imageViewDescriptor,
            const AttachmentLoadStoreAction& loadStoreAction = AttachmentLoadStoreAction());

        /// Constructor for resolve, type will be set automatically
        explicit UnifiedScopeAttachmentDescriptor(
            const AttachmentId& attachmentId,
            const AttachmentId& resolveAttachmentId,
            const ImageViewDescriptor& imageViewDescriptor = ImageViewDescriptor(),
            const AttachmentLoadStoreAction& loadStoreAction = AttachmentLoadStoreAction());

        BufferScopeAttachmentDescriptor GetAsBuffer() const;
        ImageScopeAttachmentDescriptor GetAsImage() const;
        ResolveScopeAttachmentDescriptor GetAsResolve() const;

        BufferViewDescriptor& GetBufferViewDescriptor();
        ImageViewDescriptor& GetImageViewDescriptor();

        void SetAsBuffer(const BufferViewDescriptor& desc);
        void SetAsImage(const ImageViewDescriptor& desc);
        void SetAsResolve(const ImageViewDescriptor& desc, AttachmentId resolveAttachmentId);

        AttachmentType GetType() const;

    private:

        /// Differentiates between an image, buffer and resolve attachment
        AttachmentType m_type = AttachmentType::Uninitialized;

        /// The attachment id associated with the attachment to be resolved.
        AttachmentId m_resolveAttachmentId;

        union
        {
            /// The buffer view associated with the binding.
            BufferViewDescriptor m_bufferViewDescriptor;

            /// The image view associated with the binding.
            ImageViewDescriptor m_imageViewDescriptor;
        };
    };
}
