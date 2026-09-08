/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <Atom/RHI.Reflect/Format.h>

namespace AZ::Render::GradientGI
{
    // =========================================================================
    // IEEE 754 Float32 -> Float16 Conversion
    // =========================================================================
    // O3DE has no public half-float utility. This is a standard bit-twiddling
    // conversion matching the private implementations in RPIUtils.cpp and
    // AcesDisplayMapperFeatureProcessor.cpp.

    inline uint16_t FloatToHalf(float value)
    {
        uint32_t fbits;
        memcpy(&fbits, &value, sizeof(uint32_t));

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

        // NaN
        if (exponent == 128)
        {
            return static_cast<uint16_t>(sign | 0x7C00u | (mantissa >> 13));
        }

        // Normal
        uint16_t halfExponent = static_cast<uint16_t>((exponent + 15) << 10);
        uint16_t halfMantissa = static_cast<uint16_t>(mantissa >> 13);
        return static_cast<uint16_t>(sign | halfExponent | halfMantissa);
    }

    // =========================================================================
    // R11G11B10_FLOAT Packing
    // =========================================================================
    // Packs three positive floats into a single uint32_t.
    // R: 5-bit exponent + 6-bit mantissa (11 bits)
    // G: 5-bit exponent + 6-bit mantissa (11 bits)
    // B: 5-bit exponent + 5-bit mantissa (10 bits)

    inline uint32_t FloatToR11(float value)
    {
        if (value <= 0.0f) return 0;
        uint32_t fbits;
        memcpy(&fbits, &value, sizeof(uint32_t));
        int32_t exp = ((fbits >> 23) & 0xFF) - 127 + 15;
        if (exp < 0) return 0;
        if (exp > 30) return 0x7FFu; // max
        uint32_t mant = (fbits >> (23 - 6)) & 0x3Fu;
        return (static_cast<uint32_t>(exp) << 6) | mant;
    }

    inline uint32_t FloatToR10(float value)
    {
        if (value <= 0.0f) return 0;
        uint32_t fbits;
        memcpy(&fbits, &value, sizeof(uint32_t));
        int32_t exp = ((fbits >> 23) & 0xFF) - 127 + 15;
        if (exp < 0) return 0;
        if (exp > 30) return 0x3FFu;
        uint32_t mant = (fbits >> (23 - 5)) & 0x1Fu;
        return (static_cast<uint32_t>(exp) << 5) | mant;
    }

    inline uint32_t PackR11G11B10(float r, float g, float b)
    {
        return FloatToR11(r) | (FloatToR11(g) << 11) | (FloatToR10(b) << 22);
    }

    // =========================================================================
    // Pixel Writers
    // =========================================================================

    inline size_t GetBytesPerPixel(RHI::Format format)
    {
        switch (format)
        {
        case RHI::Format::R16G16B16A16_FLOAT: return 8;
        case RHI::Format::R11G11B10_FLOAT:    return 4;
        case RHI::Format::R8G8B8A8_UNORM:     return 4;
        default: return 8;
        }
    }

    inline void WritePixel(uint8_t* dst, float r, float g, float b, float a, RHI::Format format)
    {
        switch (format)
        {
        case RHI::Format::R16G16B16A16_FLOAT:
        {
            uint16_t* out = reinterpret_cast<uint16_t*>(dst);
            out[0] = FloatToHalf(r);
            out[1] = FloatToHalf(g);
            out[2] = FloatToHalf(b);
            out[3] = FloatToHalf(a);
            break;
        }
        case RHI::Format::R11G11B10_FLOAT:
        {
            uint32_t* out = reinterpret_cast<uint32_t*>(dst);
            out[0] = PackR11G11B10(r, g, b);
            break;
        }
        case RHI::Format::R8G8B8A8_UNORM:
        {
            dst[0] = static_cast<uint8_t>(AZStd::clamp(r, 0.0f, 1.0f) * 255.0f);
            dst[1] = static_cast<uint8_t>(AZStd::clamp(g, 0.0f, 1.0f) * 255.0f);
            dst[2] = static_cast<uint8_t>(AZStd::clamp(b, 0.0f, 1.0f) * 255.0f);
            dst[3] = static_cast<uint8_t>(AZStd::clamp(a, 0.0f, 1.0f) * 255.0f);
            break;
        }
        default:
            break;
        }
    }

} // namespace AZ::Render::GradientGI
