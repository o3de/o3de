/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/AttachmentId.h>
#include <Atom/RHI.Reflect/ScopeId.h>

namespace AZ
{
    namespace RHI
    {
        class FrameGraphAttachmentDatabase;
        class Buffer;
        class BufferView;
        class Image;
        class ImageView;
        struct BufferDescriptor;
        struct ImageDescriptor;

        /**
         * FrameGraphCompileContext provides access to compiled image and buffer views
         * associated with the provided scope id, along with other query methods for
         * accessing attachment resource data. This information can be used to
         * compile ShaderResourceGroups.
         */
        class FrameGraphCompileContext
        {
        public:
            FrameGraphCompileContext() = default;

            FrameGraphCompileContext(
                const ScopeId& scopeId,
                const FrameGraphAttachmentDatabase& attachmentDatabase);

            /// Returns the scope id associated with this context.
            const ScopeId& GetScopeId() const;

            /// Returns whether the given attachment id is valid within the current frame.
            bool IsAttachmentValid(const AttachmentId& attachmentId) const;

            /// Returns the number of scope attachments used by the current scope for the given attachment
            const size_t GetScopeAttachmentCount(const AttachmentId& attachmentId) const;

            /// Returns the buffer view associated with usage on the current scope.
            const BufferView* GetBufferView(const AttachmentId& attachmentId, size_t index = 0) const;

            /// Returns the buffer associated with usage on the current scope.
            const Buffer* GetBuffer(const AttachmentId& attachmentId) const;

            /// Returns the image view associated with usage on the current scope.
            const ImageView* GetImageView(const AttachmentId& attachmentId, size_t index = 0) const;

            /// Returns the image associated with usage on the current scope.
            const Image* GetImage(const AttachmentId& attachmentId) const;

            /// Returns the buffer descriptor for the given attachment id.
            BufferDescriptor GetBufferDescriptor(const AttachmentId& attachmentId) const;

            /// Returns the image descriptor for the given attachment id.
            ImageDescriptor GetImageDescriptor(const AttachmentId& attachmentId) const;

        private:
            ScopeId m_scopeId;
            const FrameGraphAttachmentDatabase* m_attachmentDatabase = nullptr;
        };
    }
}
