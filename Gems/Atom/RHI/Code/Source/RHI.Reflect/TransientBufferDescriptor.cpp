/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/TransientBufferDescriptor.h>
#include <AzCore/Utils/TypeHash.h>

namespace AZ::RHI
{
    TransientBufferDescriptor::TransientBufferDescriptor(
        const AttachmentId& attachmentId,
        const BufferDescriptor& bufferDescriptor)
        : m_attachmentId{attachmentId}
        , m_bufferDescriptor{bufferDescriptor}
    {}

    HashValue64 TransientBufferDescriptor::GetHash(HashValue64 seed) const
    {
        seed = TypeHash64(m_attachmentId.GetHash(), seed);
        seed = m_bufferDescriptor.GetHash(seed);
        return seed;
    }
}
