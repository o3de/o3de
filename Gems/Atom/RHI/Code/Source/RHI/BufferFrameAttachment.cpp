/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/BufferFrameAttachment.h>
#include <Atom/RHI/BufferScopeAttachment.h>
#include <Atom/RHI/BufferView.h>
#include <Atom/RHI/Factory.h>

namespace AZ::RHI
{
    BufferFrameAttachment::BufferFrameAttachment(
        const AttachmentId& attachmentId,
        Ptr<SingleDeviceBuffer> buffer)
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

    const BufferScopeAttachment* BufferFrameAttachment::GetFirstScopeAttachment() const
    {
        return static_cast<const BufferScopeAttachment*>(FrameAttachment::GetFirstScopeAttachment());
    }

    BufferScopeAttachment* BufferFrameAttachment::GetFirstScopeAttachment()
    {
        return static_cast<BufferScopeAttachment*>(FrameAttachment::GetFirstScopeAttachment());
    }

    const BufferScopeAttachment* BufferFrameAttachment::GetLastScopeAttachment() const
    {
        return static_cast<const BufferScopeAttachment*>(FrameAttachment::GetLastScopeAttachment());
    }

    BufferScopeAttachment* BufferFrameAttachment::GetLastScopeAttachment()
    {
        return static_cast<BufferScopeAttachment*>(FrameAttachment::GetLastScopeAttachment());
    }

    const BufferDescriptor& BufferFrameAttachment::GetBufferDescriptor() const
    {
        return m_bufferDescriptor;
    }

    const SingleDeviceBuffer* BufferFrameAttachment::GetBuffer() const
    {
        return static_cast<const SingleDeviceBuffer*>(GetResource());
    }

    SingleDeviceBuffer* BufferFrameAttachment::GetBuffer()
    {
        return static_cast<SingleDeviceBuffer*>(GetResource());
    }
}
