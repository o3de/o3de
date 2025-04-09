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
        AZ_MATH_INLINE Vec1::FloatType Vec2::ToVec1(FloatArgType value)
        {
            return value.v[0];
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::FromVec1(Vec1::FloatArgType value)
        {
            return {{ value, value }};
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::LoadAligned(const float* __restrict addr)
        {
            return LoadUnaligned(addr);
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::LoadAligned(const int32_t* __restrict addr)
        {
            return LoadUnaligned(addr);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::LoadUnaligned(const float* __restrict addr)
        {
            FloatType result = {{ addr[0], addr[1] }};
            return result;
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::LoadUnaligned(const int32_t* __restrict addr)
        {
            Int32Type result = {{ addr[0], addr[1] }};
            return result;
        }

        AZ_MATH_INLINE void Vec2::StoreAligned(float* __restrict addr, FloatArgType value)
        {
            return StoreUnaligned(addr, value);
        }

        AZ_MATH_INLINE void Vec2::StoreAligned(int32_t* __restrict addr, Int32ArgType value)
        {
            return StoreUnaligned(addr, value);
        }

        AZ_MATH_INLINE void Vec2::StoreUnaligned(float* __restrict addr, FloatArgType value)
        {
            addr[0] = value.v[0];
            addr[1] = value.v[1];
        }

        AZ_MATH_INLINE void Vec2::StoreUnaligned(int32_t* __restrict addr, Int32ArgType value)
        {
            addr[0] = value.v[0];
            addr[1] = value.v[1];
        }

        AZ_MATH_INLINE void Vec2::StreamAligned(float* __restrict addr, FloatArgType value)
        {
            return StoreUnaligned(addr, value);
        }

        AZ_MATH_INLINE void Vec2::StreamAligned(int32_t* __restrict addr, Int32ArgType value)
        {
            return StoreUnaligned(addr, value);
        }

        AZ_MATH_INLINE float Vec2::SelectIndex0(FloatArgType value)
        {
            return value.v[0];
        }

        AZ_MATH_INLINE float Vec2::SelectIndex1(FloatArgType value)
        {
            return value.v[1];
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Splat(float value)
        {
            return LoadImmediate(value, value);
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::Splat(int32_t value)
        {
            return LoadImmediate(value, value);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::SplatIndex0(FloatArgType value)
        {
            return Splat(value.v[0]);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::SplatIndex1(FloatArgType value)
        {
            return Splat(value.v[1]);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::ReplaceIndex0(FloatArgType a, float b)
        {
            return {{ b, a.v[1] }};
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::ReplaceIndex0(FloatArgType a, FloatArgType b)
        {
            return {{ b.v[0], a.v[1] }};
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::ReplaceIndex1(FloatArgType a, float b)
        {
            return {{ a.v[0], b }};
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::ReplaceIndex1(FloatArgType a, FloatArgType b)
        {
            return {{ a.v[0], b.v[1] }};
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::LoadImmediate(float x, float y)
        {
            FloatType result = {{ x, y }};
            return result;
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::LoadImmediate(int32_t x, int32_t y)
        {
            Int32Type result = {{ x, y }};
            return result;
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Add(FloatArgType arg1, FloatArgType arg2)
        {
            return {{ arg1.v[0] + arg2.v[0], arg1.v[1] + arg2.v[1] }};
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Sub(FloatArgType arg1, FloatArgType arg2)
        {
            return {{ arg1.v[0] - arg2.v[0], arg1.v[1] - arg2.v[1] }};
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Mul(FloatArgType arg1, FloatArgType arg2)
        {
            return {{ arg1.v[0] * arg2.v[0], arg1.v[1] * arg2.v[1] }};
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Madd(FloatArgType mul1, FloatArgType mul2, FloatArgType add)
        {
            return Add(Mul(mul1, mul2), add);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Div(FloatArgType arg1, FloatArgType arg2)
        {
            return {{ arg1.v[0] / arg2.v[0], arg1.v[1] / arg2.v[1] }};
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Abs(FloatArgType value)
        {
            return {{ fabsf(value.v[0]), fabsf(value.v[1]) }};
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::Add(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ arg1.v[0] + arg2.v[0], arg1.v[1] + arg2.v[1] }};
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::Sub(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ arg1.v[0] - arg2.v[0], arg1.v[1] - arg2.v[1] }};
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::Mul(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ arg1.v[0] * arg2.v[0], arg1.v[1] * arg2.v[1] }};
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::Madd(Int32ArgType mul1, Int32ArgType mul2, Int32ArgType add)
        {
            return Add(Mul(mul1, mul2), add);
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::Abs(Int32ArgType value)
        {
            return {{ abs(value.v[0]), abs(value.v[1]) }};
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Not(FloatArgType value)
        {
            Int32Type result = {{ ~reinterpret_cast<const int32_t&>(value.v[0])
                                , ~reinterpret_cast<const int32_t&>(value.v[1]) }};
            return CastToFloat(result);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::And(FloatArgType arg1, FloatArgType arg2)
        {
            Int32Type result = {{ reinterpret_cast<const int32_t&>(arg1.v[0]) & reinterpret_cast<const int32_t&>(arg2.v[0])
                                , reinterpret_cast<const int32_t&>(arg1.v[1]) & reinterpret_cast<const int32_t&>(arg2.v[1]) }};
            return CastToFloat(result);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::AndNot(FloatArgType arg1, FloatArgType arg2)
        {
            Int32Type result = {{ ~reinterpret_cast<const int32_t&>(arg1.v[0]) & reinterpret_cast<const int32_t&>(arg2.v[0])
                                , ~reinterpret_cast<const int32_t&>(arg1.v[1]) & reinterpret_cast<const int32_t&>(arg2.v[1]) }};
            return CastToFloat(result);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Or (FloatArgType arg1, FloatArgType arg2)
        {
            Int32Type result = {{ reinterpret_cast<const int32_t&>(arg1.v[0]) | reinterpret_cast<const int32_t&>(arg2.v[0])
                                , reinterpret_cast<const int32_t&>(arg1.v[1]) | reinterpret_cast<const int32_t&>(arg2.v[1]) }};
            return CastToFloat(result);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Xor(FloatArgType arg1, FloatArgType arg2)
        {
            Int32Type result = {{ reinterpret_cast<const int32_t&>(arg1.v[0]) ^ reinterpret_cast<const int32_t&>(arg2.v[0])
                                , reinterpret_cast<const int32_t&>(arg1.v[1]) ^ reinterpret_cast<const int32_t&>(arg2.v[1]) }};
            return CastToFloat(result);
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::Not(Int32ArgType value)
        {
            return {{ ~value.v[0], ~value.v[1] }};
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::And(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ arg1.v[0] & arg2.v[0], arg1.v[1] & arg2.v[1] }};
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::AndNot(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ ~arg1.v[0] & arg2.v[0], ~arg1.v[1] & arg2.v[1] }};
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::Or(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ arg1.v[0] | arg2.v[0], arg1.v[1] | arg2.v[1] }};
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::Xor(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ arg1.v[0] ^ arg2.v[0], arg1.v[1] ^ arg2.v[1] }};
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Floor(FloatArgType value)
        {
            return {{ AZStd::floorf(value.v[0]), AZStd::floorf(value.v[1]) }};
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Ceil(FloatArgType value)
        {
            return {{ AZStd::ceilf(value.v[0]), AZStd::ceilf(value.v[1]) }};
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Round(FloatArgType value)
        {

            // While 'roundf' may seem the obvious choice here, it rounds halfway cases
            // away from zero regardless of the current rounding mode, but 'rintf' uses
            // the current rounding mode which is consistent with other implementations.
            return {{ AZStd::rintf(value.v[0]), AZStd::rintf(value.v[1]) }};
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Truncate(FloatArgType value)
        {
            return ConvertToFloat(ConvertToInt(value));
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Min(FloatArgType arg1, FloatArgType arg2)
        {
            return {{ AZStd::min(arg1.v[0], arg2.v[0]), AZStd::min(arg1.v[1], arg2.v[1]) }};
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Max(FloatArgType arg1, FloatArgType arg2)
        {
            return {{ AZStd::max(arg1.v[0], arg2.v[0]), AZStd::max(arg1.v[1], arg2.v[1]) }};
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Clamp(FloatArgType value, FloatArgType min, FloatArgType max)
        {
            return Max(min, Min(value, max));
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::Min(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ AZStd::min(arg1.v[0], arg2.v[0]), AZStd::min(arg1.v[1], arg2.v[1]) }};
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::Max(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ AZStd::max(arg1.v[0], arg2.v[0]), AZStd::max(arg1.v[1], arg2.v[1]) }};
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::Clamp(Int32ArgType value, Int32ArgType min, Int32ArgType max)
        {
            return Max(min, Min(value, max));
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::CmpEq(FloatArgType arg1, FloatArgType arg2)
        {
            Int32Type result = {{ (arg1.v[0] == arg2.v[0]) ? (int32_t)0xFFFFFFFF : 0x00000000
                                , (arg1.v[1] == arg2.v[1]) ? (int32_t)0xFFFFFFFF : 0x00000000 }};
            return CastToFloat(result);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::CmpNeq(FloatArgType arg1, FloatArgType arg2)
        {
            Int32Type result = {{ (arg1.v[0] != arg2.v[0]) ? (int32_t)0xFFFFFFFF : 0x00000000
                                , (arg1.v[1] != arg2.v[1]) ? (int32_t)0xFFFFFFFF : 0x00000000 }};
            return CastToFloat(result);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::CmpGt(FloatArgType arg1, FloatArgType arg2)
        {
            Int32Type result = {{ (arg1.v[0] > arg2.v[0]) ? (int32_t)0xFFFFFFFF : 0x00000000
                                , (arg1.v[1] > arg2.v[1]) ? (int32_t)0xFFFFFFFF : 0x00000000 }};
            return CastToFloat(result);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::CmpGtEq(FloatArgType arg1, FloatArgType arg2)
        {
            Int32Type result = {{ (arg1.v[0] >= arg2.v[0]) ? (int32_t)0xFFFFFFFF : 0x00000000
                                , (arg1.v[1] >= arg2.v[1]) ? (int32_t)0xFFFFFFFF : 0x00000000 }};
            return CastToFloat(result);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::CmpLt(FloatArgType arg1, FloatArgType arg2)
        {
            Int32Type result = {{ (arg1.v[0] < arg2.v[0]) ? (int32_t)0xFFFFFFFF : 0x00000000
                                , (arg1.v[1] < arg2.v[1]) ? (int32_t)0xFFFFFFFF : 0x00000000 }};
            return CastToFloat(result);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::CmpLtEq(FloatArgType arg1, FloatArgType arg2)
        {
            Int32Type result = {{ (arg1.v[0] <= arg2.v[0]) ? (int32_t)0xFFFFFFFF : 0x00000000
                                , (arg1.v[1] <= arg2.v[1]) ? (int32_t)0xFFFFFFFF : 0x00000000 }};
            return CastToFloat(result);
        }

        AZ_MATH_INLINE bool Vec2::CmpAllEq(FloatArgType arg1, FloatArgType arg2)
        {
            for (int32_t i = 0; i < ElementCount; ++i)
            {
                if (arg1.v[i] != arg2.v[i])
                {
                    return false;
                }
            }
            return true;
        }

        AZ_MATH_INLINE bool Vec2::CmpAllLt(FloatArgType arg1, FloatArgType arg2)
        {
            for (int32_t i = 0; i < ElementCount; ++i)
            {
                if (arg1.v[i] >= arg2.v[i])
                {
                    return false;
                }
            }
            return true;
        }

        AZ_MATH_INLINE bool Vec2::CmpAllLtEq(FloatArgType arg1, FloatArgType arg2)
        {
            for (int32_t i = 0; i < ElementCount; ++i)
            {
                if (arg1.v[i] > arg2.v[i])
                {
                    return false;
                }
            }
            return true;
        }

        AZ_MATH_INLINE bool Vec2::CmpAllGt(FloatArgType arg1, FloatArgType arg2)
        {
            for (int32_t i = 0; i < ElementCount; ++i)
            {
                if (arg1.v[i] <= arg2.v[i])
                {
                    return false;
                }
            }
            return true;
        }

        AZ_MATH_INLINE bool Vec2::CmpAllGtEq(FloatArgType arg1, FloatArgType arg2)
        {
            for (int32_t i = 0; i < ElementCount; ++i)
            {
                if (arg1.v[i] < arg2.v[i])
                {
                    return false;
                }
            }
            return true;
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::CmpEq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ (arg1.v[0] == arg2.v[0]) ? (int32_t)0xFFFFFFFF : 0x00000000
                    , (arg1.v[1] == arg2.v[1]) ? (int32_t)0xFFFFFFFF : 0x00000000 }};
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::CmpNeq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ (arg1.v[0] != arg2.v[0]) ? (int32_t)0xFFFFFFFF : 0x00000000
                    , (arg1.v[1] != arg2.v[1]) ? (int32_t)0xFFFFFFFF : 0x00000000 }};
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::CmpGt(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ (arg1.v[0] > arg2.v[0]) ? (int32_t)0xFFFFFFFF : 0x00000000
                    , (arg1.v[1] > arg2.v[1]) ? (int32_t)0xFFFFFFFF : 0x00000000 }};
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::CmpGtEq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ (arg1.v[0] >= arg2.v[0]) ? (int32_t)0xFFFFFFFF : 0x00000000
                    , (arg1.v[1] >= arg2.v[1]) ? (int32_t)0xFFFFFFFF : 0x00000000 }};
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::CmpLt(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ (arg1.v[0] < arg2.v[0]) ? (int32_t)0xFFFFFFFF : 0x00000000
                    , (arg1.v[1] < arg2.v[1]) ? (int32_t)0xFFFFFFFF : 0x00000000 }};
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::CmpLtEq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ (arg1.v[0] <= arg2.v[0]) ? (int32_t)0xFFFFFFFF : 0x00000000
                    , (arg1.v[1] <= arg2.v[1]) ? (int32_t)0xFFFFFFFF : 0x00000000 }};
        }

        AZ_MATH_INLINE bool Vec2::CmpAllEq(Int32ArgType arg1, Int32ArgType arg2)
        {
            for (int32_t i = 0; i < ElementCount; ++i)
            {
                if (arg1.v[i] != arg2.v[i])
                {
                    return false;
                }
            }
            return true;
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Select(FloatArgType arg1, FloatArgType arg2, FloatArgType mask)
        {
            const Int32Type intMask = CastToInt(mask);
            return {{ (intMask.v[0] == 0) ? arg2.v[0] : arg1.v[0], (intMask.v[1] == 0) ? arg2.v[1] : arg1.v[1] }};
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::Select(Int32ArgType arg1, Int32ArgType arg2, Int32ArgType mask)
        {
            return {{ (mask.v[0] == 0) ? arg2.v[0] : arg1.v[0], (mask.v[1] == 0) ? arg2.v[1] : arg1.v[1] }};
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Reciprocal(FloatArgType value)
        {
            return {{ 1.0f / value.v[0], 1.0f / value.v[1] }};
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::ReciprocalEstimate(FloatArgType value)
        {
            return Reciprocal(value);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Mod(FloatArgType value, FloatArgType divisor)
        {
            return {{ fmodf(value.v[0], divisor.v[0]), fmodf(value.v[1], divisor.v[1]) }};
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Wrap(FloatArgType value, FloatArgType minValue, FloatArgType maxValue)
        {
            return {{ Scalar::Wrap(value.v[0], minValue.v[0], maxValue.v[0])
                    , Scalar::Wrap(value.v[1], minValue.v[1], maxValue.v[1]) }};
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::AngleMod(FloatArgType value)
        {
            return {{ Scalar::AngleMod(value.v[0]), Scalar::AngleMod(value.v[1]) }};
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Sqrt(FloatArgType value)
        {
            return {{ sqrtf(value.v[0]), sqrtf(value.v[1]) }};
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::SqrtEstimate(FloatArgType value)
        {
            return Sqrt(value);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::SqrtInv(FloatArgType value)
        {
            return {{ 1.0f / sqrtf(value.v[0]), 1.0f / sqrtf(value.v[1]) }};
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::SqrtInvEstimate(FloatArgType value)
        {
            return SqrtInv(value);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Sin(FloatArgType value)
        {
            return {{ sinf(value.v[0]), sinf(value.v[1]) }};
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Cos(FloatArgType value)
        {
            return {{ cosf(value.v[0]), cosf(value.v[1]) }};
        }

        AZ_MATH_INLINE void Vec2::SinCos(FloatArgType value, FloatType& sin, FloatType& cos)
        {
            sin = Sin(value);
            cos = Cos(value);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::SinCos(Vec1::FloatArgType angle)
        {
            return {{ sinf(angle), cosf(angle) }};
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Acos(FloatArgType value)
        {
            return {{ acosf(value.v[0]), acosf(value.v[1]) }};
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Atan(FloatArgType value)
        {
            return {{ atanf(value.v[0]), atanf(value.v[1]) }};
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Atan2(FloatArgType y, FloatArgType x)
        {
            return {{ atan2f(y.v[0], x.v[0]), atan2f(y.v[1], x.v[1]) }};
        }

        AZ_MATH_INLINE Vec1::FloatType Vec2::Atan2(FloatArgType value)
        {
            return atan2f(value.v[1], value.v[0]);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::ExpEstimate(FloatArgType x)
        {
            return {{ Vec1::ExpEstimate(x.v[0]), Vec1::ExpEstimate(x.v[1]) }};
        }

        AZ_MATH_INLINE Vec1::FloatType Vec2::Dot(FloatArgType arg1, FloatArgType arg2)
        {
            return (arg1.v[0] * arg2.v[0]) + (arg1.v[1] * arg2.v[1]);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::Normalize(FloatArgType value)
        {
            const float invLength = 1.0f / sqrtf(Dot(value, value));
            return {{ value.v[0] * invLength, value.v[1] * invLength }};
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::NormalizeEstimate(FloatArgType value)
        {
            return Normalize(value);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::NormalizeSafe(FloatArgType value, float tolerance)
        {
            const float sqLength = Dot(value, value);
            if (sqLength <= (tolerance * tolerance))
            {
                return ZeroFloat();
            }
            else
            {
                const float invLength = 1.0f / sqrtf(sqLength);
                return {{ value.v[0] * invLength, value.v[1] * invLength }};
            }
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::NormalizeSafeEstimate(FloatArgType value, float tolerance)
        {
            return NormalizeSafe(value, tolerance);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::ConvertToFloat(Int32ArgType value)
        {
            return {{ static_cast<float>(value.v[0])
                    , static_cast<float>(value.v[1]) }};
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::ConvertToInt(FloatArgType value)
        {
            return {{ static_cast<int32_t>(value.v[0])
                    , static_cast<int32_t>(value.v[1]) }};
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::ConvertToIntNearest(FloatArgType value)
        {
            return ConvertToInt(Round(value));
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::CastToFloat(Int32ArgType value)
        {
            return {{ reinterpret_cast<const float&>(value.v[0])
                    , reinterpret_cast<const float&>(value.v[1]) }};
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::CastToInt(FloatArgType value)
        {
            return {{ reinterpret_cast<const int32_t&>(value.v[0])
                    , reinterpret_cast<const int32_t&>(value.v[1]) }};
        }

        AZ_MATH_INLINE Vec2::FloatType Vec2::ZeroFloat()
        {
            return {{ 0.0f, 0.0f }};
        }

        AZ_MATH_INLINE Vec2::Int32Type Vec2::ZeroInt()
        {
            return {{ 0, 0 }};
        }
    }
}
