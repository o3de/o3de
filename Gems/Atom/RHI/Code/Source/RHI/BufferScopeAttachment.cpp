/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/BufferScopeAttachment.h>
#include <Atom/RHI/BufferFrameAttachment.h>
#include <Atom/RHI/DeviceBuffer.h>

namespace AZ::RHI
{
    BufferScopeAttachment::BufferScopeAttachment(
        Scope& scope,
        FrameAttachment& attachment,
        ScopeAttachmentUsage usage,
        ScopeAttachmentAccess access,
        ScopeAttachmentStage stage,
        const BufferScopeAttachmentDescriptor& descriptor)
        : ScopeAttachment(scope, attachment, usage, access, stage)
        , m_descriptor{descriptor}
    {
        AZ_Assert(
            m_descriptor.m_bufferViewDescriptor.m_elementSize > 0 &&
            m_descriptor.m_bufferViewDescriptor.m_elementCount > 0,
            "Invalid buffer view for attachment.");

        if (m_descriptor.m_loadStoreAction.m_loadAction == AttachmentLoadAction::Clear)
        {
            AZ_Error(
                "FrameScheduler",
                CheckBitsAny(access, ScopeAttachmentAccess::Write),
                "Attempting to clear an attachment that is read-only");
        }
    }

    const BufferScopeAttachmentDescriptor& BufferScopeAttachment::GetDescriptor() const
    {
        return m_descriptor;
    }

    const BufferView* BufferScopeAttachment::GetBufferView() const
    {
        return static_cast<const BufferView*>(ScopeAttachment::GetResourceView());
    }

    void BufferScopeAttachment::SetBufferView(ConstPtr<BufferView> bufferView)
    {
        SetResourceView(AZStd::move(bufferView));
    }

    const ScopeAttachmentDescriptor& BufferScopeAttachment::GetScopeAttachmentDescriptor() const
    {
        return GetDescriptor();
    }

    const BufferFrameAttachment& BufferScopeAttachment::GetFrameAttachment() const
    {
        return static_cast<const BufferFrameAttachment&>(ScopeAttachment::GetFrameAttachment());
    }

    BufferFrameAttachment& BufferScopeAttachment::GetFrameAttachment()
    {
        return static_cast<BufferFrameAttachment&>(ScopeAttachment::GetFrameAttachment());
    }

    const BufferScopeAttachment* BufferScopeAttachment::GetPrevious() const
    {
        return static_cast<const BufferScopeAttachment*>(ScopeAttachment::GetPrevious());
    }

    BufferScopeAttachment* BufferScopeAttachment::GetPrevious()
    {
        return static_cast<BufferScopeAttachment*>(ScopeAttachment::GetPrevious());
    }

    const BufferScopeAttachment* BufferScopeAttachment::GetNext() const
    {
        return static_cast<const BufferScopeAttachment*>(ScopeAttachment::GetNext());
    }

    BufferScopeAttachment* BufferScopeAttachment::GetNext()
    {
        return static_cast<BufferScopeAttachment*>(ScopeAttachment::GetNext());
    }
}
