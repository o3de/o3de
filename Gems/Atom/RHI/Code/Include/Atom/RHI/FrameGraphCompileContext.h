/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/AttachmentId.h>
#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <Atom/RHI.Reflect/ScopeId.h>

namespace AZ
{
    namespace RHI
    {
        class FrameGraphAttachmentDatabase;
        class DeviceBuffer;
        class DeviceBufferView;
        class DeviceImage;
        class DeviceImageView;
        class ScopeAttachment;
        struct BufferDescriptor;
        struct ImageDescriptor;
        struct ImageViewDescriptor;

         //! FrameGraphCompileContext provides access to compiled image and buffer views
         //! associated with the provided scope id, along with other query methods for
         //! accessing attachment resource data. This information can be used to
         //! compile ShaderResourceGroups.
        class FrameGraphCompileContext
        {
        public:
            FrameGraphCompileContext() = default;

            FrameGraphCompileContext(
                const ScopeId& scopeId,
                const FrameGraphAttachmentDatabase& attachmentDatabase);

            //! Returns the scope id associated with this context.
            const ScopeId& GetScopeId() const;

            //! Returns whether the given attachment id is valid within the current frame.
            bool IsAttachmentValid(const AttachmentId& attachmentId) const;

            //! Returns the number of scope attachments used by the current scope for the given attachment
            const size_t GetScopeAttachmentCount(const AttachmentId& attachmentId) const;

            //! Returns the buffer view associated with the scope attachment.
            const DeviceBufferView* GetBufferView(const ScopeAttachment* scopeAttachment) const;

            //! Returns the buffer view associated with the attachmentId.
            const DeviceBufferView* GetBufferView(const AttachmentId& attachmentId) const;

            //! Returns the buffer view associated with attachmentId and the attachmentUsage on the current scope.
            const DeviceBufferView* GetBufferView(const AttachmentId& attachmentId, RHI::ScopeAttachmentUsage attachmentUsage) const;

            //! Returns the buffer associated with attachmentId.
            const DeviceBuffer* GetBuffer(const AttachmentId& attachmentId) const;

            //! Returns the image view associated with the scope attachment
            const DeviceImageView* GetImageView(const ScopeAttachment* scopeAttacment) const;

            //! Returns the image view associated with attachmentId, attachmentUsage and imageViewDescriptor on the current scope.
            const DeviceImageView* GetImageView(
                const AttachmentId& attachmentId,
                const ImageViewDescriptor& imageViewDescriptor,
                const RHI::ScopeAttachmentUsage attachmentUsage) const;

            //! Returns the image view associated with the attachmentId.
            const DeviceImageView* GetImageView(const AttachmentId& attachmentId) const;

            //! Returns the image associated with the attachmentId.
            const DeviceImage* GetImage(const AttachmentId& attachmentId) const;

            //! Returns the buffer descriptor for the given attachment id.
            BufferDescriptor GetBufferDescriptor(const AttachmentId& attachmentId) const;

            //! Returns the image descriptor for the given attachment id.
            ImageDescriptor GetImageDescriptor(const AttachmentId& attachmentId) const;

        private:
            ScopeId m_scopeId;
            const FrameGraphAttachmentDatabase* m_attachmentDatabase = nullptr;
        };
    }
}
