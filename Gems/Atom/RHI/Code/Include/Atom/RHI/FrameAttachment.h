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

namespace AZ::RHI
{
    class Scope;
    class ScopeAttachment;
    class SingleDeviceResource;

    //! FrameAttachment is the base class for all attachments stored in the frame graph. Attachments
    //! are "attached" to scopes via ScopeAttachment instances. These scope attachments form a linked list
    //! from the first to last scope. FrameAttachments are associated with a unique AttachmentId.
    //!
    //! FrameAttachments are rebuilt every frame, and are created through the FrameGraph.
    class FrameAttachment
    {
        friend class FrameGraphAttachmentDatabase;
        friend class FrameGraphCompiler;
    public:
        AZ_RTTI(FrameAttachment, "{F35548B3-4B2C-478C-9ED9-759CAEAE2729}");
        virtual ~FrameAttachment();

        /// Returns the attachment id.
        const AttachmentId& GetId() const;

        /// Returns the resource associated with this frame attachment.
        SingleDeviceResource* GetResource();
        const SingleDeviceResource* GetResource() const;

        /// Returns the attachment lifetime type.
        AttachmentLifetimeType GetLifetimeType() const;

        /// Returns the first scope attachment in the linked list.
        const ScopeAttachment* GetFirstScopeAttachment() const;
        ScopeAttachment* GetFirstScopeAttachment();

        /// Returns the last scope attachment in the linked list.
        const ScopeAttachment* GetLastScopeAttachment() const;
        ScopeAttachment* GetLastScopeAttachment();

        //! Returns the first / last scope associated with the lifetime of this attachment.
        //! The guarantee is that the attachment is not used by any scope with index prior to GetFirstScope
        //! or any scope with index after GetLastScope. It does not, however, guarantee that the attachment
        //! is actually used by either scope. The scope attachment list must be traversed to determine usage.
        Scope* GetFirstScope() const;
        Scope* GetLastScope() const;

        /// Returns the mask of all the hardware queues that this attachment is used on.
        HardwareQueueClassMask GetUsedQueueMask() const;

        /// Returns the mask of all the hardware queues that this attachment is supported on.
        HardwareQueueClassMask GetSupportedQueueMask() const;

        /// [Internal] Assigns the resource. This may only be done once.
        void SetResource(Ptr<SingleDeviceResource> resource);

    protected:
        FrameAttachment(
            const AttachmentId& attachmentId,
            HardwareQueueClassMask supportedQueueMask,
            AttachmentLifetimeType lifetimeType);

        FrameAttachment(const FrameAttachment&) = delete;
        FrameAttachment(FrameAttachment&&) = delete;

    private:
        AttachmentId m_attachmentId;
        Ptr<SingleDeviceResource> m_resource;
        AttachmentLifetimeType m_lifetimeType;
        HardwareQueueClassMask m_usedQueueMask = HardwareQueueClassMask::None;
        HardwareQueueClassMask m_supportedQueueMask = HardwareQueueClassMask::None;
        ScopeAttachment* m_firstScopeAttachment = nullptr;
        ScopeAttachment* m_lastScopeAttachment = nullptr;
        Scope* m_firstScope = nullptr;
        Scope* m_lastScope = nullptr;
    };
}
