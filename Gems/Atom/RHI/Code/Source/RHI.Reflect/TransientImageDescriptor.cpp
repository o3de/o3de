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

#include <Atom/RHI.Reflect/TransientImageDescriptor.h>
#include <AzCore/Utils/TypeHash.h>

namespace AZ
{
    namespace RHI
    {
        TransientImageDescriptor::TransientImageDescriptor(
            const AttachmentId& attachmentId,
            const ImageDescriptor& imageDescriptor,
            HardwareQueueClassMask supportedQueueMask,
            const ClearValue* optimizedClearValue)
            : m_attachmentId{attachmentId}
            , m_imageDescriptor{imageDescriptor}
            , m_supportedQueueMask{supportedQueueMask}
            , m_optimizedClearValue{optimizedClearValue}
        {}

        HashValue64 TransientImageDescriptor::GetHash(HashValue64 seed) const
        {
            seed = TypeHash64(m_attachmentId.GetHash(), seed);
            seed = m_imageDescriptor.GetHash(seed);
            seed = TypeHash64(m_supportedQueueMask, seed);
            if (m_optimizedClearValue)
            {
                seed = TypeHash64(m_optimizedClearValue->GetHash(seed), seed);
            }
            return seed;
        }
    }
}
