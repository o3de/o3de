/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
