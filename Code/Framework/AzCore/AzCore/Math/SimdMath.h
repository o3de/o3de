/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Internal/MathTypes.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/std/algorithm.h>

// If no supported SIMD types are enabled on this platform, fall back to scalar
#if !AZ_TRAIT_USE_PLATFORM_SIMD_SSE
#   if !AZ_TRAIT_USE_PLATFORM_SIMD_NEON
#       if !AZ_TRAIT_USE_PLATFORM_SIMD_SCALAR
#           define AZ_TRAIT_USE_PLATFORM_SIMD_SCALAR 1
#       endif
#   endif
#endif

namespace AZ
{
    namespace Simd
    {
        alignas(16) constexpr float g_vec1111[4]         = { 1.0f, 1.0f, 1.0f, 1.0f };
        alignas(16) constexpr float g_vec1000[4]         = { 1.0f, 0.0f, 0.0f, 0.0f };
        alignas(16) constexpr float g_vec0100[4]         = { 0.0f, 1.0f, 0.0f, 0.0f };
        alignas(16) constexpr float g_vec0010[4]         = { 0.0f, 0.0f, 1.0f, 0.0f };
        alignas(16) constexpr float g_vec0001[4]         = { 0.0f, 0.0f, 0.0f, 1.0f };
        alignas(16) constexpr float g_Pi[4]              = { Constants::Pi, Constants::Pi, Constants::Pi, Constants::Pi };
        alignas(16) constexpr float g_TwoPi[4]           = { Constants::TwoPi, Constants::TwoPi, Constants::TwoPi, Constants::TwoPi };
        alignas(16) constexpr float g_HalfPi[4]          = { Constants::HalfPi, Constants::HalfPi, Constants::HalfPi, Constants::HalfPi };
        alignas(16) constexpr float g_QuarterPi[4]       = { Constants::QuarterPi, Constants::QuarterPi, Constants::QuarterPi, Constants::QuarterPi };
        alignas(16) constexpr float g_TwoOverPi[4]       = { Constants::TwoOverPi, Constants::TwoOverPi, Constants::TwoOverPi, Constants::TwoOverPi };
        alignas(16) constexpr int32_t g_absMask[4]       = { (int32_t)0x7fffffff, (int32_t)0x7fffffff, (int32_t)0x7fffffff, (int32_t)0x7fffffff };
        alignas(16) constexpr int32_t g_negateMask[4]    = { (int32_t)0x80000000, (int32_t)0x80000000, (int32_t)0x80000000, (int32_t)0x80000000 };
        alignas(16) constexpr int32_t g_negateXMask[4]   = { (int32_t)0x80000000, (int32_t)0x00000000, (int32_t)0x00000000, (int32_t)0x00000000 };
        alignas(16) constexpr int32_t g_negateYMask[4]   = { (int32_t)0x00000000, (int32_t)0x80000000, (int32_t)0x00000000, (int32_t)0x00000000 };
        alignas(16) constexpr int32_t g_negateZMask[4]   = { (int32_t)0x00000000, (int32_t)0x00000000, (int32_t)0x80000000, (int32_t)0x00000000 };
        alignas(16) constexpr int32_t g_negateWMask[4]   = { (int32_t)0x00000000, (int32_t)0x00000000, (int32_t)0x00000000, (int32_t)0x80000000 };
        alignas(16) constexpr int32_t g_negateXYZMask[4] = { (int32_t)0x80000000, (int32_t)0x80000000, (int32_t)0x80000000, (int32_t)0x00000000 };
        alignas(16) constexpr int32_t g_wMask[4]         = { (int32_t)0xffffffff, (int32_t)0xffffffff, (int32_t)0xffffffff, (int32_t)0x00000000 };
    }
}

#include <AzCore/Math/SimdMathVec1.h>
#include <AzCore/Math/SimdMathVec2.h>
#include <AzCore/Math/SimdMathVec3.h>
#include <AzCore/Math/SimdMathVec4.h>

namespace AZ
{
    AZ_MATH_INLINE float Abs(float value)
    {
        return Simd::Vec1::SelectIndex0(Simd::Vec1::Abs(Simd::Vec1::Splat(value)));
    }

    AZ_MATH_INLINE float Mod(float value, float divisor)
    {
        return Simd::Vec1::SelectIndex0(Simd::Vec1::Mod(Simd::Vec1::Splat(value), Simd::Vec1::Splat(divisor)));
    }

    AZ_MATH_INLINE float Wrap(float value, float maxValue)
    {
        return Simd::Vec1::SelectIndex0(Simd::Vec1::Wrap(Simd::Vec1::Splat(value), Simd::Vec1::ZeroFloat(), Simd::Vec1::Splat(maxValue)));
    }

    AZ_MATH_INLINE float Wrap(float value, float minValue, float maxValue)
    {
        return Simd::Vec1::SelectIndex0(Simd::Vec1::Wrap(Simd::Vec1::Splat(value), Simd::Vec1::Splat(minValue), Simd::Vec1::Splat(maxValue)));
    }

    AZ_MATH_INLINE float AngleMod(float value)
    {
        return Simd::Vec1::SelectIndex0(Simd::Vec1::AngleMod(Simd::Vec1::Splat(value)));
    }

    AZ_MATH_INLINE void SinCos(float angle, float& sin, float& cos)
    {
        const Simd::Vec2::FloatType values = Simd::Vec2::SinCos(Simd::Vec1::Splat(angle));
        sin = Simd::Vec2::SelectIndex0(values);
        cos = Simd::Vec2::SelectIndex1(values);
    }

    AZ_MATH_INLINE float Sin(float angle)
    {
        return Simd::Vec1::SelectIndex0(Simd::Vec1::Sin(Simd::Vec1::Splat(angle)));
    }

    AZ_MATH_INLINE float Cos(float angle)
    {
        return Simd::Vec1::SelectIndex0(Simd::Vec1::Cos(Simd::Vec1::Splat(angle)));
    }

    AZ_MATH_INLINE float Acos(float value)
    {
        return Simd::Vec1::SelectIndex0(Simd::Vec1::Acos(Simd::Vec1::Splat(value)));
    }

    AZ_MATH_INLINE float Atan(float value)
    {
        return Simd::Vec1::SelectIndex0(Simd::Vec1::Atan(Simd::Vec1::Splat(value)));
    }

    AZ_MATH_INLINE float Atan2(float y, float x)
    {
        return Simd::Vec1::SelectIndex0(Simd::Vec1::Atan2(Simd::Vec1::Splat(y), Simd::Vec1::Splat(x)));
    }

    AZ_MATH_INLINE float Sqrt(float value)
    {
        return Simd::Vec1::SelectIndex0(Simd::Vec1::Sqrt(Simd::Vec1::Splat(value)));
    }

    AZ_MATH_INLINE float InvSqrt(float value)
    {
        return Simd::Vec1::SelectIndex0(Simd::Vec1::SqrtInv(Simd::Vec1::Splat(value)));
    }
}
