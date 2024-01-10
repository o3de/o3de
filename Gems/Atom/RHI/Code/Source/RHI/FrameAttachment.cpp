/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/FrameAttachment.h>
#include <Atom/RHI/SingleDeviceResource.h>

namespace AZ::RHI
{
    FrameAttachment::FrameAttachment(
        const AttachmentId& attachmentId,
        HardwareQueueClassMask supportedQueueMask,
        AttachmentLifetimeType lifetimeType)
        : m_attachmentId{attachmentId}
        , m_supportedQueueMask{supportedQueueMask}
        , m_lifetimeType{lifetimeType}
    {
        AZ_Assert(!attachmentId.IsEmpty(), "Frame Attachment was created with an empty attachment id!");
    }

    FrameAttachment::~FrameAttachment()
    {
        if (m_resource)
        {
            m_resource->SetFrameAttachment(nullptr);
        }
    }

    const AttachmentId& FrameAttachment::GetId() const
    {
        return m_attachmentId;
    }

    AttachmentLifetimeType FrameAttachment::GetLifetimeType() const
    {
        return m_lifetimeType;
    }

    SingleDeviceResource* FrameAttachment::GetResource()
    {
        return m_resource.get();
    }

    const SingleDeviceResource* FrameAttachment::GetResource() const
    {
        return m_resource.get();
    }

    void FrameAttachment::SetResource(Ptr<SingleDeviceResource> resource)
    {
        AZ_Assert(!m_resource, "A resource has already been assigned to this frame attachment.");
        AZ_Assert(resource, "Assigning a null resource to attachment %s.", m_attachmentId.GetCStr());
        m_resource = AZStd::move(resource);
        m_resource->SetFrameAttachment(this);
    }

    const ScopeAttachment* FrameAttachment::GetFirstScopeAttachment() const
    {
        return m_firstScopeAttachment;
    }

    ScopeAttachment* FrameAttachment::GetFirstScopeAttachment()
    {
        return m_firstScopeAttachment;
    }

    const ScopeAttachment* FrameAttachment::GetLastScopeAttachment() const
    {
        return m_lastScopeAttachment;
    }

    ScopeAttachment* FrameAttachment::GetLastScopeAttachment()
    {
        return m_lastScopeAttachment;
    }

    Scope* FrameAttachment::GetLastScope() const
    {
        return m_lastScope;
    }

    Scope* FrameAttachment::GetFirstScope() const
    {
        return m_firstScope;
    }

    HardwareQueueClassMask FrameAttachment::GetSupportedQueueMask() const
    {
        return m_supportedQueueMask;
    }

    HardwareQueueClassMask FrameAttachment::GetUsedQueueMask() const
    {
        return m_usedQueueMask;
    }
}
