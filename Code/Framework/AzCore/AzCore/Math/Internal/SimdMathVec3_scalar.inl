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
        AZ_MATH_INLINE Vec1::FloatType Vec3::ToVec1(FloatArgType value)
        {
            return value.v[0];
        }

        AZ_MATH_INLINE Vec2::FloatType Vec3::ToVec2(FloatArgType value)
        {
            return {{ value.v[0], value.v[1] }};
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::FromVec1(Vec1::FloatArgType value)
        {
            return {{ value, value, value }};
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::FromVec2(Vec2::FloatArgType value)
        {
            return {{ value.v[0], value.v[1], 0.0f }};
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::LoadAligned(const float* __restrict addr)
        {
            return LoadUnaligned(addr);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::LoadAligned(const int32_t* __restrict addr)
        {
            return LoadUnaligned(addr);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::LoadUnaligned(const float* __restrict addr)
        {
            FloatType result = {{ addr[0], addr[1], addr[2] }};
            return result;
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::LoadUnaligned(const int32_t* __restrict addr)
        {
            Int32Type result = {{ addr[0], addr[1], addr[2] }};
            return result;
        }

        AZ_MATH_INLINE void Vec3::StoreAligned(float* __restrict addr, FloatArgType value)
        {
            return StoreUnaligned(addr, value);
        }

        AZ_MATH_INLINE void Vec3::StoreAligned(int32_t* __restrict addr, Int32ArgType value)
        {
            return StoreUnaligned(addr, value);
        }

        AZ_MATH_INLINE void Vec3::StoreUnaligned(float* __restrict addr, FloatArgType value)
        {
            addr[0] = value.v[0];
            addr[1] = value.v[1];
            addr[2] = value.v[2];
        }

        AZ_MATH_INLINE void Vec3::StoreUnaligned(int32_t* __restrict addr, Int32ArgType value)
        {
            addr[0] = value.v[0];
            addr[1] = value.v[1];
            addr[2] = value.v[2];
        }

        AZ_MATH_INLINE void Vec3::StreamAligned(float* __restrict addr, FloatArgType value)
        {
            return StoreUnaligned(addr, value);
        }

        AZ_MATH_INLINE void Vec3::StreamAligned(int32_t* __restrict addr, Int32ArgType value)
        {
            return StoreUnaligned(addr, value);
        }

        AZ_MATH_INLINE float Vec3::SelectIndex0(FloatArgType value)
        {
            return value.v[0];
        }

        AZ_MATH_INLINE float Vec3::SelectIndex1(FloatArgType value)
        {
            return value.v[1];
        }

        AZ_MATH_INLINE float Vec3::SelectIndex2(FloatArgType value)
        {
            return value.v[2];
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Splat(float value)
        {
            return LoadImmediate(value, value, value);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::Splat(int32_t value)
        {
            return LoadImmediate(value, value, value);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::SplatIndex0(FloatArgType value)
        {
            return Splat(value.v[0]);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::SplatIndex1(FloatArgType value)
        {
            return Splat(value.v[1]);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::SplatIndex2(FloatArgType value)
        {
            return Splat(value.v[2]);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::ReplaceIndex0(FloatArgType a, float b)
        {
            return {{ b, a.v[1], a.v[2] }};
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::ReplaceIndex0(FloatArgType a, FloatArgType b)
        {
            return {{ b.v[0], a.v[1], a.v[2] }};
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::ReplaceIndex1(FloatArgType a, float b)
        {
            return {{ a.v[0], b, a.v[2] }};
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::ReplaceIndex1(FloatArgType a, FloatArgType b)
        {
            return {{ a.v[0], b.v[1], a.v[2] }};
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::ReplaceIndex2(FloatArgType a, float b)
        {
            return {{ a.v[0], a.v[1], b }};
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::ReplaceIndex2(FloatArgType a, FloatArgType b)
        {
            return {{ a.v[0], a.v[1], b.v[2] }};
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::LoadImmediate(float x, float y, float z)
        {
            FloatType result = {{ x, y, z }};
            return result;
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::LoadImmediate(int32_t x, int32_t y, int32_t z)
        {
            Int32Type result = {{ x, y, z }};
            return result;
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Add(FloatArgType arg1, FloatArgType arg2)
        {
            return {{ arg1.v[0] + arg2.v[0], arg1.v[1] + arg2.v[1], arg1.v[2] + arg2.v[2] }};
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Sub(FloatArgType arg1, FloatArgType arg2)
        {
            return {{ arg1.v[0] - arg2.v[0], arg1.v[1] - arg2.v[1], arg1.v[2] - arg2.v[2] }};
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Mul(FloatArgType arg1, FloatArgType arg2)
        {
            return {{ arg1.v[0] * arg2.v[0], arg1.v[1] * arg2.v[1], arg1.v[2] * arg2.v[2] }};
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Madd(FloatArgType mul1, FloatArgType mul2, FloatArgType add)
        {
            return Add(Mul(mul1, mul2), add);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Div(FloatArgType arg1, FloatArgType arg2)
        {
            return {{ arg1.v[0] / arg2.v[0], arg1.v[1] / arg2.v[1], arg1.v[2] / arg2.v[2] }};
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Abs(FloatArgType value)
        {
            return {{ fabsf(value.v[0]), fabsf(value.v[1]), fabsf(value.v[2]) }};
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::Add(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ arg1.v[0] + arg2.v[0], arg1.v[1] + arg2.v[1], arg1.v[2] + arg2.v[2] }};
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::Sub(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ arg1.v[0] - arg2.v[0], arg1.v[1] - arg2.v[1], arg1.v[2] - arg2.v[2] }};
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::Mul(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ arg1.v[0] * arg2.v[0], arg1.v[1] * arg2.v[1], arg1.v[2] * arg2.v[2] }};
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::Madd(Int32ArgType mul1, Int32ArgType mul2, Int32ArgType add)
        {
            return Add(Mul(mul1, mul2), add);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::Abs(Int32ArgType value)
        {
            return {{ abs(value.v[0]), abs(value.v[1]), abs(value.v[2]) }};
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Not(FloatArgType value)
        {
            Int32Type result = {{ ~reinterpret_cast<const int32_t&>(value.v[0])
                                , ~reinterpret_cast<const int32_t&>(value.v[1])
                                , ~reinterpret_cast<const int32_t&>(value.v[2]) }};
            return CastToFloat(result);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::And(FloatArgType arg1, FloatArgType arg2)
        {
            Int32Type result = {{ reinterpret_cast<const int32_t&>(arg1.v[0]) & reinterpret_cast<const int32_t&>(arg2.v[0])
                                , reinterpret_cast<const int32_t&>(arg1.v[1]) & reinterpret_cast<const int32_t&>(arg2.v[1])
                                , reinterpret_cast<const int32_t&>(arg1.v[2]) & reinterpret_cast<const int32_t&>(arg2.v[2]) }};
            return CastToFloat(result);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::AndNot(FloatArgType arg1, FloatArgType arg2)
        {
            Int32Type result = {{ ~reinterpret_cast<const int32_t&>(arg1.v[0]) & reinterpret_cast<const int32_t&>(arg2.v[0])
                                , ~reinterpret_cast<const int32_t&>(arg1.v[1]) & reinterpret_cast<const int32_t&>(arg2.v[1])
                                , ~reinterpret_cast<const int32_t&>(arg1.v[2]) & reinterpret_cast<const int32_t&>(arg2.v[2]) }};
            return CastToFloat(result);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Or(FloatArgType arg1, FloatArgType arg2)
        {
            Int32Type result = {{ reinterpret_cast<const int32_t&>(arg1.v[0]) | reinterpret_cast<const int32_t&>(arg2.v[0])
                                , reinterpret_cast<const int32_t&>(arg1.v[1]) | reinterpret_cast<const int32_t&>(arg2.v[1])
                                , reinterpret_cast<const int32_t&>(arg1.v[2]) | reinterpret_cast<const int32_t&>(arg2.v[2]) }};
            return CastToFloat(result);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Xor(FloatArgType arg1, FloatArgType arg2)
        {
            Int32Type result = {{ reinterpret_cast<const int32_t&>(arg1.v[0]) ^ reinterpret_cast<const int32_t&>(arg2.v[0])
                                , reinterpret_cast<const int32_t&>(arg1.v[1]) ^ reinterpret_cast<const int32_t&>(arg2.v[1])
                                , reinterpret_cast<const int32_t&>(arg1.v[2]) ^ reinterpret_cast<const int32_t&>(arg2.v[2]) }};
            return CastToFloat(result);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::Not(Int32ArgType value)
        {
            return {{ ~value.v[0], ~value.v[1], ~value.v[2] }};
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::And(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ arg1.v[0] & arg2.v[0], arg1.v[1] & arg2.v[1], arg1.v[2] & arg2.v[2] }};
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::AndNot(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ ~arg1.v[0] & arg2.v[0], ~arg1.v[1] & arg2.v[1], ~arg1.v[2] & arg2.v[2] }};
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::Or(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ arg1.v[0] | arg2.v[0], arg1.v[1] | arg2.v[1], arg1.v[2] | arg2.v[2] }};
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::Xor(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ arg1.v[0] ^ arg2.v[0], arg1.v[1] ^ arg2.v[1], arg1.v[2] ^ arg2.v[2] }};
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Floor(FloatArgType value)
        {
            return {{ AZStd::floorf(value.v[0]), AZStd::floorf(value.v[1]), AZStd::floorf(value.v[2]) }};
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Ceil(FloatArgType value)
        {
            return {{ AZStd::ceilf(value.v[0]), AZStd::ceilf(value.v[1]), AZStd::ceilf(value.v[2]) }};
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Round(FloatArgType value)
        {

            // While 'roundf' may seem the obvious choice here, it rounds halfway cases
            // away from zero regardless of the current rounding mode, but 'rintf' uses
            // the current rounding mode which is consistent with other implementations.
            return {{ AZStd::rintf(value.v[0]), AZStd::rintf(value.v[1]), AZStd::rintf(value.v[2]) }};
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Truncate(FloatArgType value)
        {
            return ConvertToFloat(ConvertToInt(value));
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Min(FloatArgType arg1, FloatArgType arg2)
        {
            return {{ (arg1.v[0] < arg2.v[0]) ? arg1.v[0] : arg2.v[0]
                    , (arg1.v[1] < arg2.v[1]) ? arg1.v[1] : arg2.v[1]
                    , (arg1.v[2] < arg2.v[2]) ? arg1.v[2] : arg2.v[2] }};
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Max(FloatArgType arg1, FloatArgType arg2)
        {
            return {{ (arg1.v[0] > arg2.v[0]) ? arg1.v[0] : arg2.v[0]
                    , (arg1.v[1] > arg2.v[1]) ? arg1.v[1] : arg2.v[1]
                    , (arg1.v[2] > arg2.v[2]) ? arg1.v[2] : arg2.v[2] }};
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Clamp(FloatArgType value, FloatArgType min, FloatArgType max)
        {
            return Max(min, Min(value, max));
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::Min(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ AZStd::min(arg1.v[0], arg2.v[0]), AZStd::min(arg1.v[1], arg2.v[1]), AZStd::min(arg1.v[2], arg2.v[2]) }};
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::Max(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ AZStd::max(arg1.v[0], arg2.v[0]), AZStd::max(arg1.v[1], arg2.v[1]), AZStd::max(arg1.v[2], arg2.v[2]) }};
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::Clamp(Int32ArgType value, Int32ArgType min, Int32ArgType max)
        {
            return Max(min, Min(value, max));
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::CmpEq(FloatArgType arg1, FloatArgType arg2)
        {
            Int32Type result = {{ (arg1.v[0] == arg2.v[0]) ? (int32_t)0xFFFFFFFF : 0x00000000
                                , (arg1.v[1] == arg2.v[1]) ? (int32_t)0xFFFFFFFF : 0x00000000
                                , (arg1.v[2] == arg2.v[2]) ? (int32_t)0xFFFFFFFF : 0x00000000 }};
            return CastToFloat(result);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::CmpNeq(FloatArgType arg1, FloatArgType arg2)
        {
            Int32Type result = {{ (arg1.v[0] != arg2.v[0]) ? (int32_t)0xFFFFFFFF : 0x00000000
                                , (arg1.v[1] != arg2.v[1]) ? (int32_t)0xFFFFFFFF : 0x00000000
                                , (arg1.v[2] != arg2.v[2]) ? (int32_t)0xFFFFFFFF : 0x00000000 }};
            return CastToFloat(result);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::CmpGt(FloatArgType arg1, FloatArgType arg2)
        {
            Int32Type result = {{ (arg1.v[0] > arg2.v[0]) ? (int32_t)0xFFFFFFFF : 0x00000000
                                , (arg1.v[1] > arg2.v[1]) ? (int32_t)0xFFFFFFFF : 0x00000000
                                , (arg1.v[2] > arg2.v[2]) ? (int32_t)0xFFFFFFFF : 0x00000000 }};
            return CastToFloat(result);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::CmpGtEq(FloatArgType arg1, FloatArgType arg2)
        {
            Int32Type result = {{ (arg1.v[0] >= arg2.v[0]) ? (int32_t)0xFFFFFFFF : 0x00000000
                                , (arg1.v[1] >= arg2.v[1]) ? (int32_t)0xFFFFFFFF : 0x00000000
                                , (arg1.v[2] >= arg2.v[2]) ? (int32_t)0xFFFFFFFF : 0x00000000 }};
            return CastToFloat(result);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::CmpLt(FloatArgType arg1, FloatArgType arg2)
        {
            Int32Type result = {{ (arg1.v[0] < arg2.v[0]) ? (int32_t)0xFFFFFFFF : 0x00000000
                                , (arg1.v[1] < arg2.v[1]) ? (int32_t)0xFFFFFFFF : 0x00000000
                                , (arg1.v[2] < arg2.v[2]) ? (int32_t)0xFFFFFFFF : 0x00000000 }};
            return CastToFloat(result);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::CmpLtEq(FloatArgType arg1, FloatArgType arg2)
        {
            Int32Type result = {{ (arg1.v[0] <= arg2.v[0]) ? (int32_t)0xFFFFFFFF : 0x00000000
                                , (arg1.v[1] <= arg2.v[1]) ? (int32_t)0xFFFFFFFF : 0x00000000
                                , (arg1.v[2] <= arg2.v[2]) ? (int32_t)0xFFFFFFFF : 0x00000000 }};
            return CastToFloat(result);
        }

        AZ_MATH_INLINE bool Vec3::CmpAllEq(FloatArgType arg1, FloatArgType arg2)
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

        AZ_MATH_INLINE bool Vec3::CmpAllLt(FloatArgType arg1, FloatArgType arg2)
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

        AZ_MATH_INLINE bool Vec3::CmpAllLtEq(FloatArgType arg1, FloatArgType arg2)
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

        AZ_MATH_INLINE bool Vec3::CmpAllGt(FloatArgType arg1, FloatArgType arg2)
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

        AZ_MATH_INLINE bool Vec3::CmpAllGtEq(FloatArgType arg1, FloatArgType arg2)
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

        AZ_MATH_INLINE Vec3::Int32Type Vec3::CmpEq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ (arg1.v[0] == arg2.v[0]) ? (int32_t)0xFFFFFFFF : 0x00000000
                    , (arg1.v[1] == arg2.v[1]) ? (int32_t)0xFFFFFFFF : 0x00000000
                    , (arg1.v[2] == arg2.v[2]) ? (int32_t)0xFFFFFFFF : 0x00000000 }};
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::CmpNeq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ (arg1.v[0] != arg2.v[0]) ? (int32_t)0xFFFFFFFF : 0x00000000
                    , (arg1.v[1] != arg2.v[1]) ? (int32_t)0xFFFFFFFF : 0x00000000
                    , (arg1.v[2] != arg2.v[2]) ? (int32_t)0xFFFFFFFF : 0x00000000 }};
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::CmpGt(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ (arg1.v[0] > arg2.v[0]) ? (int32_t)0xFFFFFFFF : 0x00000000
                    , (arg1.v[1] > arg2.v[1]) ? (int32_t)0xFFFFFFFF : 0x00000000
                    , (arg1.v[2] > arg2.v[2]) ? (int32_t)0xFFFFFFFF : 0x00000000 }};
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::CmpGtEq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ (arg1.v[0] >= arg2.v[0]) ? (int32_t)0xFFFFFFFF : 0x00000000
                    , (arg1.v[1] >= arg2.v[1]) ? (int32_t)0xFFFFFFFF : 0x00000000
                    , (arg1.v[2] >= arg2.v[2]) ? (int32_t)0xFFFFFFFF : 0x00000000 }};
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::CmpLt(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ (arg1.v[0] < arg2.v[0]) ? (int32_t)0xFFFFFFFF : 0x00000000
                    , (arg1.v[1] < arg2.v[1]) ? (int32_t)0xFFFFFFFF : 0x00000000
                    , (arg1.v[2] < arg2.v[2]) ? (int32_t)0xFFFFFFFF : 0x00000000 }};
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::CmpLtEq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ (arg1.v[0] <= arg2.v[0]) ? (int32_t)0xFFFFFFFF : 0x00000000
                    , (arg1.v[1] <= arg2.v[1]) ? (int32_t)0xFFFFFFFF : 0x00000000
                    , (arg1.v[2] <= arg2.v[2]) ? (int32_t)0xFFFFFFFF : 0x00000000 }};
        }

        AZ_MATH_INLINE bool Vec3::CmpAllEq(Int32ArgType arg1, Int32ArgType arg2)
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

        AZ_MATH_INLINE Vec3::FloatType Vec3::Select(FloatArgType arg1, FloatArgType arg2, FloatArgType mask)
        {
            const Int32Type intMask = CastToInt(mask);
            return {{ (intMask.v[0] == 0) ? arg2.v[0] : arg1.v[0]
                    , (intMask.v[1] == 0) ? arg2.v[1] : arg1.v[1]
                    , (intMask.v[2] == 0) ? arg2.v[2] : arg1.v[2] }};
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::Select(Int32ArgType arg1, Int32ArgType arg2, Int32ArgType mask)
        {
            return {{ (mask.v[0] == 0) ? arg2.v[0] : arg1.v[0]
                    , (mask.v[1] == 0) ? arg2.v[1] : arg1.v[1]
                    , (mask.v[2] == 0) ? arg2.v[2] : arg1.v[2] }};
        }

AZ_PUSH_DISABLE_WARNING(4723, "-Wunknown-warning-option") // potential divide by 0 warning
        AZ_MATH_INLINE Vec3::FloatType Vec3::Reciprocal(FloatArgType value)
        {
            return {{ 1.0f / value.v[0], 1.0f / value.v[1], 1.0f / value.v[2] }};
        }
AZ_POP_DISABLE_WARNING

        AZ_MATH_INLINE Vec3::FloatType Vec3::ReciprocalEstimate(FloatArgType value)
        {
            return Reciprocal(value);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Mod(FloatArgType value, FloatArgType divisor)
        {
            return {{ fmodf(value.v[0], divisor.v[0]), fmodf(value.v[1], divisor.v[1]), fmodf(value.v[2], divisor.v[2]) }};
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Wrap(FloatArgType value, FloatArgType minValue, FloatArgType maxValue)
        {
            return {{ Scalar::Wrap(value.v[0], minValue.v[0], maxValue.v[0])
                    , Scalar::Wrap(value.v[1], minValue.v[1], maxValue.v[1])
                    , Scalar::Wrap(value.v[2], minValue.v[2], maxValue.v[2]) }};
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::AngleMod(FloatArgType value)
        {
            return {{ Scalar::AngleMod(value.v[0]), Scalar::AngleMod(value.v[1]), Scalar::AngleMod(value.v[2]) }};
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Sqrt(FloatArgType value)
        {
            return {{ sqrtf(value.v[0]), sqrtf(value.v[1]), sqrtf(value.v[2]) }};
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::SqrtEstimate(FloatArgType value)
        {
            return Sqrt(value);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::SqrtInv(FloatArgType value)
        {
            return {{ 1.0f / sqrtf(value.v[0]), 1.0f / sqrtf(value.v[1]), 1.0f / sqrtf(value.v[2]) }};
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::SqrtInvEstimate(FloatArgType value)
        {
            return SqrtInv(value);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Sin(FloatArgType value)
        {
            return {{ sinf(value.v[0]), sinf(value.v[1]), sinf(value.v[2]) }};
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Cos(FloatArgType value)
        {
            return {{ cosf(value.v[0]), cosf(value.v[1]), cosf(value.v[2]) }};
        }

        AZ_MATH_INLINE void Vec3::SinCos(FloatArgType value, FloatType& sin, FloatType& cos)
        {
            sin = Sin(value);
            cos = Cos(value);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Acos(FloatArgType value)
        {
            return {{ acosf(value.v[0]), acosf(value.v[1]), acosf(value.v[2]) }};
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Atan(FloatArgType value)
        {
            return {{ atanf(value.v[0]), atanf(value.v[1]), atanf(value.v[2]) }};
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Atan2(FloatArgType y, FloatArgType x)
        {
            return {{ atan2f(y.v[0], x.v[0]), atan2f(y.v[1], x.v[1]), atan2f(y.v[2], x.v[2]) }};
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::ExpEstimate(FloatArgType x)
        {
            return {{ Vec1::ExpEstimate(x.v[0]), Vec1::ExpEstimate(x.v[1]), Vec1::ExpEstimate(x.v[2]) }};
        }

        AZ_MATH_INLINE Vec1::FloatType Vec3::Dot(FloatArgType arg1, FloatArgType arg2)
        {
            return (arg1.v[0] * arg2.v[0]) + (arg1.v[1] * arg2.v[1]) + (arg1.v[2] * arg2.v[2]);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Cross(FloatArgType arg1, FloatArgType arg2)
        {
            return {{ arg1.v[1] * arg2.v[2] - arg1.v[2] * arg2.v[1], arg1.v[2] * arg2.v[0] - arg1.v[0] * arg2.v[2], arg1.v[0] * arg2.v[1] - arg1.v[1] * arg2.v[0] }};
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Normalize(FloatArgType value)
        {
            const float invLength = 1.0f / sqrtf(Dot(value, value));
            return {{ value.v[0] * invLength, value.v[1] * invLength, value.v[2] * invLength }};
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::NormalizeEstimate(FloatArgType value)
        {
            return Normalize(value);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::NormalizeSafe(FloatArgType value, float tolerance)
        {
            const float sqLength = Dot(value, value);
            if (sqLength <= (tolerance * tolerance))
            {
                return ZeroFloat();
            }
            else
            {
                const float invLength = 1.0f / sqrtf(sqLength);
                return {{ value.v[0] * invLength, value.v[1] * invLength, value.v[2] * invLength }};
            }
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::NormalizeSafeEstimate(FloatArgType value, float tolerance)
        {
            return NormalizeSafe(value, tolerance);
        }

        AZ_MATH_INLINE void Vec3::Mat3x3Inverse(const FloatType* __restrict rows, FloatType* __restrict out)
        {
            // Cache these in case rows and out are aliased
            const float rows00 = rows[0].v[0];
            const float rows01 = rows[0].v[1];
            const float rows02 = rows[0].v[2];

            Mat3x3Adjugate(rows, out);

            // Calculate the determinant
            const float det = rows00 * out[0].v[0] + rows01 * out[1].v[0] + rows02 * out[2].v[0];

            // Divide cofactors by determinant
            const float f = (det == 0.0f) ? 10000000.0f : 1.0f / det;
            out[0].v[0] *= f;
            out[1].v[0] *= f;
            out[2].v[0] *= f;
            out[0].v[1] *= f;
            out[1].v[1] *= f;
            out[2].v[1] *= f;
            out[0].v[2] *= f;
            out[1].v[2] *= f;
            out[2].v[2] *= f;
        }

        AZ_MATH_INLINE void Vec3::Mat3x3Adjugate(const FloatType* __restrict rows, FloatType* __restrict out)
        {
            // 2-phase, in case rows and out are aliased
            const FloatType adj0 = {{ (rows[1].v[1] * rows[2].v[2] - rows[2].v[1] * rows[1].v[2])
                                    , (rows[2].v[1] * rows[0].v[2] - rows[0].v[1] * rows[2].v[2])
                                    , (rows[0].v[1] * rows[1].v[2] - rows[1].v[1] * rows[0].v[2]) }};
            const FloatType adj1 = {{ (rows[1].v[2] * rows[2].v[0] - rows[2].v[2] * rows[1].v[0])
                                    , (rows[2].v[2] * rows[0].v[0] - rows[0].v[2] * rows[2].v[0])
                                    , (rows[0].v[2] * rows[1].v[0] - rows[1].v[2] * rows[0].v[0]) }};
            const FloatType adj2 = {{ (rows[1].v[0] * rows[2].v[1] - rows[2].v[0] * rows[1].v[1])
                                    , (rows[2].v[0] * rows[0].v[1] - rows[0].v[0] * rows[2].v[1])
                                    , (rows[0].v[0] * rows[1].v[1] - rows[1].v[0] * rows[0].v[1]) }};
            out[0] = adj0;
            out[1] = adj1;
            out[2] = adj2;
        }

        AZ_MATH_INLINE void Vec3::Mat3x3Transpose(const FloatType* __restrict rows, FloatType* __restrict out)
        {
            // 2-phase, in case rows and out are aliased
            const FloatType cols0 = {{ rows[0].v[0], rows[1].v[0], rows[2].v[0] }};
            const FloatType cols1 = {{ rows[0].v[1], rows[1].v[1], rows[2].v[1] }};
            const FloatType cols2 = {{ rows[0].v[2], rows[1].v[2], rows[2].v[2] }};
            out[0] = cols0;
            out[1] = cols1;
            out[2] = cols2;
        }

        AZ_MATH_INLINE void Vec3::Mat3x3Multiply(const FloatType* __restrict rowsA, const FloatType* __restrict rowsB, FloatType* __restrict out)
        {
            for (int32_t row = 0; row < 3; ++row)
            {
                for (int32_t col = 0; col < 3; col++)
                {
                    out[row].v[col] = 0.0f;
                    for (int32_t k = 0; k < 3; ++k)
                    {
                        out[row].v[col] += rowsA[row].v[k] * rowsB[k].v[col];
                    }
                }
            }
        }

        AZ_MATH_INLINE void Vec3::Mat3x3TransposeMultiply(const FloatType* __restrict rowsA, const FloatType* __restrict rowsB, FloatType* __restrict out)
        {
            const FloatType bc0 = {{ rowsB[0].v[0], rowsB[1].v[0], rowsB[2].v[0] }};
            const FloatType bc1 = {{ rowsB[0].v[1], rowsB[1].v[1], rowsB[2].v[1] }};
            const FloatType bc2 = {{ rowsB[0].v[2], rowsB[1].v[2], rowsB[2].v[2] }};

            const FloatType ac0 = {{ rowsA[0].v[0], rowsA[1].v[0], rowsA[2].v[0] }};
            const FloatType ac1 = {{ rowsA[0].v[1], rowsA[1].v[1], rowsA[2].v[1] }};
            const FloatType ac2 = {{ rowsA[0].v[2], rowsA[1].v[2], rowsA[2].v[2] }};

            out[0] = {{ Dot(ac0, bc0), Dot(ac0, bc1), Dot(ac0, bc2) }};
            out[1] = {{ Dot(ac1, bc0), Dot(ac1, bc1), Dot(ac1, bc2) }};
            out[2] = {{ Dot(ac2, bc0), Dot(ac2, bc1), Dot(ac2, bc2) }};
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Mat3x3TransformVector(const FloatType* __restrict rows, FloatArgType vector)
        {
            return {{ rows[0].v[0] * vector.v[0] + rows[0].v[1] * vector.v[1] + rows[0].v[2] * vector.v[2]
                    , rows[1].v[0] * vector.v[0] + rows[1].v[1] * vector.v[1] + rows[1].v[2] * vector.v[2]
                    , rows[2].v[0] * vector.v[0] + rows[2].v[1] * vector.v[1] + rows[2].v[2] * vector.v[2] }};
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Mat3x3TransposeTransformVector(const FloatType* __restrict rows, FloatArgType vector)
        {
            return {{ rows[0].v[0] * vector.v[0] + rows[1].v[0] * vector.v[1] + rows[2].v[0] * vector.v[2]
                    , rows[0].v[1] * vector.v[0] + rows[1].v[1] * vector.v[1] + rows[2].v[1] * vector.v[2]
                    , rows[0].v[2] * vector.v[0] + rows[1].v[2] * vector.v[1] + rows[2].v[2] * vector.v[2] }};
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::ConvertToFloat(Int32ArgType value)
        {
            return {{ static_cast<float>(value.v[0])
                    , static_cast<float>(value.v[1])
                    , static_cast<float>(value.v[2]) }};
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::ConvertToInt(FloatArgType value)
        {
            return {{ static_cast<int32_t>(value.v[0])
                    , static_cast<int32_t>(value.v[1])
                    , static_cast<int32_t>(value.v[2]) }};
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::ConvertToIntNearest(FloatArgType value)
        {
            return ConvertToInt(Round(value));
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::CastToFloat(Int32ArgType value)
        {
            return {{ reinterpret_cast<const float&>(value.v[0])
                    , reinterpret_cast<const float&>(value.v[1])
                    , reinterpret_cast<const float&>(value.v[2]) }};
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::CastToInt(FloatArgType value)
        {
            return {{ reinterpret_cast<const int32_t&>(value.v[0])
                    , reinterpret_cast<const int32_t&>(value.v[1])
                    , reinterpret_cast<const int32_t&>(value.v[2]) }};
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::ZeroFloat()
        {
            return {{ 0.0f, 0.0f, 0.0f }};
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::ZeroInt()
        {
            return {{ 0, 0, 0 }};
        }
    }
}
