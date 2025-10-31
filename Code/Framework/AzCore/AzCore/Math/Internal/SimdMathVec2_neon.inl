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
        AZ_MATH_INLINE Vec1::FloatType Vec2::ToVec1(FloatArgType value)
        {
            return value;
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::FromVec1(Vec1::FloatArgType value)
        {
            // Coming from a Vec1 the last element could be garbage.
            return NeonDouble::SplatIndex0(value); // {value.x, value.x}
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::LoadAligned(const float* __restrict addr)
        {
            return NeonDouble::LoadAligned(addr);
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::LoadAligned(const int32_t* __restrict addr)
        {
            return NeonDouble::LoadAligned(addr);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::LoadUnaligned(const float* __restrict addr)
        {
            return NeonDouble::LoadUnaligned(addr);
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::LoadUnaligned(const int32_t* __restrict addr)
        {
            return NeonDouble::LoadUnaligned(addr);
        }

        AZ_MATH_INLINE void Vec2::StoreAligned(float* __restrict addr, FloatArgType value)
        {
            NeonDouble::StoreAligned(addr, value);
        }

        AZ_MATH_INLINE void Vec2::StoreAligned(int32_t* __restrict addr, Int32ArgType value)
        {
            NeonDouble::StoreAligned(addr, value);
        }

        AZ_MATH_INLINE void Vec2::StoreUnaligned(float* __restrict addr, FloatArgType value)
        {
            NeonDouble::StoreUnaligned(addr, value);
        }

        AZ_MATH_INLINE void Vec2::StoreUnaligned(int32_t* __restrict addr, Int32ArgType value)
        {
            NeonDouble::StoreUnaligned(addr, value);
        }

        AZ_MATH_INLINE void Vec2::StreamAligned(float* __restrict addr, FloatArgType value)
        {
            NeonDouble::StreamAligned(addr, value);
        }

        AZ_MATH_INLINE void Vec2::StreamAligned(int32_t* __restrict addr, Int32ArgType value)
        {
            NeonDouble::StreamAligned(addr, value);
        }

        AZ_MATH_INLINE float Vec2::SelectIndex0(FloatArgType value)
        {
            return NeonDouble::SelectIndex0(value);
        }

        AZ_MATH_INLINE float Vec2::SelectIndex1(FloatArgType value)
        {
            return NeonDouble::SelectIndex1(value);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Splat(float value)
        {
            return NeonDouble::Splat(value);
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::Splat(int32_t value)
        {
            return NeonDouble::Splat(value);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::SplatIndex0(FloatArgType value)
        {
            return NeonDouble::SplatIndex0(value);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::SplatIndex1(FloatArgType value)
        {
            return NeonDouble::SplatIndex1(value);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::ReplaceIndex0(FloatArgType a, float b)
        {
            return NeonDouble::ReplaceIndex0(a, b);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::ReplaceIndex0(FloatArgType a, FloatArgType b)
        {
            return NeonDouble::ReplaceIndex0(a, b);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::ReplaceIndex1(FloatArgType a, float b)
        {
            return NeonDouble::ReplaceIndex1(a, b);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::ReplaceIndex1(FloatArgType a, FloatArgType b)
        {
            return NeonDouble::ReplaceIndex1(a, b);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::LoadImmediate(float x, float y)
        {
            return NeonDouble::LoadImmediate(x, y);
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::LoadImmediate(int32_t x, int32_t y)
        {
            return NeonDouble::LoadImmediate(x, y);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Add(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonDouble::Add(arg1, arg2);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Sub(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonDouble::Sub(arg1, arg2);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Mul(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonDouble::Mul(arg1, arg2);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Madd(FloatArgType mul1, FloatArgType mul2, FloatArgType add)
        {
            return NeonDouble::Madd(mul1, mul2, add);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Div(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonDouble::Div(arg1, arg2);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Abs(FloatArgType value)
        {
            return NeonDouble::Abs(value);
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::Add(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonDouble::Add(arg1, arg2);
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::Sub(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonDouble::Sub(arg1, arg2);
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::Mul(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonDouble::Mul(arg1, arg2);
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::Madd(Int32ArgType mul1, Int32ArgType mul2, Int32ArgType add)
        {
            return NeonDouble::Madd(mul1, mul2, add);
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::Abs(Int32ArgType value)
        {
            return NeonDouble::Abs(value);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Not(FloatArgType value)
        {
            return NeonDouble::Not(value);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::And(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonDouble::And(arg1, arg2);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::AndNot(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonDouble::AndNot(arg1, arg2);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Or(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonDouble::Or(arg1, arg2);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Xor(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonDouble::Xor(arg1, arg2);
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::Not(Int32ArgType value)
        {
            return NeonDouble::Not(value);
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::And(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonDouble::And(arg1, arg2);
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::AndNot(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonDouble::AndNot(arg1, arg2);
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::Or(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonDouble::Or(arg1, arg2);
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::Xor(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonDouble::Xor(arg1, arg2);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Floor(FloatArgType value)
        {
            return NeonDouble::Floor(value);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Ceil(FloatArgType value)
        {
            return NeonDouble::Ceil(value);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Round(FloatArgType value)
        {
            return NeonDouble::Round(value);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Truncate(FloatArgType value)
        {
            return NeonDouble::Truncate(value);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Min(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonDouble::Min(arg1, arg2);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Max(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonDouble::Max(arg1, arg2);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Clamp(FloatArgType value, FloatArgType min, FloatArgType max)
        {
            return NeonDouble::Clamp(value, min, max);
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::Min(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonDouble::Min(arg1, arg2);
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::Max(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonDouble::Max(arg1, arg2);
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::Clamp(Int32ArgType value, Int32ArgType min, Int32ArgType max)
        {
            return NeonDouble::Clamp(value, min, max);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::CmpEq(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonDouble::CmpEq(arg1, arg2);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::CmpNeq(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonDouble::CmpNeq(arg1, arg2);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::CmpGt(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonDouble::CmpGt(arg1, arg2);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::CmpGtEq(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonDouble::CmpGtEq(arg1, arg2);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::CmpLt(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonDouble::CmpLt(arg1, arg2);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::CmpLtEq(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonDouble::CmpLtEq(arg1, arg2);
        }

        AZ_MATH_INLINE bool Vec2::CmpAllEq(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonDouble::CmpAllEq(arg1, arg2);
        }

        AZ_MATH_INLINE bool Vec2::CmpAllLt(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonDouble::CmpAllLt(arg1, arg2);
        }

        AZ_MATH_INLINE bool Vec2::CmpAllLtEq(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonDouble::CmpAllLtEq(arg1, arg2);
        }

        AZ_MATH_INLINE bool Vec2::CmpAllGt(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonDouble::CmpAllGt(arg1, arg2);
        }

        AZ_MATH_INLINE bool Vec2::CmpAllGtEq(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonDouble::CmpAllGtEq(arg1, arg2);
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::CmpEq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonDouble::CmpEq(arg1, arg2);
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::CmpNeq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonDouble::CmpNeq(arg1, arg2);
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::CmpGt(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonDouble::CmpGt(arg1, arg2);
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::CmpGtEq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonDouble::CmpGtEq(arg1, arg2);
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::CmpLt(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonDouble::CmpLt(arg1, arg2);
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::CmpLtEq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonDouble::CmpLtEq(arg1, arg2);
        }

        AZ_MATH_INLINE bool Vec2::CmpAllEq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonDouble::CmpAllEq(arg1, arg2);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Select(FloatArgType arg1, FloatArgType arg2, FloatArgType mask)
        {
            return NeonDouble::Select(arg1, arg2, mask);
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::Select(Int32ArgType arg1, Int32ArgType arg2, Int32ArgType mask)
        {
            return NeonDouble::Select(arg1, arg2, mask);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Reciprocal(FloatArgType value)
        {
            return NeonDouble::Reciprocal(value);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::ReciprocalEstimate(FloatArgType value)
        {
            return NeonDouble::ReciprocalEstimate(value);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Mod(FloatArgType value, FloatArgType divisor)
        {
            return NeonDouble::Mod(value, divisor);
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
            return NeonDouble::Sqrt(value);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::SqrtEstimate(FloatArgType value)
        {
            return NeonDouble::SqrtEstimate(value);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::SqrtInv(FloatArgType value)
        {
            return NeonDouble::SqrtInv(value);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::SqrtInvEstimate(FloatArgType value)
        {
            return NeonDouble::SqrtInvEstimate(value);
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
            return Common::Atan2<Vec1>(Vec1::Splat(SelectIndex1(value)), Vec1::Splat(SelectIndex0(value)));
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::ExpEstimate(FloatArgType x)
        {
            return Common::ExpEstimate<Vec2>(x);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec2::Dot(FloatArgType arg1, FloatArgType arg2)
        {
            return Vec1::LoadImmediate(vaddv_f32(vmul_f32(arg1, arg2)));
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
            return NeonDouble::ConvertToFloat(value);
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::ConvertToInt(FloatArgType value)
        {
            return NeonDouble::ConvertToInt(value);
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::ConvertToIntNearest(FloatArgType value)
        {
            return NeonDouble::ConvertToIntNearest(value);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::CastToFloat(Int32ArgType value)
        {
            return NeonDouble::CastToFloat(value);
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::CastToInt(FloatArgType value)
        {
            return NeonDouble::CastToInt(value);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::ZeroFloat()
        {
            return NeonDouble::ZeroFloat();
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::ZeroInt()
        {
            return NeonDouble::ZeroInt();
        }
    }
}
