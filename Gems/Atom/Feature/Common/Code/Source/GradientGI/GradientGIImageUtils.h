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

namespace AZ::Render::GradientGI
{
    // O3DE has no public half-float utility. This is a standard bit-twiddling
    // conversion matching the private implementations in RPIUtils.cpp and
    // AcesDisplayMapperFeatureProcessor.cpp.

    inline uint16_t FloatToHalf(float value)
    {
        uint32_t fbits;
        std::memcpy(&fbits, &value, sizeof(uint32_t));

        uint32_t sign = (fbits >> 16) & 0x8000u;
        int32_t exponent = ((fbits >> 23) & 0xFF) - 127;
        uint32_t mantissa = fbits & 0x007FFFFFu;

        // Zero / denorm
        if (exponent < -14)
        {
            return static_cast<uint16_t>(sign);
        }

        // Overflow -> Inf
        if (exponent > 15)
        {
            return static_cast<uint16_t>(sign | 0x7C00u);
        }

        // Normal
        uint16_t halfExponent = static_cast<uint16_t>((exponent + 15) << 10);
        uint16_t halfMantissa = static_cast<uint16_t>(mantissa >> 13);
        return static_cast<uint16_t>(sign | halfExponent | halfMantissa);
    }

    inline void WriteRgba16FloatPixel(
        uint8_t* destination,
        float red,
        float green,
        float blue,
        float alpha)
    {
        const uint16_t pixel[] =
        {
            FloatToHalf(red),
            FloatToHalf(green),
            FloatToHalf(blue),
            FloatToHalf(alpha)
        };
        std::memcpy(destination, pixel, sizeof(pixel));
    }

} // namespace AZ::Render::GradientGI
