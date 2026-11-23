/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <cstring>

namespace AZ::Render::GradientGlobalIllumination
{
    //! Round binary32 to binary16 using round-to-nearest, ties-to-even.
    inline uint16_t FloatToHalf(float value)
    {
        uint32_t bits;
        std::memcpy(&bits, &value, sizeof(bits));
        const uint32_t sign = (bits >> 16) & 0x8000u;
        const uint32_t magnitude = bits & 0x7fffffffu;
        if (magnitude > 0x7f800000u)
        {
            return static_cast<uint16_t>(sign | 0x7e00u);
        }
        if (magnitude >= 0x477ff000u)
        {
            return static_cast<uint16_t>(sign | 0x7c00u);
        }
        if (magnitude < 0x33000000u)
        {
            return static_cast<uint16_t>(sign);
        }

        uint32_t shift = 13;
        uint32_t significand = magnitude - 0x38000000u;
        if (magnitude < 0x38800000u)
        {
            shift = 126 - (magnitude >> 23);
            significand = (magnitude & 0x7fffffu) | 0x800000u;
        }
        const uint32_t rounded = (significand + (1u << (shift - 1)) - 1u + ((significand >> shift) & 1u)) >> shift;
        return static_cast<uint16_t>(sign | rounded);
    }

    inline void WriteRgba16FloatPixel(uint8_t* destination, float red, float green, float blue, float alpha)
    {
        const uint16_t pixel[] = { FloatToHalf(red), FloatToHalf(green), FloatToHalf(blue), FloatToHalf(alpha) };
        std::memcpy(destination, pixel, sizeof(pixel));
    }
} // namespace AZ::Render::GradientGlobalIllumination
