/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/MathUtils.h>

namespace SimuCore {
    class Math 
    {
    public:
        template<typename T>
        static constexpr T CubicInterp(
            const std::pair<T, T>& pair0, const std::pair<T, T>& pair1, float step, float segmentLength)
        {
            T res;
            float a1 = step * step;
            float a2 = a1 * step;

            float s0 = (2.0f * a2) - (3.0f * a1) + 1.0f;
            float s1 = a2 - (2.0f * a1) + step;
            float s2 = (a2 - a1);
            float s3 = (-2.0f * a2) + (3.0f * a1);
            res = s0 * pair0.first + segmentLength * s1 * pair0.second +
                segmentLength * s2 * pair1.second + s3 * pair1.first;
            return res;
        }
    };
}
