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
        AZ_MATH_INLINE Vec1::FloatType Vec1::LoadAligned(const float* __restrict addr)
        {
            AZ_MATH_ASSERT(IsAligned<16>(addr), "Alignment failure");
            return _mm_load_ps1(addr);
        }


        AZ_MATH_INLINE Vec1::Int32Type Vec1::LoadAligned(const int32_t* __restrict addr)
        {
            return Sse::LoadAligned(addr);
        }


        AZ_MATH_INLINE Vec1::FloatType Vec1::LoadUnaligned(const float* __restrict addr)
        {
            return _mm_load_ps1(addr);
        }


        AZ_MATH_INLINE Vec1::Int32Type Vec1::LoadUnaligned(const int32_t* __restrict addr)
        {
            return Sse::LoadUnaligned(addr);
        }


        AZ_MATH_INLINE void Vec1::StoreAligned(float* __restrict addr, FloatArgType value)
        {
            AZ_MATH_ASSERT(IsAligned<16>(addr), "Alignment failure");
            _mm_store_ss(addr, value);
        }


        AZ_MATH_INLINE void Vec1::StoreAligned(int32_t* __restrict addr, Int32ArgType value)
        {
            Sse::StoreAligned(addr, value);
        }


        AZ_MATH_INLINE void Vec1::StoreUnaligned(float* __restrict addr, FloatArgType value)
        {
            _mm_store_ss(addr, value);
        }


        AZ_MATH_INLINE void Vec1::StoreUnaligned(int32_t* __restrict addr, Int32ArgType value)
        {
            Sse::StoreUnaligned(addr, value);
        }


        AZ_MATH_INLINE void Vec1::StreamAligned(float* __restrict addr, FloatArgType value)
        {
            Sse::StreamAligned(addr, value);
        }


        AZ_MATH_INLINE void Vec1::StreamAligned(int32_t* __restrict addr, Int32ArgType value)
        {
            Sse::StreamAligned(addr, value);
        }


        AZ_MATH_INLINE float Vec1::SelectFirst(FloatArgType value)
        {
            return Sse::SelectFirst(value);
        }


        AZ_MATH_INLINE Vec1::FloatType Vec1::Splat(float value)
        {
            return _mm_set_ps1(value);
        }


        AZ_MATH_INLINE Vec1::Int32Type Vec1::Splat(int32_t value)
        {
            return Sse::Splat(value);
        }


        AZ_MATH_INLINE Vec1::FloatType Vec1::LoadImmediate(float x)
        {
            return Sse::LoadImmediate(x, 0.0f, 0.0f, 0.0f);
        }


        AZ_MATH_INLINE Vec1::Int32Type Vec1::LoadImmediate(int32_t x)
        {
            return Sse::LoadImmediate(x, 0, 0, 0);
        }


        AZ_MATH_INLINE Vec1::FloatType Vec1::Add(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::Add(arg1, arg2);
        }


        AZ_MATH_INLINE Vec1::FloatType Vec1::Sub(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::Sub(arg1, arg2);
        }


        AZ_MATH_INLINE Vec1::FloatType Vec1::Mul(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::Mul(arg1, arg2);
        }


        AZ_MATH_INLINE Vec1::FloatType Vec1::Madd(FloatArgType mul1, FloatArgType mul2, FloatArgType add)
        {
            return Sse::Madd(mul1, mul2, add);
        }


        AZ_MATH_INLINE Vec1::FloatType Vec1::Div(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::Div(arg1, arg2);
        }


        AZ_MATH_INLINE Vec1::FloatType Vec1::Abs(FloatArgType value)
        {
            return Sse::Abs(value);
        }


        AZ_MATH_INLINE Vec1::Int32Type Vec1::Add(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::Add(arg1, arg2);
        }


        AZ_MATH_INLINE Vec1::Int32Type Vec1::Sub(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::Sub(arg1, arg2);
        }


        AZ_MATH_INLINE Vec1::Int32Type Vec1::Mul(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::Mul(arg1, arg2);
        }


        AZ_MATH_INLINE Vec1::Int32Type Vec1::Madd(Int32ArgType mul1, Int32ArgType mul2, Int32ArgType add)
        {
            return Sse::Madd(mul1, mul2, add);
        }


        AZ_MATH_INLINE Vec1::Int32Type Vec1::Abs(Int32ArgType value)
        {
            return Sse::Abs(value);
        }


        AZ_MATH_INLINE Vec1::FloatType Vec1::Not(FloatArgType value)
        {
            return Sse::Not(value);
        }


        AZ_MATH_INLINE Vec1::FloatType Vec1::And(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::And(arg1, arg2);
        }


        AZ_MATH_INLINE Vec1::FloatType Vec1::AndNot(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::AndNot(arg1, arg2);
        }


        AZ_MATH_INLINE Vec1::FloatType Vec1::Or(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::Or(arg1, arg2);
        }


        AZ_MATH_INLINE Vec1::FloatType Vec1::Xor(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::Xor(arg1, arg2);
        }


        AZ_MATH_INLINE Vec1::Int32Type Vec1::Not(Int32ArgType value)
        {
            return Sse::Not(value);
        }


        AZ_MATH_INLINE Vec1::Int32Type Vec1::And(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::And(arg1, arg2);
        }


        AZ_MATH_INLINE Vec1::Int32Type Vec1::AndNot(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::AndNot(arg1, arg2);
        }


        AZ_MATH_INLINE Vec1::Int32Type Vec1::Or(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::Or(arg1, arg2);
        }


        AZ_MATH_INLINE Vec1::Int32Type Vec1::Xor(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::Xor(arg1, arg2);
        }


        AZ_MATH_INLINE Vec1::FloatType Vec1::Floor(FloatArgType value)
        {
            return Sse::Floor(value);
        }


        AZ_MATH_INLINE Vec1::FloatType Vec1::Ceil(FloatArgType value)
        {
            return Sse::Ceil(value);
        }


        AZ_MATH_INLINE Vec1::FloatType Vec1::Round(FloatArgType value)
        {
            return Sse::Round(value);
        }


        AZ_MATH_INLINE Vec1::FloatType Vec1::Truncate(FloatArgType value)
        {
            return Sse::Truncate(value);
        }


        AZ_MATH_INLINE Vec1::FloatType Vec1::Min(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::Min(arg1, arg2);
        }


        AZ_MATH_INLINE Vec1::FloatType Vec1::Max(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::Max(arg1, arg2);
        }


        AZ_MATH_INLINE Vec1::FloatType Vec1::Clamp(FloatArgType value, FloatArgType min, FloatArgType max)
        {
            return Sse::Clamp(value, min, max);
        }


        AZ_MATH_INLINE Vec1::Int32Type Vec1::Min(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::Min(arg1, arg2);
        }


        AZ_MATH_INLINE Vec1::Int32Type Vec1::Max(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::Max(arg1, arg2);
        }


        AZ_MATH_INLINE Vec1::Int32Type Vec1::Clamp(Int32ArgType value, Int32ArgType min, Int32ArgType max)
        {
            return Sse::Clamp(value, min, max);
        }


        AZ_MATH_INLINE Vec1::FloatType Vec1::CmpEq(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::CmpEq(arg1, arg2);
        }


        AZ_MATH_INLINE Vec1::FloatType Vec1::CmpNeq(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::CmpNeq(arg1, arg2);
        }


        AZ_MATH_INLINE Vec1::FloatType Vec1::CmpGt(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::CmpGt(arg1, arg2);
        }


        AZ_MATH_INLINE Vec1::FloatType Vec1::CmpGtEq(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::CmpGtEq(arg1, arg2);
        }


        AZ_MATH_INLINE Vec1::FloatType Vec1::CmpLt(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::CmpLt(arg1, arg2);
        }


        AZ_MATH_INLINE Vec1::FloatType Vec1::CmpLtEq(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::CmpLtEq(arg1, arg2);
        }


        AZ_MATH_INLINE bool Vec1::CmpAllEq(FloatArgType arg1, FloatArgType arg2)
        {
            // Only check the first bit for Vector1
            return Sse::CmpAllEq(arg1, arg2, 0b0001);
        }


        AZ_MATH_INLINE bool Vec1::CmpAllLt(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::CmpAllLt(arg1, arg2, 0b0001);
        }


        AZ_MATH_INLINE bool Vec1::CmpAllLtEq(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::CmpAllLtEq(arg1, arg2, 0b0001);
        }


        AZ_MATH_INLINE bool Vec1::CmpAllGt(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::CmpAllGt(arg1, arg2, 0b0001);
        }


        AZ_MATH_INLINE bool Vec1::CmpAllGtEq(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::CmpAllGtEq(arg1, arg2, 0b0001);
        }


        AZ_MATH_INLINE Vec1::Int32Type Vec1::CmpEq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::CmpEq(arg1, arg2);
        }


        AZ_MATH_INLINE Vec1::Int32Type Vec1::CmpNeq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::CmpNeq(arg1, arg2);
        }


        AZ_MATH_INLINE Vec1::Int32Type Vec1::CmpGt(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::CmpGt(arg1, arg2);
        }


        AZ_MATH_INLINE Vec1::Int32Type Vec1::CmpGtEq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::CmpGtEq(arg1, arg2);
        }


        AZ_MATH_INLINE Vec1::Int32Type Vec1::CmpLt(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::CmpLt(arg1, arg2);
        }


        AZ_MATH_INLINE Vec1::Int32Type Vec1::CmpLtEq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::CmpLtEq(arg1, arg2);
        }


        AZ_MATH_INLINE bool Vec1::CmpAllEq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::CmpAllEq(arg1, arg2, 0b0001);
        }


        AZ_MATH_INLINE Vec1::FloatType Vec1::Select(FloatArgType arg1, FloatArgType arg2, FloatArgType mask)
        {
            return Sse::Select(arg1, arg2, mask);
        }


        AZ_MATH_INLINE Vec1::Int32Type Vec1::Select(Int32ArgType arg1, Int32ArgType arg2, Int32ArgType mask)
        {
            return Sse::Select(arg1, arg2, mask);
        }


        AZ_MATH_INLINE Vec1::FloatType Vec1::Reciprocal(FloatArgType value)
        {
            return Sse::Reciprocal(value);
        }


        AZ_MATH_INLINE Vec1::FloatType Vec1::ReciprocalEstimate(FloatArgType value)
        {
            return Sse::ReciprocalEstimate(value);
        }


        AZ_MATH_INLINE Vec1::FloatType Vec1::Mod(FloatArgType value, FloatArgType divisor)
        {
            return Sse::Mod(value, divisor);
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
            return Sse::Sqrt(value);
        }


        AZ_MATH_INLINE Vec1::FloatType Vec1::SqrtEstimate(FloatArgType value)
        {
            return Sse::SqrtEstimate(value);
        }


        AZ_MATH_INLINE Vec1::FloatType Vec1::SqrtInv(FloatArgType value)
        {
            return Sse::SqrtInv(value);
        }


        AZ_MATH_INLINE Vec1::FloatType Vec1::SqrtInvEstimate(FloatArgType value)
        {
            return Sse::SqrtInvEstimate(value);
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


        AZ_MATH_INLINE Vec1::FloatType Vec1::ConvertToFloat(Int32ArgType value)
        {
            return Sse::ConvertToFloat(value);
        }


        AZ_MATH_INLINE Vec1::Int32Type Vec1::ConvertToInt(FloatArgType value)
        {
            return Sse::ConvertToInt(value);
        }


        AZ_MATH_INLINE Vec1::Int32Type Vec1::ConvertToIntNearest(FloatArgType value)
        {
            return Sse::ConvertToIntNearest(value);
        }


        AZ_MATH_INLINE Vec1::FloatType Vec1::CastToFloat(Int32ArgType value)
        {
            return Sse::CastToFloat(value);
        }


        AZ_MATH_INLINE Vec1::Int32Type Vec1::CastToInt(FloatArgType value)
        {
            return Sse::CastToInt(value);
        }


        AZ_MATH_INLINE Vec1::FloatType Vec1::ZeroFloat()
        {
            return Sse::ZeroFloat();
        }


        AZ_MATH_INLINE Vec1::Int32Type Vec1::ZeroInt()
        {
            return Sse::ZeroInt();
        }
    }
}
