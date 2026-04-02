/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Math/Curve.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AzFramework
{
    Curve::Curve()
    {
        KeyPoint first;
        KeyPoint last;
        first.m_time = 0.0f;
        first.m_value = 1.0f;
        last.m_time = 1.0f;
        last.m_value = 1.0f;
        m_keyPoints.emplace_back(first);
        m_keyPoints.emplace_back(last);
    }

    void Curve::KeyPoint::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<KeyPoint>()
                ->Version(1)
                ->Field("time", &KeyPoint::m_time)
                ->Field("value", &KeyPoint::m_value);
        }
    }

    void Curve::Reflect(AZ::ReflectContext* context)
    {
        KeyPoint::Reflect(context);
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<Curve>()
                ->Version(1)
                ->Field("leftExtrapMode", &Curve::m_leftExtrapMode)
                ->Field("rightExtrapMode", &Curve::m_rightExtrapMode)
                ->Field("valueFactor", &Curve::m_valueFactor)
                ->Field("timeFactor", &Curve::m_timeFactor)
                ->Field("tickMode", &Curve::m_tickMode)
                ->Field("keyPoints", &Curve::m_keyPoints);
        }
    }
}
