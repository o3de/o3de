/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TerrainRenderer/Vector2i.h>
#include <AzCore/std/limits.h>

namespace Terrain
{
    class Aabb2i
    {
    public:

        Aabb2i() = default;
        Aabb2i(const Vector2i& min, const Vector2i& max);

        Aabb2i operator+(const Vector2i& offset) const;
        Aabb2i operator-(const Vector2i& offset) const;

        Aabb2i GetClamped(Aabb2i rhs) const;
        bool IsValid() const;

        
        Vector2i m_min{AZStd::numeric_limits<int32_t>::min(), AZStd::numeric_limits<int32_t>::min()};
        Vector2i m_max{AZStd::numeric_limits<int32_t>::max(), AZStd::numeric_limits<int32_t>::max()};

    };
}
