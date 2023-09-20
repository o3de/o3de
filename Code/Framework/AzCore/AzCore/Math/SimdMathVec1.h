/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Internal/MathTypes.h>
namespace AZ
{
    namespace Simd
    {
        struct Vec1
        {
            static constexpr int32_t ElementCount = 1;

#if   AZ_TRAIT_USE_PLATFORM_SIMD_SSE
            using FloatType = __m128;
            using Int32Type = __m128i;
            using FloatArgType = FloatType;
            using Int32ArgType = Int32Type;
#elif AZ_TRAIT_USE_PLATFORM_SIMD_NEON
            using FloatType = float32x2_t;
            using Int32Type = int32x2_t;
            using FloatArgType = FloatType;
            using Int32ArgType = Int32Type;
#elif AZ_TRAIT_USE_PLATFORM_SIMD_SCALAR
            using FloatType = float;
            using Int32Type = int32_t;
            using FloatArgType = float;
            using Int32ArgType = int32_t;
#endif

            static FloatType LoadAligned(const float* __restrict addr); // addr *must* be 16-byte aligned
            static Int32Type LoadAligned(const int32_t* __restrict addr); // addr *must* be 16-byte aligned
            static FloatType LoadUnaligned(const float* __restrict addr);
            static Int32Type LoadUnaligned(const int32_t* __restrict addr);

            static void StoreAligned(float* __restrict addr, FloatArgType value); // addr *must* be 16-byte aligned
            static void StoreAligned(int32_t* __restrict addr, Int32ArgType value); // addr *must* be 16-byte aligned
            static void StoreUnaligned(float* __restrict addr, FloatArgType value);
            static void StoreUnaligned(int32_t* __restrict addr, Int32ArgType value);

            static void StreamAligned(float* __restrict addr, FloatArgType value); // addr *must* be 16-byte aligned
            static void StreamAligned(int32_t* __restrict addr, Int32ArgType value); // addr *must* be 16-byte aligned

            static float SelectIndex0(FloatArgType value);
            static float AZ_MATH_INLINE SelectFirst(FloatArgType value) { return SelectIndex0(value); } // O3DE_DEPRECATION_NOTICE(PR-16251)

            static FloatType Splat(float value);
            static Int32Type Splat(int32_t value);

            static FloatType LoadImmediate(float x);
            static Int32Type LoadImmediate(int32_t x);

            static FloatType Add(FloatArgType arg1, FloatArgType arg2);
            static FloatType Sub(FloatArgType arg1, FloatArgType arg2);
            static FloatType Mul(FloatArgType arg1, FloatArgType arg2);
            static FloatType Madd(FloatArgType mul1, FloatArgType mul2, FloatArgType add);
            static FloatType Div(FloatArgType arg1, FloatArgType arg2);
            static FloatType Abs(FloatArgType value);

            static Int32Type Add(Int32ArgType arg1, Int32ArgType arg2);
            static Int32Type Sub(Int32ArgType arg1, Int32ArgType arg2);
            static Int32Type Mul(Int32ArgType arg1, Int32ArgType arg2);
            static Int32Type Madd(Int32ArgType mul1, Int32ArgType mul2, Int32ArgType add);
            static Int32Type Abs(Int32ArgType value);

            static FloatType Not(FloatArgType value);
            static FloatType And(FloatArgType arg1, FloatArgType arg2);
            static FloatType AndNot(FloatArgType arg1, FloatArgType arg2);
            static FloatType Or(FloatArgType arg1, FloatArgType arg2);
            static FloatType Xor(FloatArgType arg1, FloatArgType arg2);

            static Int32Type Not(Int32ArgType value);
            static Int32Type And(Int32ArgType arg1, Int32ArgType arg2);
            static Int32Type AndNot(Int32ArgType arg1, Int32ArgType arg2);
            static Int32Type Or(Int32ArgType arg1, Int32ArgType arg2);
            static Int32Type Xor(Int32ArgType arg1, Int32ArgType arg2);

            static FloatType Floor(FloatArgType value);
            static FloatType Ceil(FloatArgType value);
            static FloatType Round(FloatArgType value); // Ties to even (banker's rounding)
            static FloatType Truncate(FloatArgType value);
            static FloatType Min(FloatArgType arg1, FloatArgType arg2);
            static FloatType Max(FloatArgType arg1, FloatArgType arg2);
            static FloatType Clamp(FloatArgType value, FloatArgType min, FloatArgType max);

