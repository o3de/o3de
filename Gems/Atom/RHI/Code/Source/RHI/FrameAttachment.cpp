/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/FrameAttachment.h>
#include <Atom/RHI/Resource.h>

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

    Resource* FrameAttachment::GetResource()
    {
        return m_resource.get();
    }

    const Resource* FrameAttachment::GetResource() const
    {
        return m_resource.get();
    }

    void FrameAttachment::SetResource(Ptr<Resource> resource, int deviceIndex)
    {
        AZ_Assert(!m_resource || (m_resource == resource), "A different resource has already been assigned to this frame attachment.");
        AZ_Assert(resource, "Assigning a null resource to attachment %s.", m_attachmentId.GetCStr());
        m_resource = AZStd::move(resource);
        m_resource->SetFrameAttachment(this, deviceIndex);
    }

    const ScopeAttachment* FrameAttachment::GetFirstScopeAttachment(int deviceIndex) const
    {
        auto it = m_firstScopeAttachments.find(deviceIndex);

        if (it == m_firstScopeAttachments.end())
        {
            return nullptr;
        }

        return it->second;
    }

    ScopeAttachment* FrameAttachment::GetFirstScopeAttachment(int deviceIndex)
    {
        auto it = m_firstScopeAttachments.find(deviceIndex);

        if (it == m_firstScopeAttachments.end())
        {
            return nullptr;
        }

        return it->second;
    }

    const ScopeAttachment* FrameAttachment::GetLastScopeAttachment(int deviceIndex) const
    {
        auto it = m_lastScopeAttachments.find(deviceIndex);

        if (it == m_lastScopeAttachments.end())
        {
            return nullptr;
        }

        return it->second;
    }

    ScopeAttachment* FrameAttachment::GetLastScopeAttachment(int deviceIndex)
    {
        auto it = m_lastScopeAttachments.find(deviceIndex);

        if (it == m_lastScopeAttachments.end())
        {
            return nullptr;
        }

        return it->second;
    }

    bool FrameAttachment::HasScopeAttachments() const
    {
        return !m_firstScopeAttachments.empty();
    }

    Scope* FrameAttachment::GetLastScope(int deviceIndex) const
    {
        auto it = m_lastScopes.find(deviceIndex);

        if (it == m_lastScopes.end())
        {
            return nullptr;
        }

        return it->second;
    }

    Scope* FrameAttachment::GetFirstScope(int deviceIndex) const
    {
        auto it = m_firstScopes.find(deviceIndex);

        if (it == m_firstScopes.end())
        {
            return nullptr;
        }

        return it->second;
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
