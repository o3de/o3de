/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <cmath>

namespace AZStd
{
    using std::abs;
    using std::acos;
    using std::asin;
    using std::atan;
    using std::atan2;
    using std::ceil;
    using std::cos;
    using std::exp;
    using std::exp2;
    using std::floor;
    using std::fmod;
    using std::llround;
    using std::lround;
    using std::pow;
    using std::round;
    using std::sin;
    using std::sqrt;
    using std::tan;
    using std::trunc;

} // namespace AZStd

// from c++20 standard
namespace AZStd::Internal
{
    template<typename T>
    constexpr T lerp(T a, T b, T t) noexcept
    {
        if ((a <= 0 && b >= 0) || (a >= 0 && b <= 0))
        {
            return t * b + (1 - t) * a;
        }
        if (t == 1)
        {
            return b;
        }
        const T x = a + t * (b - a);
        if ((t > 1) == (b > a))
        {
            return b < x ? x : b;
        }
        else
        {
            return x < b ? x : b;
        }
    }
} // namespace AZStd::Internal

namespace AZStd
{
    constexpr float lerp(float a, float b, float t) noexcept
    {
        return Internal::lerp(a, b, t);
    }

    constexpr double lerp(double a, double b, double t) noexcept
    {
        return Internal::lerp(a, b, t);
    }

    constexpr long double lerp(long double a, long double b, long double t) noexcept
    {
        return Internal::lerp(a, b, t);
    }
} // namespace AZStd
