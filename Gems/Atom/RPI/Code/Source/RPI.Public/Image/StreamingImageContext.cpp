/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Image/StreamingImageContext.h>

namespace AZ
{
    namespace RPI
    {
        StreamingImage* StreamingImageContext::TryGetImage() const
        {
            return m_streamingImage;
        }

        uint16_t StreamingImageContext::GetTargetMip() const
        {
            return m_mipLevelTarget;
        }

        size_t StreamingImageContext::GetLastAccessTimestamp() const
        {
            return m_lastAccessTimestamp;
        }
    }
}
