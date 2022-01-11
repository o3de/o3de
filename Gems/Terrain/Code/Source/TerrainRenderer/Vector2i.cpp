/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TerrainRenderer/Vector2i.h>

namespace Terrain
{
    auto Vector2i::operator+(const Vector2i& rhs) const -> Vector2i
    {
        Vector2i offsetPoint = *this;
        offsetPoint += rhs;
        return offsetPoint;
    }
    
    auto Vector2i::operator+=(const Vector2i& rhs) -> Vector2i&
    {
        m_x += rhs.m_x;
        m_y += rhs.m_y;
        return *this;
    }

    auto Vector2i::operator-(const Vector2i& rhs) const -> Vector2i
    {
        return *this + -rhs;
    }
    
    auto Vector2i::operator-=(const Vector2i& rhs) -> Vector2i&
    {
        return *this += -rhs;
    }
    
    auto Vector2i::operator-() const -> Vector2i
    {
        return {-m_x, -m_y};
    }
}
