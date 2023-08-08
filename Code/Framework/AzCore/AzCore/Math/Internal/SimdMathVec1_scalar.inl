/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Internal/SimdMathCommon_scalar.inl>

namespace AZ
{
    namespace Simd
    {
        AZ_MATH_INLINE Vec1::FloatType Vec1::LoadAligned(const float* __restrict addr)
        {
            return LoadUnaligned(addr);
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::LoadAligned(const int32_t* __restrict addr)
        {
            return LoadUnaligned(addr);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::LoadUnaligned(const float* __restrict addr)
        {
            return *addr;
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::LoadUnaligned(const int32_t* __restrict addr)
        {
            return *addr;
        }

        AZ_MATH_INLINE void Vec1::StoreAligned(float* __restrict addr, FloatArgType value)
        {
            return StoreUnaligned(addr, value);
        }

        AZ_MATH_INLINE void Vec1::StoreAligned(int32_t* __restrict addr, Int32ArgType value)
        {
            return StoreUnaligned(addr, value);
        }

        AZ_MATH_INLINE void Vec1::StoreUnaligned(float* __restrict addr, FloatArgType value)
        {
            *addr = value;
        }

        AZ_MATH_INLINE void Vec1::StoreUnaligned(int32_t* __restrict addr, Int32ArgType value)
        {
            *addr = value;
        }

        AZ_MATH_INLINE void Vec1::StreamAligned(float* __restrict addr, FloatArgType value)
        {
            return StoreUnaligned(addr, value);
        }

        AZ_MATH_INLINE void Vec1::StreamAligned(int32_t* __restrict addr, Int32ArgType value)
        {
            return StoreUnaligned(addr, value);
        }

        AZ_MATH_INLINE float Vec1::SelectIndex0(FloatArgType value)
        {
            return value;
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Splat(float value)
        {
            return LoadImmediate(value);
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::Splat(int32_t value)
        {
            return LoadImmediate(value);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::LoadImmediate(float x)
        {
            return x;
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::LoadImmediate(int32_t x)
        {
            return x;
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Add(FloatArgType arg1, FloatArgType arg2)
        {
            return arg1 + arg2;
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Sub(FloatArgType arg1, FloatArgType arg2)
        {
            return arg1 - arg2;
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Mul(FloatArgType arg1, FloatArgType arg2)
        {
            return arg1 * arg2;
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Madd(FloatArgType mul1, FloatArgType mul2, FloatArgType add)
        {
            return Add(Mul(mul1, mul2), add);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Div(FloatArgType arg1, FloatArgType arg2)
        {
            return arg1 / arg2;
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Abs(FloatArgType value)
        {
            return fabsf(value);
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::Add(Int32ArgType arg1, Int32ArgType arg2)
        {
            return arg1 + arg2;
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::Sub(Int32ArgType arg1, Int32ArgType arg2)
        {
            return arg1 - arg2;
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::Mul(Int32ArgType arg1, Int32ArgType arg2)
        {
            return arg1 * arg2;
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::Madd(Int32ArgType mul1, Int32ArgType mul2, Int32ArgType add)
        {
            return Add(Mul(mul1, mul2), add);
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::Abs(Int32ArgType value)
        {
            return abs(value);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Not(FloatArgType value)
        {
            Int32Type result = ~reinterpret_cast<int32_t&>(value);
            return CastToFloat(result);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::And(FloatArgType arg1, FloatArgType arg2)
        {
            Int32Type result = reinterpret_cast<int32_t&>(arg1) & reinterpret_cast<int32_t&>(arg2);
            return CastToFloat(result);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::AndNot(FloatArgType arg1, FloatArgType arg2)
        {
            Int32Type result = ~reinterpret_cast<int32_t&>(arg1) & reinterpret_cast<int32_t&>(arg2);
            return CastToFloat(result);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Or(FloatArgType arg1, FloatArgType arg2)
        {
            Int32Type result = reinterpret_cast<int32_t&>(arg1) | reinterpret_cast<int32_t&>(arg2);
            return CastToFloat(result);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Xor(FloatArgType arg1, FloatArgType arg2)
        {
            Int32Type result = reinterpret_cast<int32_t&>(arg1) ^ reinterpret_cast<int32_t&>(arg2);
            return CastToFloat(result);
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::Not(Int32ArgType value)
        {
            return ~value;
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::And(Int32ArgType arg1, Int32ArgType arg2)
        {
            return arg1 & arg2;
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::AndNot(Int32ArgType arg1, Int32ArgType arg2)
        {
            return ~arg1 & arg2;
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::Or(Int32ArgType arg1, Int32ArgType arg2)
        {
            return arg1 | arg2;
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::Xor(Int32ArgType arg1, Int32ArgType arg2)
        {
            return arg1 ^ arg2;
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Floor(FloatArgType value)
        {
            return AZStd::floorf(value);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Ceil(FloatArgType value)
        {
            return AZStd::ceilf(value);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Round(FloatArgType value)
        {
            // While 'roundf' may seem the obvious choice here, it rounds halfway cases
            // away from zero regardless of the current rounding mode, but 'rintf' uses
            // the current rounding mode which is consistent with other implementations.
            return AZStd::rintf(value);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Truncate(FloatArgType value)
        {
            return ConvertToFloat(ConvertToInt(value));
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Min(FloatArgType arg1, FloatArgType arg2)
        {
            return AZStd::min(arg1, arg2);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Max(FloatArgType arg1, FloatArgType arg2)
        {
            return AZStd::max(arg1, arg2);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Clamp(FloatArgType value, FloatArgType min, FloatArgType max)
        {
            return Max(min, Min(value, max));
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::Min(Int32ArgType arg1, Int32ArgType arg2)
        {
            return AZStd::min(arg1, arg2);
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::Max(Int32ArgType arg1, Int32ArgType arg2)
        {
            return AZStd::max(arg1, arg2);
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::Clamp(Int32ArgType value, Int32ArgType min, Int32ArgType max)
        {
            return Max(min, Min(value, max));
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::CmpEq(FloatArgType arg1, FloatArgType arg2)
        {
            Int32Type result = (arg1 == arg2) ? (int32_t)0xFFFFFFFF : 0x00000000;
            return CastToFloat(result);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::CmpNeq(FloatArgType arg1, FloatArgType arg2)
        {
            Int32Type result = (arg1 != arg2) ? (int32_t)0xFFFFFFFF : 0x00000000;
            return CastToFloat(result);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::CmpGt(FloatArgType arg1, FloatArgType arg2)
        {
            Int32Type result = (arg1 > arg2) ? (int32_t)0xFFFFFFFF : 0x00000000;
            return CastToFloat(result);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::CmpGtEq(FloatArgType arg1, FloatArgType arg2)
        {
            Int32Type result = (arg1 >= arg2) ? (int32_t)0xFFFFFFFF : 0x00000000;
            return CastToFloat(result);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::CmpLt(FloatArgType arg1, FloatArgType arg2)
        {
            Int32Type result = (arg1 < arg2) ? (int32_t)0xFFFFFFFF : 0x00000000;
            return CastToFloat(result);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::CmpLtEq(FloatArgType arg1, FloatArgType arg2)
        {
            Int32Type result = (arg1 <= arg2) ? (int32_t)0xFFFFFFFF : 0x00000000;
            return CastToFloat(result);
        }

        AZ_MATH_INLINE bool Vec1::CmpAllEq(FloatArgType arg1, FloatArgType arg2)
        {
            return arg1 == arg2;
        }

        AZ_MATH_INLINE bool Vec1::CmpAllLt(FloatArgType arg1, FloatArgType arg2)
        {
            return arg1 < arg2;
        }

        AZ_MATH_INLINE bool Vec1::CmpAllLtEq(FloatArgType arg1, FloatArgType arg2)
        {
            return arg1 <= arg2;
        }

        AZ_MATH_INLINE bool Vec1::CmpAllGt(FloatArgType arg1, FloatArgType arg2)
        {
            return arg1 > arg2;
        }

        AZ_MATH_INLINE bool Vec1::CmpAllGtEq(FloatArgType arg1, FloatArgType arg2)
        {
            return arg1 >= arg2;
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::CmpEq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return (arg1 == arg2) ? (int32_t)0xFFFFFFFF : 0x00000000;
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::CmpNeq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return (arg1 != arg2) ? (int32_t)0xFFFFFFFF : 0x00000000;
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::CmpGt(Int32ArgType arg1, Int32ArgType arg2)
        {
            return (arg1 > arg2) ? (int32_t)0xFFFFFFFF : 0x00000000;
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::CmpGtEq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return (arg1 >= arg2) ? (int32_t)0xFFFFFFFF : 0x00000000;
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::CmpLt(Int32ArgType arg1, Int32ArgType arg2)
        {
            return (arg1 < arg2) ? (int32_t)0xFFFFFFFF : 0x00000000;
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::CmpLtEq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return (arg1 <= arg2) ? (int32_t)0xFFFFFFFF : 0x00000000;
        }

        AZ_MATH_INLINE bool Vec1::CmpAllEq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return arg1 == arg2;
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Select(FloatArgType arg1, FloatArgType arg2, FloatArgType mask)
        {
            return (CastToInt(mask) == 0) ? arg2 : arg1;
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::Select(Int32ArgType arg1, Int32ArgType arg2, Int32ArgType mask)
        {
            return (mask == 0) ? arg2 : arg1;
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Reciprocal(FloatArgType value)
        {
            return 1.0f / value;
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::ReciprocalEstimate(FloatArgType value)
        {
            return Reciprocal(value);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Mod(FloatArgType value, FloatArgType divisor)
        {
            return fmodf(value, divisor);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Wrap(FloatArgType value, FloatArgType minValue, FloatArgType maxValue)
        {
            return Scalar::Wrap(value, minValue, maxValue);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::AngleMod(FloatArgType value)
        {
            return Scalar::AngleMod(value);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Sqrt(FloatArgType value)
        {
            return sqrtf(value);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::SqrtEstimate(FloatArgType value)
        {
            return Sqrt(value);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::SqrtInv(FloatArgType value)
        {
            return 1.0f / sqrtf(value);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::SqrtInvEstimate(FloatArgType value)
        {
            return SqrtInv(value);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Sin(FloatArgType value)
        {
            return sinf(value);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Cos(FloatArgType value)
        {
            return cosf(value);
        }

        AZ_MATH_INLINE void Vec1::SinCos(FloatArgType value, FloatType& sin, FloatType& cos)
        {
            sin = sinf(value);
            cos = cosf(value);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Acos(FloatArgType value)
        {
            return acosf(value);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Atan(FloatArgType value)
        {
            return atanf(value);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::Atan2(FloatArgType y, FloatArgType x)
        {
            return atan2f(y, x);
        }


        AZ_MATH_INLINE Vec1::FloatType Vec1::ExpEstimate(FloatArgType x)
        {
            static constexpr float   ExpCoef1 = 1.2102203e7f;
            static constexpr int32_t ExpCoef2 = -8388608;
            static constexpr float   ExpCoef3 = 1.1920929e-7f;
            static constexpr float   ExpCoef4 = 3.371894346e-1f;
            static constexpr float   ExpCoef5 = 6.57636276e-1f;
            static constexpr float   ExpCoef6 = 1.00172476f;

            const int32_t a = int32_t(ExpCoef1 * x);
            const int32_t b = a & ExpCoef2;
            const int32_t c = a - b;
            const float f = ExpCoef3 * float(c);
            const float i = f * ExpCoef4 + ExpCoef5;
            const float j = i * f + ExpCoef6;
            uint32_t k;
            memcpy(&k, &j, sizeof(float));
            k += b;
            float r;
            memcpy(&r, &k, sizeof(float));
            return r;
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::ConvertToFloat(Int32ArgType value)
        {
            return static_cast<float>(value);
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::ConvertToInt(FloatArgType value)
        {
            return static_cast<int32_t>(value);
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::ConvertToIntNearest(FloatArgType value)
        {
            return ConvertToInt(Round(value));
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::CastToFloat(Int32ArgType value)
        {
            return reinterpret_cast<float&>(value);
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::CastToInt(FloatArgType value)
        {
            return reinterpret_cast<int32_t&>(value);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec1::ZeroFloat()
        {
            return 0.0f;
        }

        AZ_MATH_INLINE Vec1::Int32Type Vec1::ZeroInt()
        {
            return 0;
        }
    }
}
