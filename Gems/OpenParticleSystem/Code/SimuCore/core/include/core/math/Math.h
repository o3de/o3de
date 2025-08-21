/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <algorithm>
#include <cstdint>

namespace SimuCore {
    class Math 
    {
    public:
        static float Fractional(float value);

        static float AngleToRadians(float angle);

        static float Abs(float value);

        static float InvSqrt(float value);

        static constexpr float PI = 3.14159265358979323846f;

        static constexpr float HALF_PI = 1.57079632679489661923f;

        static constexpr float EPSLON = 1e-8f;

        static const uint32_t INVALID_INDEX = 0xffffffff;

        static constexpr int32_t NEGATE_XYZ_MASK[4] = {
            static_cast<int32_t>(0x80000000), static_cast<int32_t>(0x80000000),
            static_cast<int32_t>(0x80000000), static_cast<int32_t>(0x00000000)
        };

        static bool Equal(float lval, float rval);

        static float FSign(float value);

        template <typename T>
        static T Min(const T& lhs, const T& rhs)
        {
            return lhs < rhs ? lhs : rhs;
        }

        template <typename T>
        static T Max(const T& lhs, const T& rhs)
        {
            return lhs > rhs ? lhs : rhs;
        }

        template<typename T>
        static constexpr T Clamp(T value, T min, T max)
        {
            if (value < min) {
                return min;
            } else if (value > max) {
                return max;
            } else {
                return value;
            }
        }

        template<typename T>
        static constexpr T Lerp(T begin, T end, T step)
        {
            return begin + step * (end - begin);
        }

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
