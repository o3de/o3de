/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/BufferFrameAttachment.h>
#include <Atom/RHI/BufferScopeAttachment.h>
#include <Atom/RHI/DeviceBufferView.h>
#include <Atom/RHI/Factory.h>

namespace AZ::RHI
{
    BufferFrameAttachment::BufferFrameAttachment(
        const AttachmentId& attachmentId,
        Ptr<Buffer> buffer)
        : FrameAttachment(
            attachmentId,
            HardwareQueueClassMask::All,
            AttachmentLifetimeType::Imported)
        , m_bufferDescriptor{buffer->GetDescriptor()}
    {
        SetResource(AZStd::move(buffer));
    }

    BufferFrameAttachment::BufferFrameAttachment(const TransientBufferDescriptor& descriptor)
        : FrameAttachment(
            descriptor.m_attachmentId,
            HardwareQueueClassMask::All,
            AttachmentLifetimeType::Transient)
        , m_bufferDescriptor{descriptor.m_bufferDescriptor}
    {}

    const BufferScopeAttachment* BufferFrameAttachment::GetFirstScopeAttachment(int deviceIndex) const
    {
        return static_cast<const BufferScopeAttachment*>(FrameAttachment::GetFirstScopeAttachment(deviceIndex));
    }

    BufferScopeAttachment* BufferFrameAttachment::GetFirstScopeAttachment(int deviceIndex)
    {
        return static_cast<BufferScopeAttachment*>(FrameAttachment::GetFirstScopeAttachment(deviceIndex));
    }

    const BufferScopeAttachment* BufferFrameAttachment::GetLastScopeAttachment(int deviceIndex) const
    {
        return static_cast<const BufferScopeAttachment*>(FrameAttachment::GetLastScopeAttachment(deviceIndex));
    }

    BufferScopeAttachment* BufferFrameAttachment::GetLastScopeAttachment(int deviceIndex)
    {
        return static_cast<BufferScopeAttachment*>(FrameAttachment::GetLastScopeAttachment(deviceIndex));
    }

    const BufferDescriptor& BufferFrameAttachment::GetBufferDescriptor() const
    {
        return m_bufferDescriptor;
    }

    const Buffer* BufferFrameAttachment::GetBuffer() const
    {
        return static_cast<const Buffer*>(GetResource());
    }

    Buffer* BufferFrameAttachment::GetBuffer()
    {
        return static_cast<Buffer*>(GetResource());
    }
}
