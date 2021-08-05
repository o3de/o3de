/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AZ
{
    namespace Simd
    {
        namespace Scalar
        {
            AZ_MATH_INLINE float Wrap(float value, float maxValue)
            {
                return fmodf(value, maxValue) + ((value < 0.0f) ? maxValue : 0.0f);
            }

            AZ_MATH_INLINE float Wrap(float value, float minValue, float maxValue)
            {
                return Wrap(value - minValue, maxValue - minValue) + minValue;
            }

            AZ_MATH_INLINE float AngleMod(float value)
            {
                return (value >= 0.0f) ? (fmodf(value + Constants::Pi, Constants::TwoPi) - Constants::Pi) : (fmodf(value - Constants::Pi, Constants::TwoPi) + Constants::Pi);
            }
        }
    }
}
