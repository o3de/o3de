/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Base.h>

namespace AZ
{
    class ReflectContext;
}

namespace AZ::RHI
{
    struct Interval
    {
        AZ_TYPE_INFO(Interval, "{B121C9FE-1C23-4721-9C3E-6BE036612743}");
        static void Reflect(AZ::ReflectContext* context);

        Interval() = default;
        Interval(uint32_t min, uint32_t max);

        uint32_t m_min = 0;
        uint32_t m_max = 0;

        bool operator == (const Interval& rhs) const;
        bool operator != (const Interval& rhs) const;

        //! Return true if it overlaps with an interval.
        //! Overlapping m_min or m_max counts as interval overlap (e.g. [0, 3] overlaps with [3, 4])
        bool Overlaps(const Interval& rhs) const;
    };
}
