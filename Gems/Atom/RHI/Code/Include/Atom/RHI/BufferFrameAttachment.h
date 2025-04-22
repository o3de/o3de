/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/TransientBufferDescriptor.h>
#include <Atom/RHI/FrameAttachment.h>
#include <Atom/RHI/Buffer.h>
#include <Atom/RHI/ObjectCache.h>
#include <AzCore/Memory/PoolAllocator.h>

namespace AZ::RHI
{
    class DeviceBufferView;
    class BufferScopeAttachment;

    //! A specialization of Attachment for a buffer. Provides access to the buffer.
    class BufferFrameAttachment
        : public FrameAttachment
    {
    public:
        AZ_RTTI(BufferFrameAttachment, "{2E6463F2-AB93-46C4-AD3C-30C3DD0B7151}", FrameAttachment);
        AZ_CLASS_ALLOCATOR(BufferFrameAttachment, SystemAllocator);
        virtual ~BufferFrameAttachment() override = default;

        /// Initialization for imported buffers.
        BufferFrameAttachment(const AttachmentId& attachmentId, Ptr<Buffer> buffer);

        /// Initialization for transient buffers.
        BufferFrameAttachment(const TransientBufferDescriptor& descriptor);

        /// Returns the first scope attachment in the linked list.
        const BufferScopeAttachment* GetFirstScopeAttachment(int deviceIndex) const;
        BufferScopeAttachment* GetFirstScopeAttachment(int deviceIndex);

        /// Returns the last scope attachment in the linked list.
        const BufferScopeAttachment* GetLastScopeAttachment(int deviceIndex) const;
        BufferScopeAttachment* GetLastScopeAttachment(int deviceIndex);

        /// Returns the buffer resource assigned to this attachment. This is not guaranteed to exist
        /// until after frame graph compilation.
        const Buffer* GetBuffer() const;
        Buffer* GetBuffer();

        /// Returns the buffer descriptor assigned to this attachment.
        const BufferDescriptor& GetBufferDescriptor() const;

    protected:
        BufferDescriptor m_bufferDescriptor;
    };
}
