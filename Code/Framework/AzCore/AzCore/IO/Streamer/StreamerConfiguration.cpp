/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/Streamer/StreamerConfiguration.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ::IO
{
    void IStreamerStackConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
        {
            serializeContext->Class<IStreamerStackConfig>()
                ->Version(1)
                ->SerializeWithNoData();
        }
    }

    void StreamerConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
        {
            serializeContext->Class<StreamerConfig>()
                ->Version(1)
                ->Field("Stack", &StreamerConfig::m_stackConfig);
        }
    }
} // namespace AZ::IO
