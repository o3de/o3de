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

#include <AzCore/Utils/TypeHash.h>

#include <Atom/RHI.Reflect/UnifiedAttachmentDescriptor.h>

namespace AZ
{
    namespace RHI
    {
        UnifiedAttachmentDescriptor::UnifiedAttachmentDescriptor()
        { }

        UnifiedAttachmentDescriptor::UnifiedAttachmentDescriptor(const BufferDescriptor& bufferDescriptor)
            : m_buffer(bufferDescriptor)
            , m_type(AttachmentType::Buffer)
        { }

        UnifiedAttachmentDescriptor::UnifiedAttachmentDescriptor(const ImageDescriptor& imageDescriptor)
            : m_image(imageDescriptor)
            , m_type(AttachmentType::Image)
        { }

        HashValue64 UnifiedAttachmentDescriptor::GetHash(HashValue64 seed /* = 0 */) const
        {
            return TypeHash64(*this, seed);
        }
    }
}
