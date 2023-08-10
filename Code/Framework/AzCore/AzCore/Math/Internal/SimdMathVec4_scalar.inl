/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Internal/SimdMathCommon_scalar.inl>

// Unity builds on windows using the scalar backend are tripping some really strange warning behavior..
// Disable the warning so we can test the scalar implementation with unity on windows
AZ_PUSH_DISABLE_WARNING(4723, "-Wunknown-warning-option") // Potential divide by zero

namespace AZ
{
    namespace Simd
    {
        AZ_MATH_INLINE Vec1::FloatType Vec4::ToVec1(FloatArgType value)
        {
            return value.v[0];
        }

        AZ_MATH_INLINE Vec2::FloatType Vec4::ToVec2(FloatArgType value)
        {
            return {{ value.v[0], value.v[1] }};
        }

        AZ_MATH_INLINE Vec3::FloatType Vec4::ToVec3(FloatArgType value)
        {
            return {{ value.v[0], value.v[1], value.v[2] }};
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::FromVec1(Vec1::FloatArgType value)
        {
            return {{ value, value, value, value }};
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::FromVec2(Vec2::FloatArgType value)
        {
            return {{ value.v[0], value.v[1], 0.0f, 0.0f }};
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::FromVec3(Vec3::FloatArgType value)
        {
            return {{ value.v[0], value.v[1], value.v[2], 0.0f }};
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::LoadAligned(const float* __restrict addr)
        {
            return LoadUnaligned(addr);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::LoadAligned(const int32_t* __restrict addr)
        {
            return LoadUnaligned(addr);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::LoadUnaligned(const float* __restrict addr)
        {
            FloatType result = {{ addr[0], addr[1], addr[2], addr[3] }};
            return result;
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::LoadUnaligned(const int32_t* __restrict addr)
        {
            Int32Type result = {{ addr[0], addr[1], addr[2], addr[3] }};
            return result;
        }

        AZ_MATH_INLINE void Vec4::StoreAligned(float* __restrict addr, FloatArgType value)
        {
            return StoreUnaligned(addr, value);
        }

        AZ_MATH_INLINE void Vec4::StoreAligned(int32_t* __restrict addr, Int32ArgType value)
        {
            return StoreUnaligned(addr, value);
        }

        AZ_MATH_INLINE void Vec4::StoreUnaligned(float* __restrict addr, FloatArgType value)
        {
            addr[0] = value.v[0];
            addr[1] = value.v[1];
            addr[2] = value.v[2];
            addr[3] = value.v[3];
        }

        AZ_MATH_INLINE void Vec4::StoreUnaligned(int32_t* __restrict addr, Int32ArgType value)
        {
            addr[0] = value.v[0];
            addr[1] = value.v[1];
            addr[2] = value.v[2];
            addr[3] = value.v[3];
        }

        AZ_MATH_INLINE void Vec4::StreamAligned(float* __restrict addr, FloatArgType value)
        {
            return StoreUnaligned(addr, value);
        }

        AZ_MATH_INLINE void Vec4::StreamAligned(int32_t* __restrict addr, Int32ArgType value)
        {
            return StoreUnaligned(addr, value);
        }

        AZ_MATH_INLINE float Vec4::SelectIndex0(FloatArgType value)
        {
            return value.v[0];
        }

        AZ_MATH_INLINE float Vec4::SelectIndex1(FloatArgType value)
        {
            return value.v[1];
        }

        AZ_MATH_INLINE float Vec4::SelectIndex2(FloatArgType value)
        {
            return value.v[2];
        }

        AZ_MATH_INLINE float Vec4::SelectIndex3(FloatArgType value)
        {
            return value.v[3];
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Splat(float value)
        {
            return LoadImmediate(value, value, value, value);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::Splat(int32_t value)
        {
            return LoadImmediate(value, value, value, value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::SplatIndex0(FloatArgType value)
        {
            return Splat(value.v[0]);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::SplatIndex1(FloatArgType value)
        {
            return Splat(value.v[1]);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::SplatIndex2(FloatArgType value)
        {
            return Splat(value.v[2]);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::SplatIndex3(FloatArgType value)
        {
            return Splat(value.v[3]);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::ReplaceIndex0(FloatArgType a, float b)
        {
            return {{ b, a.v[1], a.v[2], a.v[3] }};
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::ReplaceIndex0(FloatArgType a, FloatArgType b)
        {
            return {{ b.v[0], a.v[1], a.v[2], a.v[3] }};
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::ReplaceIndex1(FloatArgType a, float b)
        {
            return {{ a.v[0], b, a.v[2], a.v[3] }};
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::ReplaceIndex1(FloatArgType a, FloatArgType b)
        {
            return {{ a.v[0], b.v[1], a.v[2], a.v[3] }};
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::ReplaceIndex2(FloatArgType a, float b)
        {
            return {{ a.v[0], a.v[1], b, a.v[3] }};
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::ReplaceIndex2(FloatArgType a, FloatArgType b)
        {
            return {{ a.v[0], a.v[1], b.v[2], a.v[3] }};
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::ReplaceIndex3(FloatArgType a, float b)
        {
            return {{ a.v[0], a.v[1], a.v[2], b }};
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::ReplaceIndex3(FloatArgType a, FloatArgType b)
        {
            return {{ a.v[0], a.v[1], a.v[2], b.v[3] }};
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::LoadImmediate(float x, float y, float z, float w)
        {
            FloatType result = {{ x, y, z, w }};
            return result;
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::LoadImmediate(int32_t x, int32_t y, int32_t z, int32_t w)
        {
            Int32Type result = {{ x, y, z, w }};
            return result;
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Add(FloatArgType arg1, FloatArgType arg2)
        {
            return {{ arg1.v[0] + arg2.v[0], arg1.v[1] + arg2.v[1], arg1.v[2] + arg2.v[2], arg1.v[3] + arg2.v[3] }};
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Sub(FloatArgType arg1, FloatArgType arg2)
        {
            return {{ arg1.v[0] - arg2.v[0], arg1.v[1] - arg2.v[1], arg1.v[2] - arg2.v[2], arg1.v[3] - arg2.v[3] }};
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Mul(FloatArgType arg1, FloatArgType arg2)
        {
            return {{ arg1.v[0] * arg2.v[0], arg1.v[1] * arg2.v[1], arg1.v[2] * arg2.v[2], arg1.v[3] * arg2.v[3] }};
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Madd(FloatArgType mul1, FloatArgType mul2, FloatArgType add)
        {
            return Add(Mul(mul1, mul2), add);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Div(FloatArgType arg1, FloatArgType arg2)
        {
            return {{ arg1.v[0] / arg2.v[0], arg1.v[1] / arg2.v[1], arg1.v[2] / arg2.v[2], arg1.v[3] / arg2.v[3] }};
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Abs(FloatArgType value)
        {
            return {{ fabsf(value.v[0]), fabsf(value.v[1]), fabsf(value.v[2]), fabsf(value.v[3]) }};
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::Add(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ arg1.v[0] + arg2.v[0], arg1.v[1] + arg2.v[1], arg1.v[2] + arg2.v[2], arg1.v[3] + arg2.v[3] }};
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::Sub(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ arg1.v[0] - arg2.v[0], arg1.v[1] - arg2.v[1], arg1.v[2] - arg2.v[2], arg1.v[3] - arg2.v[3] }};
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::Mul(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ arg1.v[0] * arg2.v[0], arg1.v[1] * arg2.v[1], arg1.v[2] * arg2.v[2], arg1.v[3] * arg2.v[3] }};
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::Madd(Int32ArgType mul1, Int32ArgType mul2, Int32ArgType add)
        {
            return Add(Mul(mul1, mul2), add);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::Abs(Int32ArgType value)
        {
            return {{ abs(value.v[0]), abs(value.v[1]), abs(value.v[2]), abs(value.v[3]) }};
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Not(FloatArgType value)
        {
            Int32Type result = {{ ~reinterpret_cast<const int32_t&>(value.v[0])
                                , ~reinterpret_cast<const int32_t&>(value.v[1])
                                , ~reinterpret_cast<const int32_t&>(value.v[2])
                                , ~reinterpret_cast<const int32_t&>(value.v[3]) }};
            return CastToFloat(result);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::And(FloatArgType arg1, FloatArgType arg2)
        {
            Int32Type result = {{ reinterpret_cast<const int32_t&>(arg1.v[0]) & reinterpret_cast<const int32_t&>(arg2.v[0])
                                , reinterpret_cast<const int32_t&>(arg1.v[1]) & reinterpret_cast<const int32_t&>(arg2.v[1])
                                , reinterpret_cast<const int32_t&>(arg1.v[2]) & reinterpret_cast<const int32_t&>(arg2.v[2])
                                , reinterpret_cast<const int32_t&>(arg1.v[3]) & reinterpret_cast<const int32_t&>(arg2.v[3]) }};
            return CastToFloat(result);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::AndNot(FloatArgType arg1, FloatArgType arg2)
        {
            Int32Type result = {{ ~reinterpret_cast<const int32_t&>(arg1.v[0]) & reinterpret_cast<const int32_t&>(arg2.v[0])
                                , ~reinterpret_cast<const int32_t&>(arg1.v[1]) & reinterpret_cast<const int32_t&>(arg2.v[1])
                                , ~reinterpret_cast<const int32_t&>(arg1.v[2]) & reinterpret_cast<const int32_t&>(arg2.v[2])
                                , ~reinterpret_cast<const int32_t&>(arg1.v[3]) & reinterpret_cast<const int32_t&>(arg2.v[3]) }};
            return CastToFloat(result);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Or (FloatArgType arg1, FloatArgType arg2)
        {
            Int32Type result = {{ reinterpret_cast<const int32_t&>(arg1.v[0]) | reinterpret_cast<const int32_t&>(arg2.v[0])
                                , reinterpret_cast<const int32_t&>(arg1.v[1]) | reinterpret_cast<const int32_t&>(arg2.v[1])
                                , reinterpret_cast<const int32_t&>(arg1.v[2]) | reinterpret_cast<const int32_t&>(arg2.v[2])
                                , reinterpret_cast<const int32_t&>(arg1.v[3]) | reinterpret_cast<const int32_t&>(arg2.v[3]) }};
            return CastToFloat(result);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Xor(FloatArgType arg1, FloatArgType arg2)
        {
            Int32Type result = {{ reinterpret_cast<const int32_t&>(arg1.v[0]) ^ reinterpret_cast<const int32_t&>(arg2.v[0])
                                , reinterpret_cast<const int32_t&>(arg1.v[1]) ^ reinterpret_cast<const int32_t&>(arg2.v[1])
                                , reinterpret_cast<const int32_t&>(arg1.v[2]) ^ reinterpret_cast<const int32_t&>(arg2.v[2])
                                , reinterpret_cast<const int32_t&>(arg1.v[3]) ^ reinterpret_cast<const int32_t&>(arg2.v[3]) }};
            return CastToFloat(result);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::Not(Int32ArgType value)
        {
            return {{ ~value.v[0], ~value.v[1], ~value.v[2], ~value.v[3] }};
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::And(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ arg1.v[0] & arg2.v[0], arg1.v[1] & arg2.v[1], arg1.v[2] & arg2.v[2], arg1.v[3] & arg2.v[3] }};
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::AndNot(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ ~arg1.v[0] & arg2.v[0], ~arg1.v[1] & arg2.v[1], ~arg1.v[2] & arg2.v[2], ~arg1.v[3] & arg2.v[3] }};
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::Or(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ arg1.v[0] | arg2.v[0], arg1.v[1] | arg2.v[1], arg1.v[2] | arg2.v[2], arg1.v[3] | arg2.v[3] }};
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::Xor(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ arg1.v[0] ^ arg2.v[0], arg1.v[1] ^ arg2.v[1], arg1.v[2] ^ arg2.v[2], arg1.v[3] ^ arg2.v[3] }};
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Floor(FloatArgType value)
        {
            return {{ AZStd::floorf(value.v[0]), AZStd::floorf(value.v[1]), AZStd::floorf(value.v[2]), AZStd::floorf(value.v[3]) }};
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Ceil(FloatArgType value)
        {
            return {{ AZStd::ceilf(value.v[0]), AZStd::ceilf(value.v[1]), AZStd::ceilf(value.v[2]), AZStd::ceilf(value.v[3]) }};
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Round(FloatArgType value)
        {

            // While 'roundf' may seem the obvious choice here, it rounds halfway cases
            // away from zero regardless of the current rounding mode, but 'rintf' uses
            // the current rounding mode which is consistent with other implementations.
            return {{ AZStd::rintf(value.v[0]), AZStd::rintf(value.v[1]), AZStd::rintf(value.v[2]), AZStd::rintf(value.v[3]) }};
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Truncate(FloatArgType value)
        {
            return ConvertToFloat(ConvertToInt(value));
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Min(FloatArgType arg1, FloatArgType arg2)
        {
            return {{ AZStd::min(arg1.v[0], arg2.v[0]), AZStd::min(arg1.v[1], arg2.v[1]), AZStd::min(arg1.v[2], arg2.v[2]), AZStd::min(arg1.v[3], arg2.v[3]) }};
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Max(FloatArgType arg1, FloatArgType arg2)
        {
            return {{ AZStd::max(arg1.v[0], arg2.v[0]), AZStd::max(arg1.v[1], arg2.v[1]), AZStd::max(arg1.v[2], arg2.v[2]), AZStd::max(arg1.v[3], arg2.v[3]) }};
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Clamp(FloatArgType value, FloatArgType min, FloatArgType max)
        {
            return Max(min, Min(value, max));
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::Min(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ AZStd::min(arg1.v[0], arg2.v[0]), AZStd::min(arg1.v[1], arg2.v[1]), AZStd::min(arg1.v[2], arg2.v[2]), AZStd::min(arg1.v[3], arg2.v[3]) }};
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::Max(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ AZStd::max(arg1.v[0], arg2.v[0]), AZStd::max(arg1.v[1], arg2.v[1]), AZStd::max(arg1.v[2], arg2.v[2]), AZStd::max(arg1.v[3], arg2.v[3]) }};
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::Clamp(Int32ArgType value, Int32ArgType min, Int32ArgType max)
        {
            return Max(min, Min(value, max));
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::CmpEq(FloatArgType arg1, FloatArgType arg2)
        {
            Int32Type result = {{ (arg1.v[0] == arg2.v[0]) ? (int32_t)0xFFFFFFFF : 0x00000000
                                , (arg1.v[1] == arg2.v[1]) ? (int32_t)0xFFFFFFFF : 0x00000000
                                , (arg1.v[2] == arg2.v[2]) ? (int32_t)0xFFFFFFFF : 0x00000000
                                , (arg1.v[3] == arg2.v[3]) ? (int32_t)0xFFFFFFFF : 0x00000000 }};
            return CastToFloat(result);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::CmpNeq(FloatArgType arg1, FloatArgType arg2)
        {
            Int32Type result = {{ (arg1.v[0] != arg2.v[0]) ? (int32_t)0xFFFFFFFF : 0x00000000
                                , (arg1.v[1] != arg2.v[1]) ? (int32_t)0xFFFFFFFF : 0x00000000
                                , (arg1.v[2] != arg2.v[2]) ? (int32_t)0xFFFFFFFF : 0x00000000
                                , (arg1.v[3] != arg2.v[3]) ? (int32_t)0xFFFFFFFF : 0x00000000 }};
            return CastToFloat(result);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::CmpGt(FloatArgType arg1, FloatArgType arg2)
        {
            Int32Type result = {{ (arg1.v[0] > arg2.v[0]) ? (int32_t)0xFFFFFFFF : 0x00000000
                                , (arg1.v[1] > arg2.v[1]) ? (int32_t)0xFFFFFFFF : 0x00000000
                                , (arg1.v[2] > arg2.v[2]) ? (int32_t)0xFFFFFFFF : 0x00000000
                                , (arg1.v[3] > arg2.v[3]) ? (int32_t)0xFFFFFFFF : 0x00000000 }};
            return CastToFloat(result);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::CmpGtEq(FloatArgType arg1, FloatArgType arg2)
        {
            Int32Type result = {{ (arg1.v[0] >= arg2.v[0]) ? (int32_t)0xFFFFFFFF : 0x00000000
                                , (arg1.v[1] >= arg2.v[1]) ? (int32_t)0xFFFFFFFF : 0x00000000
                                , (arg1.v[2] >= arg2.v[2]) ? (int32_t)0xFFFFFFFF : 0x00000000
                                , (arg1.v[3] >= arg2.v[3]) ? (int32_t)0xFFFFFFFF : 0x00000000 }};
            return CastToFloat(result);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::CmpLt(FloatArgType arg1, FloatArgType arg2)
        {
            Int32Type result = {{ (arg1.v[0] < arg2.v[0]) ? (int32_t)0xFFFFFFFF : 0x00000000
                                , (arg1.v[1] < arg2.v[1]) ? (int32_t)0xFFFFFFFF : 0x00000000
                                , (arg1.v[2] < arg2.v[2]) ? (int32_t)0xFFFFFFFF : 0x00000000
                                , (arg1.v[3] < arg2.v[3]) ? (int32_t)0xFFFFFFFF : 0x00000000 }};
            return CastToFloat(result);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::CmpLtEq(FloatArgType arg1, FloatArgType arg2)
        {
            Int32Type result = {{ (arg1.v[0] <= arg2.v[0]) ? (int32_t)0xFFFFFFFF : 0x00000000
                                , (arg1.v[1] <= arg2.v[1]) ? (int32_t)0xFFFFFFFF : 0x00000000
                                , (arg1.v[2] <= arg2.v[2]) ? (int32_t)0xFFFFFFFF : 0x00000000
                                , (arg1.v[3] <= arg2.v[3]) ? (int32_t)0xFFFFFFFF : 0x00000000 }};
            return CastToFloat(result);
        }

        AZ_MATH_INLINE bool Vec4::CmpAllEq(FloatArgType arg1, FloatArgType arg2)
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

        AZ_MATH_INLINE bool Vec4::CmpAllLt(FloatArgType arg1, FloatArgType arg2)
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

        AZ_MATH_INLINE bool Vec4::CmpAllLtEq(FloatArgType arg1, FloatArgType arg2)
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

        AZ_MATH_INLINE bool Vec4::CmpAllGt(FloatArgType arg1, FloatArgType arg2)
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

        AZ_MATH_INLINE bool Vec4::CmpAllGtEq(FloatArgType arg1, FloatArgType arg2)
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

        AZ_MATH_INLINE Vec4::Int32Type Vec4::CmpEq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ (arg1.v[0] == arg2.v[0]) ? (int32_t)0xFFFFFFFF : 0x00000000
                    , (arg1.v[1] == arg2.v[1]) ? (int32_t)0xFFFFFFFF : 0x00000000
                    , (arg1.v[2] == arg2.v[2]) ? (int32_t)0xFFFFFFFF : 0x00000000
                    , (arg1.v[3] == arg2.v[3]) ? (int32_t)0xFFFFFFFF : 0x00000000 }};
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::CmpNeq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ (arg1.v[0] != arg2.v[0]) ? (int32_t)0xFFFFFFFF : 0x00000000
                    , (arg1.v[1] != arg2.v[1]) ? (int32_t)0xFFFFFFFF : 0x00000000
                    , (arg1.v[2] != arg2.v[2]) ? (int32_t)0xFFFFFFFF : 0x00000000
                    , (arg1.v[3] != arg2.v[3]) ? (int32_t)0xFFFFFFFF : 0x00000000 }};
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::CmpGt(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ (arg1.v[0] > arg2.v[0]) ? (int32_t)0xFFFFFFFF : 0x00000000
                    , (arg1.v[1] > arg2.v[1]) ? (int32_t)0xFFFFFFFF : 0x00000000
                    , (arg1.v[2] > arg2.v[2]) ? (int32_t)0xFFFFFFFF : 0x00000000
                    , (arg1.v[3] > arg2.v[3]) ? (int32_t)0xFFFFFFFF : 0x00000000 }};
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::CmpGtEq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ (arg1.v[0] >= arg2.v[0]) ? (int32_t)0xFFFFFFFF : 0x00000000
                    , (arg1.v[1] >= arg2.v[1]) ? (int32_t)0xFFFFFFFF : 0x00000000
                    , (arg1.v[2] >= arg2.v[2]) ? (int32_t)0xFFFFFFFF : 0x00000000
                    , (arg1.v[3] >= arg2.v[3]) ? (int32_t)0xFFFFFFFF : 0x00000000 }};
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::CmpLt(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ (arg1.v[0] < arg2.v[0]) ? (int32_t)0xFFFFFFFF : 0x00000000
                    , (arg1.v[1] < arg2.v[1]) ? (int32_t)0xFFFFFFFF : 0x00000000
                    , (arg1.v[2] < arg2.v[2]) ? (int32_t)0xFFFFFFFF : 0x00000000
                    , (arg1.v[3] < arg2.v[3]) ? (int32_t)0xFFFFFFFF : 0x00000000 }};
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::CmpLtEq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return {{ (arg1.v[0] <= arg2.v[0]) ? (int32_t)0xFFFFFFFF : 0x00000000
                    , (arg1.v[1] <= arg2.v[1]) ? (int32_t)0xFFFFFFFF : 0x00000000
                    , (arg1.v[2] <= arg2.v[2]) ? (int32_t)0xFFFFFFFF : 0x00000000
                    , (arg1.v[3] <= arg2.v[3]) ? (int32_t)0xFFFFFFFF : 0x00000000 }};
        }

        AZ_MATH_INLINE bool Vec4::CmpAllEq(Int32ArgType arg1, Int32ArgType arg2)
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

        AZ_MATH_INLINE Vec4::FloatType Vec4::Select(FloatArgType arg1, FloatArgType arg2, FloatArgType mask)
        {
            const Int32Type intMask = CastToInt(mask);
            return {{ (intMask.v[0] == 0) ? arg2.v[0] : arg1.v[0]
                    , (intMask.v[1] == 0) ? arg2.v[1] : arg1.v[1]
                    , (intMask.v[2] == 0) ? arg2.v[2] : arg1.v[2]
                    , (intMask.v[3] == 0) ? arg2.v[3] : arg1.v[3] }};
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::Select(Int32ArgType arg1, Int32ArgType arg2, Int32ArgType mask)
        {
            return {{ (mask.v[0] == 0) ? arg2.v[0] : arg1.v[0]
                    , (mask.v[1] == 0) ? arg2.v[1] : arg1.v[1]
                    , (mask.v[2] == 0) ? arg2.v[2] : arg1.v[2]
                    , (mask.v[3] == 0) ? arg2.v[3] : arg1.v[3] }};
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Reciprocal(FloatArgType value)
        {
            return {{ 1.0f / value.v[0], 1.0f / value.v[1], 1.0f / value.v[2], 1.0f / value.v[3] }};
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::ReciprocalEstimate(FloatArgType value)
        {
            return Reciprocal(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Mod(FloatArgType value, FloatArgType divisor)
        {
            return {{ fmodf(value.v[0], divisor.v[0]), fmodf(value.v[1], divisor.v[1]), fmodf(value.v[2], divisor.v[2]), fmodf(value.v[3], divisor.v[3]) }};
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Wrap(FloatArgType value, FloatArgType minValue, FloatArgType maxValue)
        {
            return {{ Scalar::Wrap(value.v[0], minValue.v[0], maxValue.v[0])
                    , Scalar::Wrap(value.v[1], minValue.v[1], maxValue.v[1])
                    , Scalar::Wrap(value.v[2], minValue.v[2], maxValue.v[2])
                    , Scalar::Wrap(value.v[3], minValue.v[3], maxValue.v[3]) }};
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::AngleMod(FloatArgType value)
        {
            return {{ Scalar::AngleMod(value.v[0]), Scalar::AngleMod(value.v[1]), Scalar::AngleMod(value.v[2]), Scalar::AngleMod(value.v[3]) }};
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Sqrt(FloatArgType value)
        {
            return {{ sqrtf(value.v[0]), sqrtf(value.v[1]), sqrtf(value.v[2]), sqrtf(value.v[3]) }};
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::SqrtEstimate(FloatArgType value)
        {
            return Sqrt(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::SqrtInv(FloatArgType value)
        {
            return {{ 1.0f / sqrtf(value.v[0]), 1.0f / sqrtf(value.v[1]), 1.0f / sqrtf(value.v[2]), 1.0f / sqrtf(value.v[3]) }};
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::SqrtInvEstimate(FloatArgType value)
        {
            return SqrtInv(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Sin(FloatArgType value)
        {
            return {{ sinf(value.v[0]), sinf(value.v[1]), sinf(value.v[2]), sinf(value.v[3]) }};
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Cos(FloatArgType value)
        {
            return {{ cosf(value.v[0]), cosf(value.v[1]), cosf(value.v[2]), cosf(value.v[3]) }};
        }

        AZ_MATH_INLINE void Vec4::SinCos(FloatArgType value, FloatType& sin, FloatType& cos)
        {
            sin = Sin(value);
            cos = Cos(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::SinCos(FloatArgType angles)
        {
            return {{ sinf(angles.v[0]), cosf(angles.v[1]), sinf(angles.v[2]), cosf(angles.v[3]) }};
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Acos(FloatArgType value)
        {
            return {{ acosf(value.v[0]), acosf(value.v[1]), acosf(value.v[2]), acosf(value.v[3]) }};
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Atan(FloatArgType value)
        {
            return {{ atanf(value.v[0]), atanf(value.v[1]), atanf(value.v[2]), atanf(value.v[3]) }};
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Atan2(FloatArgType y, FloatArgType x)
        {
            return {{ atan2f(y.v[0], x.v[0]), atan2f(y.v[1], x.v[1]), atan2f(y.v[2], x.v[2]), atan2f(y.v[3], x.v[3]) }};
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::ExpEstimate(FloatArgType x)
        {
            return {{ Vec1::ExpEstimate(x.v[0]), Vec1::ExpEstimate(x.v[1]), Vec1::ExpEstimate(x.v[2]), Vec1::ExpEstimate(x.v[3]) }};
        }

        AZ_MATH_INLINE Vec1::FloatType Vec4::Dot(FloatArgType arg1, FloatArgType arg2)
        {
            return (arg1.v[0] * arg2.v[0]) + (arg1.v[1] * arg2.v[1]) + (arg1.v[2] * arg2.v[2]) + (arg1.v[3] * arg2.v[3]);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Normalize(FloatArgType value)
        {
            const float invLength = 1.0f / sqrtf(Dot(value, value));
            return {{ value.v[0] * invLength, value.v[1] * invLength, value.v[2] * invLength, value.v[3] * invLength }};
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::NormalizeEstimate(FloatArgType value)
        {
            return Normalize(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::NormalizeSafe(FloatArgType value, float tolerance)
        {
            const float sqLength = Dot(value, value);
            if (sqLength <= (tolerance * tolerance))
            {
                return ZeroFloat();
            }
            else
            {
                const float invLength = 1.0f / sqrtf(sqLength);
                return {{ value.v[0] * invLength, value.v[1] * invLength, value.v[2] * invLength, value.v[3] * invLength }};
            }
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::NormalizeSafeEstimate(FloatArgType value, float tolerance)
        {
            return NormalizeSafe(value, tolerance);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::QuaternionMultiply(FloatArgType arg1, FloatArgType arg2)
        {
            return {{ (arg1.v[1] * arg2.v[2]) - (arg1.v[2] * arg2.v[1]) + (arg1.v[3] * arg2.v[0]) + (arg1.v[0] * arg2.v[3])
                    , (arg1.v[2] * arg2.v[0]) - (arg1.v[0] * arg2.v[2]) + (arg1.v[3] * arg2.v[1]) + (arg1.v[1] * arg2.v[3])
                    , (arg1.v[0] * arg2.v[1]) - (arg1.v[1] * arg2.v[0]) + (arg1.v[3] * arg2.v[2]) + (arg1.v[2] * arg2.v[3])
                    , (arg1.v[3] * arg2.v[3]) - (arg1.v[0] * arg2.v[0]) - (arg1.v[1] * arg2.v[1]) - (arg1.v[2] * arg2.v[2]) }};
        }

        AZ_MATH_INLINE Vec3::FloatType Vec4::QuaternionTransform(FloatArgType quat, Vec3::FloatArgType vec3)
        {
            const float scalarTwo = quat.v[3] * 2.0f;
            const float scalarSquare = quat.v[3] * quat.v[3];
            const Vec3::FloatType vecQuat = ToVec3(quat);

            const Vec3::FloatType partial1 = Vec3::FromVec1(Vec3::Dot(vecQuat, vec3));
            const Vec3::FloatType partial2 = Vec3::Mul(vecQuat, partial1);
            const Vec3::FloatType sum1 = Vec3::Mul(partial2, Vec3::Splat(2.0f)); // quat.Dot(vec3) * vec3 * 2.0f

            const Vec3::FloatType partial3 = Vec3::FromVec1(Vec3::Dot(vecQuat, vecQuat));
            const Vec3::FloatType partial4 = Vec3::FromVec1(scalarSquare);
            const Vec3::FloatType partial5 = Vec3::Sub(partial4, partial3);
            const Vec3::FloatType sum2 = Vec3::Mul(partial5, vec3); // vec3 * (scalar * scalar - quat.Dot(quat))

            const Vec3::FloatType partial6 = Vec3::FromVec1(scalarTwo);
            const Vec3::FloatType partial7 = Vec3::Cross(vecQuat, vec3);
            const Vec3::FloatType sum3 = Vec3::Mul(partial6, partial7); // scalar * 2.0f * quat.Cross(vec3)

            return Vec3::Add(Vec3::Add(sum1, sum2), sum3);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::ConstructPlane(Vec3::FloatArgType normal, Vec3::FloatArgType point)
        {
            const float distance = normal.v[0] * point.v[0] + normal.v[1] * point.v[1] + normal.v[2] * point.v[2];
            return {{ normal.v[0], normal.v[1], normal.v[2], -distance }};
        }

        AZ_MATH_INLINE Vec1::FloatType Vec4::PlaneDistance(FloatArgType plane, Vec3::FloatArgType point)
        {
            const FloatType referencePoint = {{ point.v[0], point.v[1], point.v[2], 1.0f }};
            return Dot(referencePoint, plane);
        }

        AZ_MATH_INLINE void Vec4::Mat3x4InverseFast(const FloatType* __restrict rows, FloatType* __restrict out)
        {
            Mat3x4Transpose(rows, out);
            const Vec3::FloatType translation = {{ rows[0].v[3], rows[1].v[3], rows[2].v[3] }};
            out[0].v[3] = -Vec3::Dot(ToVec3(out[0]), translation);
            out[1].v[3] = -Vec3::Dot(ToVec3(out[1]), translation);
            out[2].v[3] = -Vec3::Dot(ToVec3(out[2]), translation);
        }

        AZ_MATH_INLINE void Vec4::Mat3x4Transpose(const FloatType* __restrict rows, FloatType* __restrict out)
        {
            // 2-phase, in case rows and out are aliased
            const FloatType cols0 = {{ rows[0].v[0], rows[1].v[0], rows[2].v[0], 0.0f }};
            const FloatType cols1 = {{ rows[0].v[1], rows[1].v[1], rows[2].v[1], 0.0f }};
            const FloatType cols2 = {{ rows[0].v[2], rows[1].v[2], rows[2].v[2], 0.0f }};
            out[0] = cols0;
            out[1] = cols1;
            out[2] = cols2;
        }

        namespace Internal
        {
            AZ_MATH_INLINE Vec3::FloatType GetColumn3x4(int32_t col, const Vec4::FloatType* __restrict rows)
            {
                return {{ rows[0].v[col], rows[1].v[col], rows[2].v[col] }};
            }
        }

        AZ_MATH_INLINE void Vec4::Mat3x4Multiply(const FloatType* __restrict rowsA, const FloatType* __restrict rowsB, FloatType* __restrict out)
        {
            for (int32_t row = 0; row < 3; ++row)
            {
                out[row] = {{ Vec3::Dot(ToVec3(rowsA[row]), Internal::GetColumn3x4(0, rowsB))
                            , Vec3::Dot(ToVec3(rowsA[row]), Internal::GetColumn3x4(1, rowsB))
                            , Vec3::Dot(ToVec3(rowsA[row]), Internal::GetColumn3x4(2, rowsB))
                            , Vec3::Dot(ToVec3(rowsA[row]), Internal::GetColumn3x4(3, rowsB)) + rowsA[row].v[3] }};
            }
        }

        AZ_MATH_INLINE void Vec4::Mat4x4InverseFast(const FloatType* __restrict rows, FloatType* __restrict out)
        {
            for (int32_t i = 0; i < 3; ++i)
            {
                for (int32_t j = 0; j < 3; j++)
                {
                    out[i].v[j] = rows[j].v[i];
                }
            }

            const Vec3::FloatType position = {{ rows[0].v[3], rows[1].v[3], rows[2].v[3] }};
            out[0].v[3] = -(Simd::Vec3::Dot(position, {{ rows[0].v[0], rows[1].v[0], rows[2].v[0] }}));
            out[1].v[3] = -(Simd::Vec3::Dot(position, {{ rows[0].v[1], rows[1].v[1], rows[2].v[1] }}));
            out[2].v[3] = -(Simd::Vec3::Dot(position, {{ rows[0].v[2], rows[1].v[2], rows[2].v[2] }}));
            out[3].v[0] = 0.0f;
            out[3].v[1] = 0.0f;
            out[3].v[2] = 0.0f;
            out[3].v[3] = 1.0f;
        }

        AZ_MATH_INLINE void Vec4::Mat4x4Transpose(const FloatType* rows, FloatType* out)
        {
            // 2-phase, in case rows and out are aliased
            const FloatType cols0 = {{ rows[0].v[0], rows[1].v[0], rows[2].v[0], rows[3].v[0] }};
            const FloatType cols1 = {{ rows[0].v[1], rows[1].v[1], rows[2].v[1], rows[3].v[1] }};
            const FloatType cols2 = {{ rows[0].v[2], rows[1].v[2], rows[2].v[2], rows[3].v[2] }};
            const FloatType cols3 = {{ rows[0].v[3], rows[1].v[3], rows[2].v[3], rows[3].v[3] }};
            out[0] = cols0;
            out[1] = cols1;
            out[2] = cols2;
            out[3] = cols3;
        }

        AZ_MATH_INLINE void Vec4::Mat4x4Multiply(const FloatType* rowsA, const FloatType* rowsB, FloatType* out)
        {
            for (int32_t row = 0; row < 4; ++row)
            {
                for (int32_t col = 0; col < 4; col++)
                {
                    out[row].v[col] = 0.0f;
                    for (int32_t k = 0; k < 4; ++k)
                    {
                        out[row].v[col] += rowsA[row].v[k] * rowsB[k].v[col];
                    }
                }
            }
        }

        AZ_MATH_INLINE void Vec4::Mat4x4MultiplyAdd(const FloatType* rowsA, const FloatType* rowsB, const FloatType* add, FloatType* out)
        {
            for (int32_t row = 0; row < 4; ++row)
            {
                for (int32_t col = 0; col < 4; col++)
                {
                    out[row].v[col] = add[row].v[col];
                    for (int32_t k = 0; k < 4; ++k)
                    {
                        out[row].v[col] += rowsA[row].v[k] * rowsB[k].v[col];
                    }
                }
            }
        }

        AZ_MATH_INLINE void Vec4::Mat4x4TransposeMultiply(const FloatType* rowsA, const FloatType* rowsB, FloatType* out)
        {
            const FloatType bc0 = {{ rowsB[0].v[0], rowsB[1].v[0], rowsB[2].v[0], rowsB[3].v[0] }};
            const FloatType bc1 = {{ rowsB[0].v[1], rowsB[1].v[1], rowsB[2].v[1], rowsB[3].v[1] }};
            const FloatType bc2 = {{ rowsB[0].v[2], rowsB[1].v[2], rowsB[2].v[2], rowsB[3].v[2] }};
            const FloatType bc3 = {{ rowsB[0].v[2], rowsB[1].v[2], rowsB[2].v[2], rowsB[3].v[2] }};

            const FloatType ac0 = {{ rowsA[0].v[0], rowsA[1].v[0], rowsA[2].v[0], rowsA[3].v[0] }};
            const FloatType ac1 = {{ rowsA[0].v[1], rowsA[1].v[1], rowsA[2].v[1], rowsA[3].v[1] }};
            const FloatType ac2 = {{ rowsA[0].v[2], rowsA[1].v[2], rowsA[2].v[2], rowsA[3].v[2] }};
            const FloatType ac3 = {{ rowsA[0].v[2], rowsA[1].v[2], rowsA[2].v[2], rowsA[3].v[2] }};

            out[0] = {{ Dot(ac0, bc0), Dot(ac0, bc1), Dot(ac0, bc2), Dot(ac0, bc3) }};
            out[1] = {{ Dot(ac1, bc0), Dot(ac1, bc1), Dot(ac1, bc2), Dot(ac1, bc3) }};
            out[2] = {{ Dot(ac2, bc0), Dot(ac2, bc1), Dot(ac2, bc2), Dot(ac2, bc3) }};
            out[3] = {{ Dot(ac3, bc0), Dot(ac3, bc1), Dot(ac3, bc2), Dot(ac3, bc3) }};
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Mat4x4TransformVector(const FloatType* __restrict rows, FloatArgType vector)
        {
            return {{ rows[0].v[0] * vector.v[0] + rows[0].v[1] * vector.v[1] + rows[0].v[2] * vector.v[2] + rows[0].v[3] * vector.v[3]
                    , rows[1].v[0] * vector.v[0] + rows[1].v[1] * vector.v[1] + rows[1].v[2] * vector.v[2] + rows[1].v[3] * vector.v[3]
                    , rows[2].v[0] * vector.v[0] + rows[2].v[1] * vector.v[1] + rows[2].v[2] * vector.v[2] + rows[2].v[3] * vector.v[3]
                    , rows[3].v[0] * vector.v[0] + rows[3].v[1] * vector.v[1] + rows[3].v[2] * vector.v[2] + rows[3].v[3] * vector.v[3] }};
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Mat4x4TransposeTransformVector(const FloatType* __restrict rows, FloatArgType vector)
        {
            return {{ rows[0].v[0] * vector.v[0] + rows[1].v[0] * vector.v[1] + rows[2].v[0] * vector.v[2] + rows[3].v[0] * vector.v[3]
                    , rows[0].v[1] * vector.v[0] + rows[1].v[1] * vector.v[1] + rows[2].v[1] * vector.v[2] + rows[3].v[1] * vector.v[3]
                    , rows[0].v[2] * vector.v[0] + rows[1].v[2] * vector.v[1] + rows[2].v[2] * vector.v[2] + rows[3].v[2] * vector.v[3]
                    , rows[0].v[3] * vector.v[0] + rows[1].v[3] * vector.v[1] + rows[2].v[3] * vector.v[2] + rows[3].v[3] * vector.v[3] }};
        }

        AZ_MATH_INLINE Vec3::FloatType Vec4::Mat4x4TransformPoint3(const FloatType* __restrict rows, Vec3::FloatArgType vector)
        {
            return {{ rows[0].v[0] * vector.v[0] + rows[0].v[1] * vector.v[1] + rows[0].v[2] * vector.v[2] + rows[0].v[3]
                    , rows[1].v[0] * vector.v[0] + rows[1].v[1] * vector.v[1] + rows[1].v[2] * vector.v[2] + rows[1].v[3]
                    , rows[2].v[0] * vector.v[0] + rows[2].v[1] * vector.v[1] + rows[2].v[2] * vector.v[2] + rows[2].v[3] }};
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::ConvertToFloat(Int32ArgType value)
        {
            return {{ static_cast<float>(value.v[0])
                    , static_cast<float>(value.v[1])
                    , static_cast<float>(value.v[2])
                    , static_cast<float>(value.v[3]) }};
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::ConvertToInt(FloatArgType value)
        {
            return {{ static_cast<int32_t>(value.v[0])
                    , static_cast<int32_t>(value.v[1])
                    , static_cast<int32_t>(value.v[2])
                    , static_cast<int32_t>(value.v[3]) }};
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::ConvertToIntNearest(FloatArgType value)
        {
            return ConvertToInt(Round(value));
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::CastToFloat(Int32ArgType value)
        {
            return {{ reinterpret_cast<const float&>(value.v[0])
                    , reinterpret_cast<const float&>(value.v[1])
                    , reinterpret_cast<const float&>(value.v[2])
                    , reinterpret_cast<const float&>(value.v[3]) }};
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::CastToInt(FloatArgType value)
        {
            return {{ reinterpret_cast<const int32_t&>(value.v[0])
                    , reinterpret_cast<const int32_t&>(value.v[1])
                    , reinterpret_cast<const int32_t&>(value.v[2])
                    , reinterpret_cast<const int32_t&>(value.v[3]) }};
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::ZeroFloat()
        {
            return {{ 0.0f, 0.0f, 0.0f, 0.0f }};
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::ZeroInt()
        {
            return {{ 0, 0, 0, 0 }};
        }
    }
}

AZ_POP_DISABLE_WARNING
