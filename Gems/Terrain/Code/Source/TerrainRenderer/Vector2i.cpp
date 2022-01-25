/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TerrainRenderer/Vector2i.h>
#include <AzCore/Casting/numeric_cast.h>

namespace Terrain
{
    
    Vector2i::Vector2i(int32_t x, int32_t y)
        : m_x(x)
        , m_y(y)
    {}

    Vector2i::Vector2i(uint32_t value)
        : m_x(aznumeric_cast<int32_t>(value))
        , m_y(aznumeric_cast<int32_t>(value))
    {}

    Vector2i::Vector2i(int32_t value)
        : m_x(value)
        , m_y(value)
    {}

    Vector2i Vector2i::operator+(const Vector2i& rhs) const
    {
        Vector2i returnPoint = *this;
        returnPoint += rhs;
        return returnPoint;
    }
    
    Vector2i& Vector2i::operator+=(const Vector2i& rhs)
    {
        m_x += rhs.m_x;
        m_y += rhs.m_y;
        return *this;
    }

    Vector2i Vector2i::operator-(const Vector2i& rhs) const
    {
        return *this + -rhs;
    }
    
    Vector2i& Vector2i::operator-=(const Vector2i& rhs)
    {
        return *this += -rhs;
    }
    
    Vector2i Vector2i::operator-() const
    {
        return {-m_x, -m_y};
    }
    
    Vector2i Vector2i::operator*(const Vector2i& rhs) const
    {
        Vector2i returnPoint = *this;
        returnPoint *= rhs;
        return returnPoint;
    }
    
    Vector2i& Vector2i::operator*=(const Vector2i& rhs)
    {
        m_x *= rhs.m_x;
        m_y *= rhs.m_y;
        return *this;
    }

    Vector2i Vector2i::operator/(const Vector2i& rhs) const
    {
        Vector2i returnPoint = *this;
        returnPoint /= rhs;
        return returnPoint;
    }
    
    Vector2i& Vector2i::operator/=(const Vector2i& rhs)
    {
        m_x /= rhs.m_x;
        m_y /= rhs.m_y;
        return *this;
    }
    
    bool Vector2i::operator==(const Vector2i& rhs) const
    {
        return rhs.m_x == m_x && rhs.m_y == m_y;
    }
    
    bool Vector2i::operator!=(const Vector2i& rhs) const
    {
        return !(*this == rhs);
    }
}