            static Int32Type Min(Int32ArgType arg1, Int32ArgType arg2);
            static Int32Type Max(Int32ArgType arg1, Int32ArgType arg2);
            static Int32Type Clamp(Int32ArgType value, Int32ArgType min, Int32ArgType max);

            static FloatType CmpEq(FloatArgType arg1, FloatArgType arg2);
            static FloatType CmpNeq(FloatArgType arg1, FloatArgType arg2);
            static FloatType CmpGt(FloatArgType arg1, FloatArgType arg2);
            static FloatType CmpGtEq(FloatArgType arg1, FloatArgType arg2);
            static FloatType CmpLt(FloatArgType arg1, FloatArgType arg2);
            static FloatType CmpLtEq(FloatArgType arg1, FloatArgType arg2);

            static bool CmpAllEq(FloatArgType arg1, FloatArgType arg2);
            static bool CmpAllLt(FloatArgType arg1, FloatArgType arg2);
            static bool CmpAllLtEq(FloatArgType arg1, FloatArgType arg2);
            static bool CmpAllGt(FloatArgType arg1, FloatArgType arg2);
            static bool CmpAllGtEq(FloatArgType arg1, FloatArgType arg2);

            static Int32Type CmpEq(Int32ArgType arg1, Int32ArgType arg2);
            static Int32Type CmpNeq(Int32ArgType arg1, Int32ArgType arg2);
            static Int32Type CmpGt(Int32ArgType arg1, Int32ArgType arg2);
            static Int32Type CmpGtEq(Int32ArgType arg1, Int32ArgType arg2);
            static Int32Type CmpLt(Int32ArgType arg1, Int32ArgType arg2);
            static Int32Type CmpLtEq(Int32ArgType arg1, Int32ArgType arg2);

            static bool CmpAllEq(Int32ArgType arg1, Int32ArgType arg2);

            static FloatType Select(FloatArgType arg1, FloatArgType arg2, FloatArgType mask);
            static Int32Type Select(Int32ArgType arg1, Int32ArgType arg2, Int32ArgType mask);

            static FloatType Reciprocal(FloatArgType value); // Slow, but full accuracy
            static FloatType ReciprocalEstimate(FloatArgType value); // Fastest, but roughly half precision on supported platforms

            //! Returns the floating point remainder of value / divisor.
            static FloatType Mod(FloatArgType value, FloatArgType divisor);

            //! Wraps [minValue, maxValue].
            static FloatType Wrap(FloatArgType value, FloatArgType minValue, FloatArgType maxValue);

            //! Wraps the angle into the [-pi, pi] range.
            static FloatType AngleMod(FloatArgType value);

            static FloatType Sqrt(FloatArgType value); // Slow, but full accuracy
            static FloatType SqrtEstimate(FloatArgType value); // Fastest, but roughly half precision on supported platforms
            static FloatType SqrtInv(FloatArgType value); // Slow, but full accuracy
            static FloatType SqrtInvEstimate(FloatArgType value); // Fastest, but roughly half precision on supported platforms

            static FloatType Sin(FloatArgType value);
            static FloatType Cos(FloatArgType value);
            static void SinCos(FloatArgType value, FloatType& sin, FloatType& cos);
            static FloatType Acos(FloatArgType value);
            static FloatType Atan(FloatArgType value);
            static FloatType Atan2(FloatArgType y, FloatArgType x);
            static FloatType ExpEstimate(FloatArgType x);

            static FloatType ConvertToFloat(Int32ArgType value);
            static Int32Type ConvertToInt(FloatArgType value); // Truncates
            static Int32Type ConvertToIntNearest(FloatArgType value); // Ronuds to nearest int with ties to even (banker's rounding)

            static FloatType CastToFloat(Int32ArgType value);
            static Int32Type CastToInt(FloatArgType value);

            static FloatType ZeroFloat();
            static Int32Type ZeroInt();
        };
    }
}


#if   AZ_TRAIT_USE_PLATFORM_SIMD_SSE
#   include <AzCore/Math/Internal/SimdMathVec1_sse.inl>
#elif AZ_TRAIT_USE_PLATFORM_SIMD_NEON
#   include <AzCore/Math/Internal/SimdMathVec1_neon.inl>
#elif AZ_TRAIT_USE_PLATFORM_SIMD_SCALAR
#   include <AzCore/Math/Internal/SimdMathVec1_scalar.inl>
#endif
