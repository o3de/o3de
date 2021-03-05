/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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