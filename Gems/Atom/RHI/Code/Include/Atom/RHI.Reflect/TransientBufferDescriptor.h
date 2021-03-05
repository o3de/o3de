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
#pragma once

#include <Atom/RHI.Reflect/BufferDescriptor.h>
#include <Atom/RHI.Reflect/AttachmentId.h>
#include <Atom/RHI.Reflect/AttachmentEnums.h>

namespace AZ
{
    namespace RHI
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
}
