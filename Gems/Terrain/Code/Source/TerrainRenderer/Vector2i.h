/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/Math/Vector2.h>

namespace Terrain
{
    class Vector2i
    {
    public:

        Vector2i() = default;
        Vector2i(int32_t x, int32_t y);
        Vector2i(uint32_t value);
        Vector2i(int32_t value);

        AZ::Vector2 ToVector2();

        Vector2i operator+(const Vector2i& rhs) const;
        Vector2i& operator+=(const Vector2i& rhs);
        Vector2i operator-(const Vector2i& rhs) const;
        Vector2i& operator-=(const Vector2i& rhs);
        Vector2i operator-() const;
        
        Vector2i operator*(const Vector2i& rhs) const;
        Vector2i& operator*=(const Vector2i& rhs);
        Vector2i operator/(const Vector2i& rhs) const;
        Vector2i& operator/=(const Vector2i& rhs);

        bool operator==(const Vector2i& rhs) const;
        bool operator!=(const Vector2i& rhs) const;

        int32_t m_x{ 0 };
        int32_t m_y{ 0 };
    };
}
