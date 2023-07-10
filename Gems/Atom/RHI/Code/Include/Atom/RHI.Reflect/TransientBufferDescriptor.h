/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/BufferDescriptor.h>
#include <Atom/RHI.Reflect/AttachmentId.h>
#include <Atom/RHI.Reflect/AttachmentEnums.h>

namespace AZ::RHI
{
    struct TransientBufferDescriptor
    {
        TransientBufferDescriptor() = default;

        TransientBufferDescriptor(
            const AttachmentId& attachmentId,
            const BufferDescriptor& bufferDescriptor);

        /// The attachment id to associate with the transient buffer.
        AttachmentId m_attachmentId;

        /// The buffer descriptor used to create the transient buffer.
        BufferDescriptor m_bufferDescriptor;

        HashValue64 GetHash(HashValue64 seed = HashValue64{ 0 }) const;
    };
}
