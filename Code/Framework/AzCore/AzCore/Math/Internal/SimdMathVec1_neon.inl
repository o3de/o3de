/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Internal/SimdMathCommon_neon.inl>
#include <AzCore/Math/Internal/SimdMathCommon_neonDouble.inl>
#include <AzCore/Math/Internal/SimdMathCommon_simd.inl>

namespace AZ
{
    namespace Simd
    {
        AZ_MATH_INLINE Vec1::FloatType Vec1::LoadAligned(const float* __restrict addr)
        {
            return NeonDouble::LoadAligned(addr);
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::LoadAligned(const int32_t* __restrict addr)
        {
            return NeonDouble::LoadAligned(addr);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::LoadUnaligned(const float* __restrict addr)
        {
            return NeonDouble::LoadUnaligned(addr);
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::LoadUnaligned(const int32_t* __restrict addr)
        {
            return NeonDouble::LoadUnaligned(addr);
        }

        AZ_MATH_INLINE void Vec1::StoreAligned(float* __restrict addr, FloatArgType value)
        {
            NeonDouble::StoreAligned(addr, value);
        }

        AZ_MATH_INLINE void Vec1::StoreAligned(int32_t* __restrict addr, Int32ArgType value)
        {
            NeonDouble::StoreAligned(addr, value);
        }

        AZ_MATH_INLINE void Vec1::StoreUnaligned(float* __restrict addr, FloatArgType value)
        {
            NeonDouble::StoreUnaligned(addr, value);
        }

        AZ_MATH_INLINE void Vec1::StoreUnaligned(int32_t* __restrict addr, Int32ArgType value)
        {
            NeonDouble::StoreUnaligned(addr, value);
        }

        AZ_MATH_INLINE void Vec1::StreamAligned(float* __restrict addr, FloatArgType value)
        {
            NeonDouble::StreamAligned(addr, value);
        }

        AZ_MATH_INLINE void Vec1::StreamAligned(int32_t* __restrict addr, Int32ArgType value)
        {
            NeonDouble::StreamAligned(addr, value);
        }

        AZ_MATH_INLINE float Vec1::SelectIndex0(FloatArgType value)
        {
            return NeonDouble::SelectIndex0(value);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Splat(float value)
        {
            return NeonDouble::Splat(value);
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::Splat(int32_t value)
        {
            return NeonDouble::Splat(value);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::LoadImmediate(float x)
        {
            return NeonDouble::LoadImmediate(x, 0.0f);
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::LoadImmediate(int32_t x)
        {
            return NeonDouble::LoadImmediate(x, 0);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Add(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonDouble::Add(arg1, arg2);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Sub(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonDouble::Sub(arg1, arg2);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Mul(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonDouble::Mul(arg1, arg2);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Madd(FloatArgType mul1, FloatArgType mul2, FloatArgType add)
        {
            return NeonDouble::Madd(mul1, mul2, add);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Div(FloatArgType arg1, FloatArgType arg2)
        {
            // In Vec1 the second element can be zero, avoid doing division by zero
            arg2 = NeonDouble::ReplaceIndex1(arg2, 1.0f);
            return NeonDouble::Div(arg1, arg2);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Abs(FloatArgType value)
        {
            return NeonDouble::Abs(value);
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::Add(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonDouble::Add(arg1, arg2);
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::Sub(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonDouble::Sub(arg1, arg2);
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::Mul(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonDouble::Mul(arg1, arg2);
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::Madd(Int32ArgType mul1, Int32ArgType mul2, Int32ArgType add)
        {
            return NeonDouble::Madd(mul1, mul2, add);
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::Abs(Int32ArgType value)
        {
            return NeonDouble::Abs(value);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Not(FloatArgType value)
        {
            return NeonDouble::Not(value);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::And(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonDouble::And(arg1, arg2);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::AndNot(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonDouble::AndNot(arg1, arg2);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Or(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonDouble::Or(arg1, arg2);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Xor(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonDouble::Xor(arg1, arg2);
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::Not(Int32ArgType value)
        {
            return NeonDouble::Not(value);
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::And(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonDouble::And(arg1, arg2);
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::AndNot(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonDouble::AndNot(arg1, arg2);
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::Or(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonDouble::Or(arg1, arg2);
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::Xor(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonDouble::Xor(arg1, arg2);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Floor(FloatArgType value)
        {
            return NeonDouble::Floor(value);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Ceil(FloatArgType value)
        {
            return NeonDouble::Ceil(value);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Round(FloatArgType value)
        {
            return NeonDouble::Round(value);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Truncate(FloatArgType value)
        {
            return NeonDouble::Truncate(value);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Min(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonDouble::Min(arg1, arg2);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Max(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonDouble::Max(arg1, arg2);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Clamp(FloatArgType value, FloatArgType min, FloatArgType max)
        {
            return NeonDouble::Clamp(value, min, max);
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::Min(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonDouble::Min(arg1, arg2);
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::Max(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonDouble::Max(arg1, arg2);
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::Clamp(Int32ArgType value, Int32ArgType min, Int32ArgType max)
        {
            return NeonDouble::Clamp(value, min, max);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::CmpEq(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonDouble::CmpEq(arg1, arg2);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::CmpNeq(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonDouble::CmpNeq(arg1, arg2);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::CmpGt(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonDouble::CmpGt(arg1, arg2);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::CmpGtEq(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonDouble::CmpGtEq(arg1, arg2);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::CmpLt(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonDouble::CmpLt(arg1, arg2);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::CmpLtEq(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonDouble::CmpLtEq(arg1, arg2);
        }

        AZ_MATH_INLINE bool Vec1::CmpAllEq(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonDouble::CmpAllEq(arg1, arg2);
        }

        AZ_MATH_INLINE bool Vec1::CmpAllLt(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonDouble::CmpAllLt(arg1, arg2);
        }

        AZ_MATH_INLINE bool Vec1::CmpAllLtEq(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonDouble::CmpAllLtEq(arg1, arg2);
        }

        AZ_MATH_INLINE bool Vec1::CmpAllGt(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonDouble::CmpAllGt(arg1, arg2);
        }

        AZ_MATH_INLINE bool Vec1::CmpAllGtEq(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonDouble::CmpAllGtEq(arg1, arg2);
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::CmpEq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonDouble::CmpEq(arg1, arg2);
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::CmpNeq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonDouble::CmpNeq(arg1, arg2);
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::CmpGt(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonDouble::CmpGt(arg1, arg2);
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::CmpGtEq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonDouble::CmpGtEq(arg1, arg2);
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::CmpLt(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonDouble::CmpLt(arg1, arg2);
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::CmpLtEq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonDouble::CmpLtEq(arg1, arg2);
        }

        AZ_MATH_INLINE bool Vec1::CmpAllEq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonDouble::CmpAllEq(arg1, arg2);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Select(FloatArgType arg1, FloatArgType arg2, FloatArgType mask)
        {
            return NeonDouble::Select(arg1, arg2, mask);
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::Select(Int32ArgType arg1, Int32ArgType arg2, Int32ArgType mask)
        {
            return NeonDouble::Select(arg1, arg2, mask);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Reciprocal(FloatArgType value)
        {
            // In Vec1 the second element can be garbage or 0.
            // Using (value.x, 1) to avoid divisions by 0.
            return NeonDouble::Reciprocal(
                NeonDouble::ReplaceIndex1(value, 1.0f));
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::ReciprocalEstimate(FloatArgType value)
        {
            // In Vec1 the second element can be garbage or 0.
            // Using (value.x, 1) to avoid divisions by 0.
            return NeonDouble::ReciprocalEstimate(
                NeonDouble::ReplaceIndex1(value, 1.0f));
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Mod(FloatArgType value, FloatArgType divisor)
        {
            return NeonDouble::Mod(value, divisor);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Wrap(FloatArgType value, FloatArgType minValue, FloatArgType maxValue)
        {
            return Common::Wrap<Vec1>(value, minValue, maxValue);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::AngleMod(FloatArgType value)
        {
            return Common::AngleMod<Vec1>(value);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Sqrt(FloatArgType value)
        {
            return NeonDouble::Sqrt(value);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::SqrtEstimate(FloatArgType value)
        {
            return NeonDouble::SqrtEstimate(value);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::SqrtInv(FloatArgType value)
        {
            return NeonDouble::SqrtInv(value);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::SqrtInvEstimate(FloatArgType value)
        {
            return NeonDouble::SqrtInvEstimate(value);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Sin(FloatArgType value)
        {
            return Common::Sin<Vec1>(value);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Cos(FloatArgType value)
        {
            return Common::Cos<Vec1>(value);
        }

        AZ_MATH_INLINE void Vec1::SinCos(FloatArgType value, FloatType& sin, FloatType& cos)
        {
            Common::SinCos<Vec1>(value, sin, cos);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Acos(FloatArgType value)
        {
            return Common::Acos<Vec1>(value);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Atan(FloatArgType value)
        {
            return Common::Atan<Vec1>(value);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Atan2(FloatArgType y, FloatArgType x)
        {
            return Common::Atan2<Vec1>(y, x);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::ExpEstimate(FloatArgType x)
        {
            return Common::ExpEstimate<Vec1>(x);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::ConvertToFloat(Int32ArgType value)
        {
            return NeonDouble::ConvertToFloat(value);
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::ConvertToInt(FloatArgType value)
        {
            return NeonDouble::ConvertToInt(value);
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::ConvertToIntNearest(FloatArgType value)
        {
            return NeonDouble::ConvertToIntNearest(value);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::CastToFloat(Int32ArgType value)
        {
            return NeonDouble::CastToFloat(value);
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::CastToInt(FloatArgType value)
        {
            return NeonDouble::CastToInt(value);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::ZeroFloat()
        {
            return NeonDouble::ZeroFloat();
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::ZeroInt()
        {
            return NeonDouble::ZeroInt();
        }
    }
}
