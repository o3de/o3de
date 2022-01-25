/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TerrainRenderer/Aabb2i.h>
#include <AzCore/Math/MathUtils.h>

namespace Terrain
{
    Aabb2i::Aabb2i(const Vector2i& min, const Vector2i& max)
        : m_min(min)
        , m_max(max)
    {}

    Aabb2i Aabb2i::operator+(const Vector2i& rhs) const
    {
        return { m_min + rhs, m_max + rhs };
    }

    Aabb2i Aabb2i::operator-(const Vector2i& rhs) const
    {
        return *this + -rhs;
    }

    Aabb2i Aabb2i::GetClamped(Aabb2i rhs) const
    {
        Aabb2i ret;
        ret.m_min.m_x = AZ::GetMax(m_min.m_x, rhs.m_min.m_x);
        ret.m_min.m_y = AZ::GetMax(m_min.m_y, rhs.m_min.m_y);
        ret.m_max.m_x = AZ::GetMin(m_max.m_x, rhs.m_max.m_x);
        ret.m_max.m_y = AZ::GetMin(m_max.m_y, rhs.m_max.m_y);
        return ret;
    }

    bool Aabb2i::IsValid() const
    {
        // Intentionally strict, equal min/max not valid.
        return m_min.m_x < m_max.m_x && m_min.m_y < m_max.m_y;
    }
}
