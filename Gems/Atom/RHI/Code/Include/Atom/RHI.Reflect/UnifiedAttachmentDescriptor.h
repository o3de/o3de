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

#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <Atom/RHI.Reflect/BufferDescriptor.h>
#include <Atom/RHI.Reflect/ImageDescriptor.h>

namespace AZ
{
    namespace RHI
    {
        /**
         *  A unified attachment descriptor used to store either an image or a buffer descriptor.
         *  Used primarily to simplify pass attachment logic while supporting both attachment types.
         */
        struct UnifiedAttachmentDescriptor
        {
            UnifiedAttachmentDescriptor();
            UnifiedAttachmentDescriptor(const BufferDescriptor& bufferDescriptor);
            UnifiedAttachmentDescriptor(const ImageDescriptor& imageDescriptor);

            HashValue64 GetHash(HashValue64 seed = HashValue64{ 0 }) const;

            /// Differentiates between an image, buffer and resolve attachment
            AttachmentType m_type = AttachmentType::Uninitialized;

            union
            {
                BufferDescriptor m_buffer;
                ImageDescriptor m_image;
            };
        };
    }
}
