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

#include <Atom/RHI/BufferFrameAttachment.h>
#include <Atom/RHI/BufferScopeAttachment.h>
#include <Atom/RHI/BufferView.h>
#include <Atom/RHI/Factory.h>

namespace AZ
{
    namespace RHI
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

        const Buffer* BufferFrameAttachment::GetBuffer() const
        {
            return static_cast<const Buffer*>(GetResource());
        }

        Buffer* BufferFrameAttachment::GetBuffer()
        {
            return static_cast<Buffer*>(GetResource());
        }
    }
}