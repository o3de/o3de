/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/TransientImageDescriptor.h>
#include <AzCore/Utils/TypeHash.h>

namespace AZ::RHI
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
