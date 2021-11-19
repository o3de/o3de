/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Internal/SimdMathCommon_sse.inl>
#include <AzCore/Math/Internal/SimdMathCommon_simd.inl>

namespace AZ
{
    namespace Simd
    {
        AZ_MATH_INLINE Vec1::FloatType Vec2::ToVec1(FloatArgType value)
        {
            return value;
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::FromVec1(Vec1::FloatArgType value)
        {
            return value;
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::LoadAligned(const float* __restrict addr)
        {
            return Sse::LoadAligned(addr);
        }


        AZ_MATH_INLINE Vec2::Int32Type Vec2::LoadAligned(const int32_t* __restrict addr)
        {
            return Sse::LoadAligned(addr);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::LoadUnaligned(const float* __restrict addr)
        {
            return Sse::LoadUnaligned(addr);
        }


        AZ_MATH_INLINE Vec2::Int32Type Vec2::LoadUnaligned(const int32_t* __restrict addr)
        {
            return Sse::LoadUnaligned(addr);
        }


        AZ_MATH_INLINE void Vec2::StoreAligned(float* __restrict addr, FloatArgType value)
        {
            return Sse::StoreAligned(addr, value);
        }


        AZ_MATH_INLINE void Vec2::StoreAligned(int32_t* __restrict addr, Int32ArgType value)
        {
            return Sse::StoreAligned(addr, value);
        }


        AZ_MATH_INLINE void Vec2::StoreUnaligned(float* __restrict addr, FloatArgType value)
        {
            return Sse::StoreUnaligned(addr, value);
        }


        AZ_MATH_INLINE void Vec2::StoreUnaligned(int32_t* __restrict addr, Int32ArgType value)
        {
            return Sse::StoreUnaligned(addr, value);
        }


        AZ_MATH_INLINE void Vec2::StreamAligned(float* __restrict addr, FloatArgType value)
        {
            return Sse::StreamAligned(addr, value);
        }


        AZ_MATH_INLINE void Vec2::StreamAligned(int32_t* __restrict addr, Int32ArgType value)
        {
            return Sse::StreamAligned(addr, value);
        }


        AZ_MATH_INLINE float Vec2::SelectFirst(FloatArgType value)
        {
            return Sse::SelectFirst(value);
        }


        AZ_MATH_INLINE float Vec2::SelectSecond(FloatArgType value)
        {
            return Sse::SelectFirst(Sse::SplatSecond(value));
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::Splat(float value)
        {
            return Sse::Splat(value);
        }


        AZ_MATH_INLINE Vec2::Int32Type Vec2::Splat(int32_t value)
        {
            return Sse::Splat(value);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::SplatFirst(FloatArgType value)
        {
            return Sse::SplatFirst(value);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::SplatSecond(FloatArgType value)
        {
            return Sse::SplatSecond(value);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::ReplaceFirst(FloatArgType a, float b)
        {
            return Sse::ReplaceFirst(a, b);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::ReplaceFirst(FloatArgType a, FloatArgType b)
        {
            return Sse::ReplaceFirst(a, b);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::ReplaceSecond(FloatArgType a, float b)
        {
            return Sse::ReplaceSecond(a, b);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::ReplaceSecond(FloatArgType a, FloatArgType b)
        {
            return Sse::ReplaceSecond(a, b);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::LoadImmediate(float x, float y)
        {
            return Sse::LoadImmediate(x, y, 0.0f, 0.0f);
        }


        AZ_MATH_INLINE Vec2::Int32Type Vec2::LoadImmediate(int32_t x, int32_t y)
        {
            return Sse::LoadImmediate(x, y, 0, 0);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::Add(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::Add(arg1, arg2);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::Sub(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::Sub(arg1, arg2);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::Mul(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::Mul(arg1, arg2);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::Madd(FloatArgType mul1, FloatArgType mul2, FloatArgType add)
        {
            return Sse::Madd(mul1, mul2, add);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::Div(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::Div(arg1, arg2);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::Abs(FloatArgType value)
        {
            return Sse::Abs(value);
        }


        AZ_MATH_INLINE Vec2::Int32Type Vec2::Add(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::Add(arg1, arg2);
        }


        AZ_MATH_INLINE Vec2::Int32Type Vec2::Sub(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::Sub(arg1, arg2);
        }


        AZ_MATH_INLINE Vec2::Int32Type Vec2::Mul(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::Mul(arg1, arg2);
        }


        AZ_MATH_INLINE Vec2::Int32Type Vec2::Madd(Int32ArgType mul1, Int32ArgType mul2, Int32ArgType add)
        {
            return Sse::Madd(mul1, mul2, add);
        }


        AZ_MATH_INLINE Vec2::Int32Type Vec2::Abs(Int32ArgType value)
        {
            return Sse::Abs(value);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::Not(FloatArgType value)
        {
            return Sse::Not(value);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::And(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::And(arg1, arg2);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::AndNot(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::AndNot(arg1, arg2);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::Or(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::Or(arg1, arg2);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::Xor(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::Xor(arg1, arg2);
        }


        AZ_MATH_INLINE Vec2::Int32Type Vec2::Not(Int32ArgType value)
        {
            return Sse::Not(value);
        }


        AZ_MATH_INLINE Vec2::Int32Type Vec2::And(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::And(arg1, arg2);
        }


        AZ_MATH_INLINE Vec2::Int32Type Vec2::AndNot(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::AndNot(arg1, arg2);
        }


        AZ_MATH_INLINE Vec2::Int32Type Vec2::Or(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::Or(arg1, arg2);
        }


        AZ_MATH_INLINE Vec2::Int32Type Vec2::Xor(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::Xor(arg1, arg2);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::Floor(FloatArgType value)
        {
            return Sse::Floor(value);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::Ceil(FloatArgType value)
        {
            return Sse::Ceil(value);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::Round(FloatArgType value)
        {
            return Sse::Round(value);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::Truncate(FloatArgType value)
        {
            return Sse::Truncate(value);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::Min(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::Min(arg1, arg2);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::Max(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::Max(arg1, arg2);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::Clamp(FloatArgType value, FloatArgType min, FloatArgType max)
        {
            return Sse::Clamp(value, min, max);
        }


        AZ_MATH_INLINE Vec2::Int32Type Vec2::Min(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::Min(arg1, arg2);
        }


        AZ_MATH_INLINE Vec2::Int32Type Vec2::Max(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::Max(arg1, arg2);
        }


        AZ_MATH_INLINE Vec2::Int32Type Vec2::Clamp(Int32ArgType value, Int32ArgType min, Int32ArgType max)
        {
            return Sse::Clamp(value, min, max);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::CmpEq(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::CmpEq(arg1, arg2);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::CmpNeq(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::CmpNeq(arg1, arg2);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::CmpGt(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::CmpGt(arg1, arg2);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::CmpGtEq(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::CmpGtEq(arg1, arg2);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::CmpLt(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::CmpLt(arg1, arg2);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::CmpLtEq(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::CmpLtEq(arg1, arg2);
        }


        AZ_MATH_INLINE bool Vec2::CmpAllEq(FloatArgType arg1, FloatArgType arg2)
        {
            // Only check the first two bits for Vector2
            return Sse::CmpAllEq(arg1, arg2, 0b0011);
        }


        AZ_MATH_INLINE bool Vec2::CmpAllLt(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::CmpAllLt(arg1, arg2, 0b0011);
        }


        AZ_MATH_INLINE bool Vec2::CmpAllLtEq(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::CmpAllLtEq(arg1, arg2, 0b0011);
        }


        AZ_MATH_INLINE bool Vec2::CmpAllGt(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::CmpAllGt(arg1, arg2, 0b0011);
        }


        AZ_MATH_INLINE bool Vec2::CmpAllGtEq(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::CmpAllGtEq(arg1, arg2, 0b0011);
        }


        AZ_MATH_INLINE Vec2::Int32Type Vec2::CmpEq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::CmpEq(arg1, arg2);
        }


        AZ_MATH_INLINE Vec2::Int32Type Vec2::CmpNeq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::CmpNeq(arg1, arg2);
        }


        AZ_MATH_INLINE Vec2::Int32Type Vec2::CmpGt(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::CmpGt(arg1, arg2);
        }


        AZ_MATH_INLINE Vec2::Int32Type Vec2::CmpGtEq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::CmpGtEq(arg1, arg2);
        }


        AZ_MATH_INLINE Vec2::Int32Type Vec2::CmpLt(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::CmpLt(arg1, arg2);
        }


        AZ_MATH_INLINE Vec2::Int32Type Vec2::CmpLtEq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::CmpLtEq(arg1, arg2);
        }


        AZ_MATH_INLINE bool Vec2::CmpAllEq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::CmpAllEq(arg1, arg2, 0x000F);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::Select(FloatArgType arg1, FloatArgType arg2, FloatArgType mask)
        {
            return Sse::Select(arg1, arg2, mask);
        }


        AZ_MATH_INLINE Vec2::Int32Type Vec2::Select(Int32ArgType arg1, Int32ArgType arg2, Int32ArgType mask)
        {
            return Sse::Select(arg1, arg2, mask);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::Reciprocal(FloatArgType value)
        {
            value = Sse::ReplaceFourth(Sse::ReplaceThird(value, 1.0f), 1.0f);
            return Sse::Reciprocal(value);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::ReciprocalEstimate(FloatArgType value)
        {
            return Sse::ReciprocalEstimate(value);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::Mod(FloatArgType value, FloatArgType divisor)
        {
            return Sse::Mod(value, divisor);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::Wrap(FloatArgType value, FloatArgType minValue, FloatArgType maxValue)
        {
            return Common::Wrap<Vec2>(value, minValue, maxValue);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::AngleMod(FloatArgType value)
        {
            return Common::AngleMod<Vec2>(value);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::Sqrt(FloatArgType value)
        {
            return Sse::Sqrt(value);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::SqrtEstimate(FloatArgType value)
        {
            return Sse::SqrtEstimate(value);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::SqrtInv(FloatArgType value)
        {
            value = Sse::ReplaceFourth(Sse::ReplaceThird(value, 1.0f), 1.0f);
            return Sse::SqrtInv(value);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::SqrtInvEstimate(FloatArgType value)
        {
            return Sse::SqrtInvEstimate(value);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::Sin(FloatArgType value)
        {
            return Common::Sin<Vec2>(value);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::Cos(FloatArgType value)
        {
            return Common::Cos<Vec2>(value);
        }


        AZ_MATH_INLINE void Vec2::SinCos(FloatArgType value, FloatType& sin, FloatType& cos)
        {
            Common::SinCos<Vec2>(value, sin, cos);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::SinCos(Vec1::FloatArgType angle)
        {
            const FloatType angleOffset = LoadImmediate(0.0f, Constants::HalfPi);
            const FloatType angles = Add(FromVec1(angle), angleOffset);
            return Sin(angles);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::Acos(FloatArgType value)
        {
            return Common::Acos<Vec2>(value);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::Atan(FloatArgType value)
        {
            return Common::Atan<Vec2>(value);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::Atan2(FloatArgType y, FloatArgType x)
        {
            return Common::Atan2<Vec2>(y, x);
        }


        AZ_MATH_INLINE Vec1::FloatType Vec2::Atan2(FloatArgType value)
        {
            return Common::Atan2<Vec1>(Vec1::Splat(SelectSecond(value)), Vec1::Splat(SelectFirst(value)));
        }


        AZ_MATH_INLINE Vec1::FloatType Vec2::Dot(FloatArgType arg1, FloatArgType arg2)
        {
#if 0
            return _mm_dp_ps(arg1, arg2, 0x33);
#else
            const FloatType x2 = Mul(arg1, arg2);
            return SplatFirst(Add(SplatSecond(x2), x2));
#endif
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::Normalize(FloatArgType value)
        {
            return Common::Normalize<Vec2>(value);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::NormalizeEstimate(FloatArgType value)
        {
            return Common::NormalizeEstimate<Vec2>(value);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::NormalizeSafe(FloatArgType value, float tolerance)
        {
            return Common::NormalizeSafe<Vec2>(value, tolerance);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::NormalizeSafeEstimate(FloatArgType value, float tolerance)
        {
            return Common::NormalizeSafeEstimate<Vec2>(value, tolerance);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::ConvertToFloat(Int32ArgType value)
        {
            return Sse::ConvertToFloat(value);
        }


        AZ_MATH_INLINE Vec2::Int32Type Vec2::ConvertToInt(FloatArgType value)
        {
            return Sse::ConvertToInt(value);
        }


        AZ_MATH_INLINE Vec2::Int32Type Vec2::ConvertToIntNearest(FloatArgType value)
        {
            return Sse::ConvertToIntNearest(value);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::CastToFloat(Int32ArgType value)
        {
            return Sse::CastToFloat(value);
        }


        AZ_MATH_INLINE Vec2::Int32Type Vec2::CastToInt(FloatArgType value)
        {
            return Sse::CastToInt(value);
        }


        AZ_MATH_INLINE Vec2::FloatType Vec2::ZeroFloat()
        {
            return Sse::ZeroFloat();
        }


        AZ_MATH_INLINE Vec2::Int32Type Vec2::ZeroInt()
        {
            return Sse::ZeroInt();
        }
    }
}
