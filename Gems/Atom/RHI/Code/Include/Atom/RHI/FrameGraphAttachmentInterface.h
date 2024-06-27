/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/FrameGraphAttachmentDatabase.h>
#include <Atom/RHI/SwapChain.h>
#include <Atom/RHI/Buffer.h>

#include <Atom/RHI.Reflect/TransientImageDescriptor.h>
#include <Atom/RHI.Reflect/TransientBufferDescriptor.h>
#include <Atom/RHI.Reflect/ScopeId.h>

namespace AZ::RHI
{
    class Image;
    class Buffer;

    //! This interface exposes FrameGraphAttachmentDatabase functionality to non-RHI systems (like the RPI). This is in order
    //! to reduce access to certain public functions in FrameGraphAttachmentDatabase that are intended for RHI use only.
    //!
    //! Attachment registration for a particular AttachmentId occurs just once per frame. A registration
    //! event makes the attachment immediately visible via the AttachmentId. Any "Use" operation after this call,
    //! either on this scope or a downstream scope, may reference that attachment by AttachmentId.
    //! 
    //! Attachments fall into two categories:
    //! 
    //!   (Imports):
    //!      Persistent attachments owned by the user are imported into the frame scheduler each
    //!      frame. The frame scheduler merely references the attachment; it does not dictate ownership.
    //! 
    //!   (Transients):
    //!      Transient Attachments are owned and managed by the frame scheduler. They persist only for the current frame.
    //!      The user references the transient attachment by AttachmentId, and is able to access resource contents
    //!      in the Compile and Execute phases of a ScopeProducer (via the respective phase contexts).
    class FrameGraphAttachmentInterface
    {
    public:
        FrameGraphAttachmentInterface(FrameGraphAttachmentDatabase& attachmentDatabase)
            : m_attachmentDatabase(attachmentDatabase)
        { }
        ~FrameGraphAttachmentInterface() = default;

        //! Imports a persistent image as an attachment.
        //! \param imageAttachment The image attachment to import.
        ResultCode ImportImage(const AttachmentId& attachmentId, Ptr<Image> image)
        {
            return m_attachmentDatabase.ImportImage(attachmentId, image);
        }

        //! Imports a swap chain image as an attachment.
        //! \param swapChainAttachment The swap chain attachment to import.
        ResultCode ImportSwapChain(const AttachmentId& attachmentId, Ptr<SwapChain> swapChain)
        {
            return m_attachmentDatabase.ImportSwapChain(attachmentId, swapChain);
        }

        //! Imports a persistent buffer as an attachment.
        //! \param bufferAttachment The buffer attachment to import.
        ResultCode ImportBuffer(const AttachmentId& attachmentId, Ptr<Buffer> buffer)
        {
            return m_attachmentDatabase.ImportBuffer(attachmentId, buffer);
        }

        //! Creates a transient image as an attachment. The provided attachment id is associated with the new attachment.
        ResultCode CreateTransientImage(const TransientImageDescriptor& descriptor)
        {
            return m_attachmentDatabase.CreateTransientImage(descriptor);
        }

        //! Creates a transient buffer as an attachment. The provided attachment id is associated with the new attachment.
        ResultCode CreateTransientBuffer(const TransientBufferDescriptor& descriptor)
        {
            return m_attachmentDatabase.CreateTransientBuffer(descriptor);
        }

        //! Returns whether the attachment id was registered via a call to Create / Import.
        bool IsAttachmentValid(const AttachmentId& attachmentId) const
        {
            return m_attachmentDatabase.IsAttachmentValid(attachmentId);
        }

        //! Returns the FrameAttachment for a given AttachmentId, or nullptr if not found.
        const FrameAttachment* FindAttachment(const AttachmentId& attachmentId) const
        {
            return m_attachmentDatabase.FindAttachment(attachmentId);
        }

        //! Resolves an attachment id to a buffer descriptor. This is useful when accessing buffer information for
        //! an attachment that was declared in a different scope.
        //! \param attachmentId The attachment id used to lookup the descriptors.
        ImageDescriptor GetImageDescriptor(const AttachmentId& attachmentId)
        {
            return m_attachmentDatabase.GetImageDescriptor(attachmentId);
        }

        //! Resolves an attachment id to an image descriptor. This is useful when accessing image information for
        //! an attachment that was declared in a different scope.
        //! \param attachmentId The attachment id used to lookup the descriptors.
        BufferDescriptor GetBufferDescriptor(const AttachmentId& attachmentId) const
        {
            return m_attachmentDatabase.GetBufferDescriptor(attachmentId);
        }

    private:

        //! Reference to the underlying attachment database. All functions calls are forwarded to this member.
        FrameGraphAttachmentDatabase& m_attachmentDatabase;
    };
}
