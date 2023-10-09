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
        AZ_MATH_INLINE Vec1::FloatType Vec4::ToVec1(FloatArgType value)
        {
            return value;
        }

        AZ_MATH_INLINE Vec2::FloatType Vec4::ToVec2(FloatArgType value)
        {
            return value;
        }

        AZ_MATH_INLINE Vec3::FloatType Vec4::ToVec3(FloatArgType value)
        {
            return value;
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::FromVec1(Vec1::FloatArgType value)
        {
            // Coming from a Vec1 the last 3 elements could be garbage.
            return Sse::SplatIndex0(value); // {value.x, value.x, value.x, value.x}
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::FromVec2(Vec2::FloatArgType value)
        {
            // Coming from a Vec2 the last 2 elements could be garbage.
            return Sse::ReplaceIndex3(Sse::ReplaceIndex2(value, 0.0f), 0.0f); // {value.x, value.x, 0.0f, 0.0f}
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::FromVec3(Vec3::FloatArgType value)
        {
            // Coming from a Vec3 the last element could be garbage.
            return Sse::ReplaceIndex3(value, 0.0f); // {value.x, value.y, value.z, 0.0f}
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::LoadAligned(const float* __restrict addr)
        {
            return Sse::LoadAligned(addr);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::LoadAligned(const int32_t* __restrict addr)
        {
            return Sse::LoadAligned(addr);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::LoadUnaligned(const float* __restrict addr)
        {
            return Sse::LoadUnaligned(addr);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::LoadUnaligned(const int32_t* __restrict addr)
        {
            return Sse::LoadUnaligned(addr);
        }

        AZ_MATH_INLINE void Vec4::StoreAligned(float* __restrict addr, FloatArgType value)
        {
            return Sse::StoreAligned(addr, value);
        }

        AZ_MATH_INLINE void Vec4::StoreAligned(int32_t* __restrict addr, Int32ArgType value)
        {
            return Sse::StoreAligned(addr, value);
        }

        AZ_MATH_INLINE void Vec4::StoreUnaligned(float* __restrict addr, FloatArgType value)
        {
            return Sse::StoreUnaligned(addr, value);
        }

        AZ_MATH_INLINE void Vec4::StoreUnaligned(int32_t* __restrict addr, Int32ArgType value)
        {
            return Sse::StoreUnaligned(addr, value);
        }

        AZ_MATH_INLINE void Vec4::StreamAligned(float* __restrict addr, FloatArgType value)
        {
            return Sse::StreamAligned(addr, value);
        }

        AZ_MATH_INLINE void Vec4::StreamAligned(int32_t* __restrict addr, Int32ArgType value)
        {
            return Sse::StreamAligned(addr, value);
        }

        AZ_MATH_INLINE float Vec4::SelectIndex0(FloatArgType value)
        {
            return Sse::SelectIndex0(value);
        }

        AZ_MATH_INLINE float Vec4::SelectIndex1(FloatArgType value)
        {
            return Sse::SelectIndex0(Sse::SplatIndex1(value));
        }

        AZ_MATH_INLINE float Vec4::SelectIndex2(FloatArgType value)
        {
            return Sse::SelectIndex0(Sse::SplatIndex2(value));
        }

        AZ_MATH_INLINE float Vec4::SelectIndex3(FloatArgType value)
        {
            return Sse::SelectIndex0(Sse::SplatIndex3(value));
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Splat(float value)
        {
            return Sse::Splat(value);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::Splat(int32_t value)
        {
            return Sse::Splat(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::SplatIndex0(FloatArgType value)
        {
            return Sse::SplatIndex0(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::SplatIndex1(FloatArgType value)
        {
            return Sse::SplatIndex1(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::SplatIndex2(FloatArgType value)
        {
            return Sse::SplatIndex2(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::SplatIndex3(FloatArgType value)
        {
            return Sse::SplatIndex3(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::ReplaceIndex0(FloatArgType a, float b)
        {
            return Sse::ReplaceIndex0(a, b);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::ReplaceIndex0(FloatArgType a, FloatArgType b)
        {
            return Sse::ReplaceIndex0(a, b);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::ReplaceIndex1(FloatArgType a, float b)
        {
            return Sse::ReplaceIndex1(a, b);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::ReplaceIndex1(FloatArgType a, FloatArgType b)
        {
            return Sse::ReplaceIndex1(a, b);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::ReplaceIndex2(FloatArgType a, float b)
        {
            return Sse::ReplaceIndex2(a, b);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::ReplaceIndex2(FloatArgType a, FloatArgType b)
        {
            return Sse::ReplaceIndex2(a, b);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::ReplaceIndex3(FloatArgType a, float b)
        {
            return Sse::ReplaceIndex3(a, b);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::ReplaceIndex3(FloatArgType a, FloatArgType b)
        {
            return Sse::ReplaceIndex3(a, b);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::LoadImmediate(float x, float y, float z, float w)
        {
            return Sse::LoadImmediate(x, y, z, w);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::LoadImmediate(int32_t x, int32_t y, int32_t z, int32_t w)
        {
            return Sse::LoadImmediate(x, y, z, w);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Add(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::Add(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Sub(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::Sub(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Mul(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::Mul(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Madd(FloatArgType mul1, FloatArgType mul2, FloatArgType add)
        {
            return Sse::Madd(mul1, mul2, add);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Div(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::Div(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Abs(FloatArgType value)
        {
            return Sse::Abs(value);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::Add(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::Add(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::Sub(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::Sub(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::Mul(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::Mul(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::Madd(Int32ArgType mul1, Int32ArgType mul2, Int32ArgType add)
        {
            return Sse::Madd(mul1, mul2, add);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::Abs(Int32ArgType value)
        {
            return Sse::Abs(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Not(FloatArgType value)
        {
            return Sse::Not(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::And(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::And(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::AndNot(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::AndNot(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Or(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::Or(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Xor(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::Xor(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::Not(Int32ArgType value)
        {
            return Sse::Not(value);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::And(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::And(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::AndNot(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::AndNot(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::Or(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::Or(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::Xor(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::Xor(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Floor(FloatArgType value)
        {
            return Sse::Floor(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Ceil(FloatArgType value)
        {
            return Sse::Ceil(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Round(FloatArgType value)
        {
            return Sse::Round(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Truncate(FloatArgType value)
        {
            return Sse::Truncate(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Min(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::Min(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Max(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::Max(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Clamp(FloatArgType value, FloatArgType min, FloatArgType max)
        {
            return Sse::Clamp(value, min, max);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::Min(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::Min(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::Max(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::Max(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::Clamp(Int32ArgType value, Int32ArgType min, Int32ArgType max)
        {
            return Sse::Clamp(value, min, max);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::CmpEq(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::CmpEq(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::CmpNeq(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::CmpNeq(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::CmpGt(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::CmpGt(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::CmpGtEq(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::CmpGtEq(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::CmpLt(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::CmpLt(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::CmpLtEq(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::CmpLtEq(arg1, arg2);
        }

        AZ_MATH_INLINE bool Vec4::CmpAllEq(FloatArgType arg1, FloatArgType arg2)
        {
            // Check the first four bits for Vector4
            return Sse::CmpAllEq(arg1, arg2, 0b1111);
        }

        AZ_MATH_INLINE bool Vec4::CmpAllLt(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::CmpAllLt(arg1, arg2, 0b1111);
        }

        AZ_MATH_INLINE bool Vec4::CmpAllLtEq(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::CmpAllLtEq(arg1, arg2, 0b1111);
        }

        AZ_MATH_INLINE bool Vec4::CmpAllGt(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::CmpAllGt(arg1, arg2, 0b1111);
        }

        AZ_MATH_INLINE bool Vec4::CmpAllGtEq(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::CmpAllGtEq(arg1, arg2, 0b1111);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::CmpEq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::CmpEq(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::CmpNeq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::CmpNeq(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::CmpGt(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::CmpGt(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::CmpGtEq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::CmpGtEq(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::CmpLt(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::CmpLt(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::CmpLtEq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::CmpLtEq(arg1, arg2);
        }

        AZ_MATH_INLINE bool Vec4::CmpAllEq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::CmpAllEq(arg1, arg2, 0b1111);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Select(FloatArgType arg1, FloatArgType arg2, FloatArgType mask)
        {
            return Sse::Select(arg1, arg2, mask);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::Select(Int32ArgType arg1, Int32ArgType arg2, Int32ArgType mask)
        {
            return Sse::Select(arg1, arg2, mask);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Reciprocal(FloatArgType value)
        {
            return Sse::Reciprocal(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::ReciprocalEstimate(FloatArgType value)
        {
            return Sse::ReciprocalEstimate(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Mod(FloatArgType value, FloatArgType divisor)
        {
            return Sse::Mod(value, divisor);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Wrap(FloatArgType value, FloatArgType minValue, FloatArgType maxValue)
        {
            return Common::Wrap<Vec4>(value, minValue, maxValue);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::AngleMod(FloatArgType value)
        {
            return Common::AngleMod<Vec4>(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Sqrt(FloatArgType value)
        {
            return Sse::Sqrt(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::SqrtEstimate(FloatArgType value)
        {
            return Sse::SqrtEstimate(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::SqrtInv(FloatArgType value)
        {
            return Sse::SqrtInv(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::SqrtInvEstimate(FloatArgType value)
        {
            return Sse::SqrtInvEstimate(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Sin(FloatArgType value)
        {
            return Common::Sin<Vec4>(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Cos(FloatArgType value)
        {
            return Common::Cos<Vec4>(value);
        }

        AZ_MATH_INLINE void Vec4::SinCos(FloatArgType value, FloatType& sin, FloatType& cos)
        {
            Common::SinCos<Vec4>(value, sin, cos);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::SinCos(FloatArgType angles)
        {
            const FloatType angleOffset = LoadImmediate(0.0f, Constants::HalfPi, 0.0f, Constants::HalfPi);
            const FloatType sinAngles = Add(angles, angleOffset);
            return Sin(sinAngles);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Acos(FloatArgType value)
        {
            return Common::Acos<Vec4>(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Atan(FloatArgType value)
        {
            return Common::Atan<Vec4>(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Atan2(FloatArgType y, FloatArgType x)
        {
            return Common::Atan2<Vec4>(y, x);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::ExpEstimate(FloatArgType value)
        {
            return Common::ExpEstimate<Vec4>(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Dot(FloatArgType arg1, FloatArgType arg2)
        {
#if 0
            return _mm_dp_ps(arg1, arg2, 0xff);
#else
            const FloatType x2  = Mul(arg1, arg2);
            const FloatType tmp = Add(x2, _mm_shuffle_ps(x2, x2, _MM_SHUFFLE(2, 3, 0, 1)));
            return Add(tmp, _mm_shuffle_ps(tmp, tmp, _MM_SHUFFLE(1, 0, 2, 3)));
#endif
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Normalize(FloatArgType value)
        {
            return Common::Normalize<Vec4>(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::NormalizeEstimate(FloatArgType value)
        {
            return Common::NormalizeEstimate<Vec4>(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::NormalizeSafe(FloatArgType value, float tolerance)
        {
            return Common::NormalizeSafe<Vec4>(value, tolerance);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::NormalizeSafeEstimate(FloatArgType value, float tolerance)
        {
            return Common::NormalizeSafeEstimate<Vec4>(value, tolerance);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::QuaternionMultiply(FloatArgType arg1, FloatArgType arg2)
        {
            // _x = (arg1.y * arg2.z) - (arg1.z * arg2.y) + (arg1.w * arg2.x) + (arg1.x * arg2.w);
            // _y = (arg1.z * arg2.x) - (arg1.x * arg2.z) + (arg1.w * arg2.y) + (arg1.y * arg2.w);
            // _z = (arg1.x * arg2.y) - (arg1.y * arg2.x) + (arg1.w * arg2.z) + (arg1.z * arg2.w);
            // _w = (arg1.w * arg2.w) - (arg1.x * arg2.x) - (arg1.y * arg2.y) - (arg1.z * arg2.z);
            const FloatType flipWSign = Common::FastLoadConstant<Vec4>((const float*)g_negateWMask);
            const FloatType val1 = _mm_shuffle_ps(arg1, arg1, _MM_SHUFFLE(3, 0, 2, 1)); // arg1 y z x w
            const FloatType val2 = _mm_shuffle_ps(arg2, arg2, _MM_SHUFFLE(3, 1, 0, 2)); // arg2 z x y w
            const FloatType val3 = _mm_shuffle_ps(arg1, arg1, _MM_SHUFFLE(0, 1, 0, 2)); // arg1 z x y x
            const FloatType val4 = _mm_shuffle_ps(arg2, arg2, _MM_SHUFFLE(0, 0, 2, 1)); // arg2 y z x x
            const FloatType val5 = _mm_shuffle_ps(arg1, arg1, _MM_SHUFFLE(1, 3, 3, 3)); // arg1 w w w y
            const FloatType val6 = _mm_shuffle_ps(arg2, arg2, _MM_SHUFFLE(1, 2, 1, 0)); // arg2 x y z y
            const FloatType val7 = _mm_shuffle_ps(arg1, arg1, _MM_SHUFFLE(2, 2, 1, 0)); // arg1 x y z z
            const FloatType val8 = _mm_shuffle_ps(arg2, arg2, _MM_SHUFFLE(2, 3, 3, 3)); // arg2 w w w z
            const FloatType firstTerm  = Mul(val1, val2); // (arg1.y * arg2.z), (arg1.z * arg2.x), (arg1.x * arg2.y), (arg1.w * arg2.w)
            const FloatType secondTerm = Mul(val3, val4); // (arg1.z * arg2.y), (arg1.x * arg2.z), (arg1.y * arg2.x), (arg1.x * arg2.x)
            const FloatType thirdTerm  = Mul(val5, val6); // (arg1.w * arg2.x), (arg1.w * arg2.y), (arg1.w * arg2.z), (arg1.y * arg2.y)
            const FloatType fourthTerm = Mul(val7, val8); // (arg1.x * arg2.w), (arg1.y * arg2.w), (arg1.z * arg2.w), (arg1.z * arg2.z)
            const FloatType partialOne = Sub(firstTerm, secondTerm);
            const FloatType partialTwo = Xor(Add(thirdTerm, fourthTerm), flipWSign);
            return Add(partialOne, partialTwo);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec4::QuaternionTransform(FloatArgType quat, Vec3::FloatArgType vec3)
        {
            return Common::QuaternionTransform<Vec4, Vec3>(quat, vec3);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::ConstructPlane(Vec3::FloatArgType normal, Vec3::FloatArgType point)
        {
            return Common::ConstructPlane<Vec4, Vec3>(normal, point);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec4::PlaneDistance(FloatArgType plane, Vec3::FloatArgType point)
        {
            return Common::PlaneDistance<Vec4, Vec3>(plane, point);
        }

        AZ_MATH_INLINE void Vec4::Mat3x4InverseFast(const FloatType* __restrict rows, FloatType* __restrict out)
        {
            const FloatType pos = Sub(ZeroFloat(), Madd(rows[0], SplatIndex3(rows[0]), Madd(rows[1], SplatIndex3(rows[1]), Mul(rows[2], SplatIndex3(rows[2])))));
            const FloatType tmp0 = _mm_shuffle_ps(rows[0], rows[1], 0x44);
            const FloatType tmp2 = _mm_shuffle_ps(rows[0], rows[1], 0xEE);
            const FloatType tmp1 = _mm_shuffle_ps(rows[2], pos, 0x44);
            const FloatType tmp3 = _mm_shuffle_ps(rows[2], pos, 0xEE);
            out[0] = _mm_shuffle_ps(tmp0, tmp1, 0x88);
            out[1] = _mm_shuffle_ps(tmp0, tmp1, 0xDD);
            out[2] = _mm_shuffle_ps(tmp2, tmp3, 0x88);
        }

        AZ_MATH_INLINE void Vec4::Mat3x4Transpose(const FloatType* __restrict rows, FloatType* __restrict out)
        {
            const FloatType fourth = Common::FastLoadConstant<Vec4>(g_vec0001);
            const FloatType tmp0 = _mm_shuffle_ps(rows[0], rows[1], 0x44);
            const FloatType tmp2 = _mm_shuffle_ps(rows[0], rows[1], 0xEE);
            const FloatType tmp1 = _mm_shuffle_ps(rows[2], fourth, 0x44);
            const FloatType tmp3 = _mm_shuffle_ps(rows[2], fourth, 0xEE);
            out[0] = _mm_shuffle_ps(tmp0, tmp1, 0x88);
            out[1] = _mm_shuffle_ps(tmp0, tmp1, 0xDD);
            out[2] = _mm_shuffle_ps(tmp2, tmp3, 0x88);
        }

        AZ_MATH_INLINE void Vec4::Mat3x4Multiply(const FloatType* __restrict rowsA, const FloatType* __restrict rowsB, FloatType* __restrict out)
        {
            Common::Mat3x4Multiply<Vec4>(rowsA, rowsB, out);
        }

        AZ_MATH_INLINE void Vec4::Mat4x4InverseFast(const FloatType* __restrict rows, FloatType* __restrict out)
        {
            Common::Mat4x4InverseFast<Vec4>(rows, out);
        }

        AZ_MATH_INLINE void Vec4::Mat4x4Transpose(const FloatType* __restrict rows, FloatType* __restrict out)
        {
            const FloatType tmp0 = _mm_shuffle_ps(rows[0], rows[1], 0x44);
            const FloatType tmp2 = _mm_shuffle_ps(rows[0], rows[1], 0xEE);
            const FloatType tmp1 = _mm_shuffle_ps(rows[2], rows[3], 0x44);
            const FloatType tmp3 = _mm_shuffle_ps(rows[2], rows[3], 0xEE);
            out[0] = _mm_shuffle_ps(tmp0, tmp1, 0x88);
            out[1] = _mm_shuffle_ps(tmp0, tmp1, 0xDD);
            out[2] = _mm_shuffle_ps(tmp2, tmp3, 0x88);
            out[3] = _mm_shuffle_ps(tmp2, tmp3, 0xDD);
        }

        AZ_MATH_INLINE void Vec4::Mat4x4Multiply(const FloatType* __restrict rowsA, const FloatType* __restrict rowsB, FloatType* __restrict out)
        {
            Common::Mat4x4Multiply<Vec4>(rowsA, rowsB, out);
        }

        AZ_MATH_INLINE void Vec4::Mat4x4MultiplyAdd(const FloatType* __restrict rowsA, const FloatType* __restrict rowsB, const FloatType* __restrict add, FloatType* __restrict out)
        {
            Common::Mat4x4MultiplyAdd<Vec4>(rowsA, rowsB, add, out);
        }

        AZ_MATH_INLINE void Vec4::Mat4x4TransposeMultiply(const FloatType* __restrict rowsA, const FloatType* __restrict rowsB, FloatType* __restrict out)
        {
            Common::Mat4x4TransposeMultiply<Vec4>(rowsA, rowsB, out);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Mat4x4TransformVector(const FloatType* __restrict rows, FloatArgType vector)
        {
            const FloatType prod1 = Mul(rows[0], vector);
            const FloatType prod2 = Mul(rows[1], vector);
            const FloatType prod3 = Mul(rows[2], vector);
            const FloatType prod4 = Mul(rows[3], vector);
            return _mm_hadd_ps(_mm_hadd_ps(prod1, prod2), _mm_hadd_ps(prod3, prod4));
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Mat4x4TransposeTransformVector(const FloatType* __restrict rows, FloatArgType vector)
        {
            return Common::Mat4x4TransposeTransformVector<Vec4>(rows, vector);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec4::Mat4x4TransformPoint3(const FloatType* __restrict rows, Vec3::FloatArgType vector)
        {
            // The hadd solution is profiling slightly faster than transpose + transposetransform
            const FloatType vecXYZ = ReplaceIndex3(vector, 1.0f); // Explicitly set the w cord to 1 to include translation
            const FloatType prod1 = Mul(rows[0], vecXYZ);
            const FloatType prod2 = Mul(rows[1], vecXYZ);
            const FloatType prod3 = Mul(rows[2], vecXYZ);
            return _mm_hadd_ps(_mm_hadd_ps(prod1, prod2), _mm_hadd_ps(prod3, ZeroFloat()));
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::ConvertToFloat(Int32ArgType value)
        {
            return Sse::ConvertToFloat(value);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::ConvertToInt(FloatArgType value)
        {
            return Sse::ConvertToInt(value);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::ConvertToIntNearest(FloatArgType value)
        {
            return Sse::ConvertToIntNearest(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::CastToFloat(Int32ArgType value)
        {
            return Sse::CastToFloat(value);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::CastToInt(FloatArgType value)
        {
            return Sse::CastToInt(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::ZeroFloat()
        {
            return Sse::ZeroFloat();
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::ZeroInt()
        {
            return Sse::ZeroInt();
        }
    }
}
