/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/Interval.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ::RHI
{
    void Interval::Reflect(AZ::ReflectContext* context)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<Interval>()
                ->Version(1)
                ->Field("m_min", &Interval::m_min)
                ->Field("m_max", &Interval::m_max);
        }
    }

    Interval::Interval(uint32_t min, uint32_t max)
        : m_min{min}
        , m_max{max}
    {}

    bool Interval::operator == (const Interval& rhs) const
    {
        return m_min == rhs.m_min && m_max == rhs.m_max;
    }

    bool Interval::operator != (const Interval& rhs) const
    {
        return m_min != rhs.m_min || m_max != rhs.m_max;
    }

    bool Interval::Overlaps(const Interval& rhs) const
    {
        return m_min <= rhs.m_max && rhs.m_min <= m_max;
    }
}
