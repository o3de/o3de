/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <cmath>
#include "core/math/Math.h"

namespace SimuCore {
    float Math::Fractional(float value)
    {
        return value - std::truncf(value);
    }

    float Math::AngleToRadians(float angle)
    {
        return angle / 180.f * PI;
    }

    float Math::Abs(float value)
    {
        return std::abs(value);
    }

    float Math::InvSqrt(float value)
    {
        return 1.0f / sqrtf(value);
    }

    bool Math::Equal(float lval, float rval)
    {
        return std::fabs(lval - rval) < EPSLON;
    }

    float Math::FSign(float value)
    {
        if (value < 0.0f) {
            return -1.0f;
        } else {
            return 1.0f;
        }
    }
}

