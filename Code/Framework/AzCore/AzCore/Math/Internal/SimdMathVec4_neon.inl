/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Internal/SimdMathCommon_neon.inl>
#include <AzCore/Math/Internal/SimdMathCommon_neonQuad.inl>
#include <AzCore/Math/Internal/SimdMathCommon_simd.inl>

namespace AZ
{
    namespace Simd
    {
        AZ_MATH_INLINE Vec1::FloatType Vec4::ToVec1(FloatArgType value)
        {
            return NeonQuad::ToVec1(value);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec4::ToVec2(FloatArgType value)
        {
            return NeonQuad::ToVec2(value);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec4::ToVec3(FloatArgType value)
        {
            return value;
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::FromVec1(Vec1::FloatArgType value)
        {
            return NeonQuad::FromVec1(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::FromVec2(Vec2::FloatArgType value)
        {
            return NeonQuad::FromVec2(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::FromVec3(Vec3::FloatArgType value)
        {
            // Coming from a Vec3 the last element could be garbage.
            return NeonQuad::ReplaceIndex3(value, 0.0f); // {value.x, value.y, value.z, 0.0f}
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::LoadAligned(const float* __restrict addr)
        {
            return NeonQuad::LoadAligned(addr);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::LoadAligned(const int32_t* __restrict addr)
        {
            return NeonQuad::LoadAligned(addr);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::LoadUnaligned(const float* __restrict addr)
        {
            return NeonQuad::LoadUnaligned(addr);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::LoadUnaligned(const int32_t* __restrict addr)
        {
            return NeonQuad::LoadUnaligned(addr);
        }

        AZ_MATH_INLINE void Vec4::StoreAligned(float* __restrict addr, FloatArgType value)
        {
            NeonQuad::StoreAligned(addr, value);
        }

        AZ_MATH_INLINE void Vec4::StoreAligned(int32_t* __restrict addr, Int32ArgType value)
        {
            NeonQuad::StoreAligned(addr, value);
        }

        AZ_MATH_INLINE void Vec4::StoreUnaligned(float* __restrict addr, FloatArgType value)
        {
            NeonQuad::StoreUnaligned(addr, value);
        }

        AZ_MATH_INLINE void Vec4::StoreUnaligned(int32_t* __restrict addr, Int32ArgType value)
        {
            NeonQuad::StoreUnaligned(addr, value);
        }

        AZ_MATH_INLINE void Vec4::StreamAligned(float* __restrict addr, FloatArgType value)
        {
            NeonQuad::StreamAligned(addr, value);
        }

        AZ_MATH_INLINE void Vec4::StreamAligned(int32_t* __restrict addr, Int32ArgType value)
        {
            NeonQuad::StreamAligned(addr, value);
        }

        AZ_MATH_INLINE float Vec4::SelectIndex0(FloatArgType value)
        {
            return NeonQuad::SelectIndex0(value);
        }

        AZ_MATH_INLINE float Vec4::SelectIndex1(FloatArgType value)
        {
            return NeonQuad::SelectIndex1(value);
        }

        AZ_MATH_INLINE float Vec4::SelectIndex2(FloatArgType value)
        {
            return NeonQuad::SelectIndex2(value);
        }

        AZ_MATH_INLINE float Vec4::SelectIndex3(FloatArgType value)
        {
            return NeonQuad::SelectIndex3(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Splat(float value)
        {
            return NeonQuad::Splat(value);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::Splat(int32_t value)
        {
            return NeonQuad::Splat(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::SplatIndex0(FloatArgType value)
        {
            return NeonQuad::SplatIndex0(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::SplatIndex1(FloatArgType value)
        {
            return NeonQuad::SplatIndex1(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::SplatIndex2(FloatArgType value)
        {
            return NeonQuad::SplatIndex2(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::SplatIndex3(FloatArgType value)
        {
            return NeonQuad::SplatIndex3(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::ReplaceIndex0(FloatArgType a, float b)
        {
            return NeonQuad::ReplaceIndex0(a, b);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::ReplaceIndex0(FloatArgType a, FloatArgType b)
        {
            return NeonQuad::ReplaceIndex0(a, b);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::ReplaceIndex1(FloatArgType a, float b)
        {
            return NeonQuad::ReplaceIndex1(a, b);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::ReplaceIndex1(FloatArgType a, FloatArgType b)
        {
            return NeonQuad::ReplaceIndex1(a, b);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::ReplaceIndex2(FloatArgType a, float b)
        {
            return NeonQuad::ReplaceIndex2(a, b);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::ReplaceIndex2(FloatArgType a, FloatArgType b)
        {
            return NeonQuad::ReplaceIndex2(a, b);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::ReplaceIndex3(FloatArgType a, float b)
        {
            return NeonQuad::ReplaceIndex3(a, b);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::ReplaceIndex3(FloatArgType a, FloatArgType b)
        {
            return NeonQuad::ReplaceIndex3(a, b);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::LoadImmediate(float x, float y, float z, float w)
        {
            return NeonQuad::LoadImmediate(x, y, z, w);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::LoadImmediate(int32_t x, int32_t y, int32_t z, int32_t w)
        {
            return NeonQuad::LoadImmediate(x, y, z, w);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Add(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonQuad::Add(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Sub(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonQuad::Sub(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Mul(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonQuad::Mul(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Madd(FloatArgType mul1, FloatArgType mul2, FloatArgType add)
        {
            return NeonQuad::Madd(mul1, mul2, add);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Div(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonQuad::Div(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Abs(FloatArgType value)
        {
            return NeonQuad::Abs(value);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::Add(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonQuad::Add(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::Sub(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonQuad::Sub(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::Mul(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonQuad::Mul(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::Madd(Int32ArgType mul1, Int32ArgType mul2, Int32ArgType add)
        {
            return NeonQuad::Madd(mul1, mul2, add);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::Abs(Int32ArgType value)
        {
            return NeonQuad::Abs(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Not(FloatArgType value)
        {
            return NeonQuad::Not(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::And(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonQuad::And(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::AndNot(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonQuad::AndNot(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Or(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonQuad::Or(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Xor(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonQuad::Xor(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::Not(Int32ArgType value)
        {
            return NeonQuad::Not(value);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::And(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonQuad::And(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::AndNot(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonQuad::AndNot(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::Or(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonQuad::Or(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::Xor(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonQuad::Xor(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Floor(FloatArgType value)
        {
            return NeonQuad::Floor(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Ceil(FloatArgType value)
        {
            return NeonQuad::Ceil(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Round(FloatArgType value)
        {
            return NeonQuad::Round(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Truncate(FloatArgType value)
        {
            return NeonQuad::Truncate(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Min(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonQuad::Min(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Max(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonQuad::Max(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Clamp(FloatArgType value, FloatArgType min, FloatArgType max)
        {
            return NeonQuad::Clamp(value, min, max);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::Min(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonQuad::Min(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::Max(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonQuad::Max(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::Clamp(Int32ArgType value, Int32ArgType min, Int32ArgType max)
        {
            return NeonQuad::Clamp(value, min, max);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::CmpEq(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonQuad::CmpEq(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::CmpNeq(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonQuad::CmpNeq(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::CmpGt(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonQuad::CmpGt(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::CmpGtEq(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonQuad::CmpGtEq(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::CmpLt(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonQuad::CmpLt(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::CmpLtEq(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonQuad::CmpLtEq(arg1, arg2);
        }

        AZ_MATH_INLINE bool Vec4::CmpAllEq(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonQuad::CmpAllEq(arg1, arg2);
        }

        AZ_MATH_INLINE bool Vec4::CmpAllLt(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonQuad::CmpAllLt(arg1, arg2);
        }

        AZ_MATH_INLINE bool Vec4::CmpAllLtEq(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonQuad::CmpAllLtEq(arg1, arg2);
        }

        AZ_MATH_INLINE bool Vec4::CmpAllGt(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonQuad::CmpAllGt(arg1, arg2);
        }

        AZ_MATH_INLINE bool Vec4::CmpAllGtEq(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonQuad::CmpAllGtEq(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::CmpEq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonQuad::CmpEq(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::CmpNeq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonQuad::CmpNeq(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::CmpGt(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonQuad::CmpGt(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::CmpGtEq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonQuad::CmpGtEq(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::CmpLt(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonQuad::CmpLt(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::CmpLtEq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonQuad::CmpLtEq(arg1, arg2);
        }

        AZ_MATH_INLINE bool Vec4::CmpAllEq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonQuad::CmpAllEq(arg1, arg2);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Select(FloatArgType arg1, FloatArgType arg2, FloatArgType mask)
        {
            return NeonQuad::Select(arg1, arg2, mask);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::Select(Int32ArgType arg1, Int32ArgType arg2, Int32ArgType mask)
        {
            return NeonQuad::Select(arg1, arg2, mask);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Reciprocal(FloatArgType value)
        {
            return NeonQuad::Reciprocal(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::ReciprocalEstimate(FloatArgType value)
        {
            return NeonQuad::ReciprocalEstimate(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Mod(FloatArgType value, FloatArgType divisor)
        {
            return NeonQuad::Mod(value, divisor);
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
            return NeonQuad::Sqrt(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::SqrtEstimate(FloatArgType value)
        {
            return NeonQuad::SqrtEstimate(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::SqrtInv(FloatArgType value)
        {
            return NeonQuad::SqrtInv(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::SqrtInvEstimate(FloatArgType value)
        {
            return NeonQuad::SqrtInvEstimate(value);
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

        AZ_MATH_INLINE Vec4::FloatType Vec4::ExpEstimate(FloatArgType x)
        {
            return Common::ExpEstimate<Vec4>(x);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec4::Dot(FloatArgType arg1, FloatArgType arg2)
        {
            return Vec1::LoadImmediate(vaddvq_f32(vmulq_f32(arg1, arg2)));
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
            // We need to return:
            //
            // x = arg1.y * arg2.z - arg1.z * arg2.y + arg1.w * arg2.x + arg1.x * arg2.w;
            // y = arg1.z * arg2.x - arg1.x * arg2.z + arg1.w * arg2.y + arg1.y * arg2.w;
            // z = arg1.x * arg2.y - arg1.y * arg2.x + arg1.w * arg2.z + arg1.z * arg2.w;
            // y = arg1.w * arg2.w - arg1.x * arg2.x - arg1.y * arg2.y - arg1.z * arg2.z;
            //
            // but there are no Neon intrinsics to shuffle vector elements,
            // so we must use a combination of other intrinsic instructions
            // to manipulate each input argument vector in order to obtain:
            //
            // arg1 -> (y, z, x, w)
            // arg1 -> (z, x, y, x)
            // arg1 -> (w, w, w, y)
            // arg1 -> (x, y, z, z)
            // arg2 -> (z, x, y, w)
            // arg2 -> (y, z, x, x)
            // arg2 -> (x, y, z, y)
            // arg2 -> (w, w, w, z)
            
            // Use the extract vector intrinsic to get arg1 -> (x, y, z, z)
            const FloatType arg1_zwxy = vextq_f32(arg1, arg1, 2);
            const FloatType arg1_wxyz = vextq_f32(arg1, arg1, 3);
            const FloatType arg1_xyzz = vextq_f32(arg1_wxyz, arg1_zwxy, 1);

            // Use the transpose and extract vector intrinsics to get arg1 -> (w, w, w, y)
            const FloatType arg1_wwyy = vtrn1q_f32(arg1_wxyz, arg1_wxyz);
            const FloatType arg1_wwwy = vextq_f32(arg1, arg1_wwyy, 3);

            // Use the extract vector intrinsic to get arg1 -> (z, x, y, x)
            const FloatType arg1_xyxy = vextq_f32(arg1_zwxy, arg1, 2);
            const FloatType arg1_zxyx = vextq_f32(arg1_xyzz, arg1_xyxy, 3);

            // Use the transpose and extract vector intrinsics to get arg1 -> (y, z, x, w)
            const FloatType arg1_xwxy = vtrn2q_f32(arg1_zxyx, arg1_zwxy);
            const FloatType arg1_yzxw = vextq_f32(arg1_wxyz, arg1_xwxy, 2);

            // Use the extract and transpose vector intrinsics to get arg2 -> (y, z, x, x)
            const FloatType arg2_zwxy = vextq_f32(arg2, arg2, 2);
            const FloatType arg2_zxxz = vtrn1q_f32(arg2_zwxy, arg2);
            const FloatType arg2_yzxx = vextq_f32(arg2_zwxy, arg2_zxxz, 3);

            // Use the extract vector intrinsics to get arg2 -> (x, y, z, y)
            const FloatType arg2_wxyz = vextq_f32(arg2, arg2, 3);
            const FloatType arg2_xyzy = vextq_f32(arg2_wxyz, arg2_yzxx, 1);

            // Use the unzip vector intrinsic to get arg2 -> (z, x, y, w)
            const FloatType arg2_zxyw = vuzp2q_f32(arg2_yzxx, arg2);

            // Use the extract vector intrinsics to get arg2 -> (w, w, w, z)
            const FloatType arg2_wzwx = vextq_f32(arg2, arg2_zwxy, 3);
            const FloatType arg2_wwzw = vextq_f32(arg2, arg2_wzwx, 3);
            const FloatType arg2_wwwz = vextq_f32(arg2, arg2_wwzw, 3);

            // We can now perform the quaternion multiplication.
            const FloatType term1 = vmulq_f32(arg1_yzxw, arg2_zxyw);
            const FloatType term2 = vmulq_f32(arg1_zxyx, arg2_yzxx);
            const FloatType term3 = vmulq_f32(arg1_wwwy, arg2_xyzy);
            const FloatType term4 = vmulq_f32(arg1_xyzz, arg2_wwwz);
            const FloatType partial1 = vsubq_f32(term1, term2);
            const FloatType partial2 = vaddq_f32(term3, term4);
            const FloatType negateW = LoadImmediate(1.0f, 1.0f, 1.0f, -1.0f);
            const FloatType result = vmlaq_f32(partial1, partial2, negateW);
            return result;
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
            const FloatType tmp0 = vtrn1q_f32(rows[0], rows[1]);
            const FloatType tmp1 = vtrn2q_f32(rows[0], rows[1]);
            const FloatType tmp2 = vtrn1q_f32(rows[2], pos);
            const FloatType tmp3 = vtrn2q_f32(rows[2], pos);
            out[0] = vtrn1q_f64(tmp0, tmp2);
            out[1] = vtrn1q_f64(tmp1, tmp3);
            out[2] = vtrn2q_f64(tmp0, tmp2);
        }

        AZ_MATH_INLINE void Vec4::Mat3x4Transpose(const FloatType* __restrict rows, FloatType* __restrict out)
        {
            // abcd    aecg    aei_
            // efgh -> bfdh -> bfj_
            // ijkl    i_k_    cgk_
            // ____    j_l_    dhl_
            const FloatType row3 = vld1q_f32(g_vec0001);
            const FloatType tmp0 = vtrn1q_f32(rows[0], rows[1]);
            const FloatType tmp1 = vtrn2q_f32(rows[0], rows[1]);
            const FloatType tmp2 = vtrn1q_f32(rows[2], row3);
            const FloatType tmp3 = vtrn2q_f32(rows[2], row3);
            out[0] = vtrn1q_f64(tmp0, tmp2);
            out[1] = vtrn1q_f64(tmp1, tmp3);
            out[2] = vtrn2q_f64(tmp0, tmp2);
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
            // abcd    aecg    aeim
            // efgh -> bfdh -> bfjn
            // ijkl    imko    cgko
            // mnop    jnlp    dhlp
            const FloatType tmp0 = vtrn1q_f32(rows[0], rows[1]);
            const FloatType tmp1 = vtrn2q_f32(rows[0], rows[1]);
            const FloatType tmp2 = vtrn1q_f32(rows[2], rows[3]);
            const FloatType tmp3 = vtrn2q_f32(rows[2], rows[3]);
            out[0] = vtrn1q_f64(tmp0, tmp2);
            out[1] = vtrn1q_f64(tmp1, tmp3);
            out[2] = vtrn2q_f64(tmp0, tmp2);
            out[3] = vtrn2q_f64(tmp1, tmp3);
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
            FloatType transposed[4];
            Mat4x4Transpose(rows, transposed);
            return Mat4x4TransposeTransformVector(transposed, vector);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::Mat4x4TransposeTransformVector(const FloatType* __restrict rows, FloatArgType vector)
        {
            return Common::Mat4x4TransposeTransformVector<Vec4>(rows, vector);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec4::Mat4x4TransformPoint3(const FloatType* __restrict rows, Vec3::FloatArgType vector)
        {
            FloatType transposed[4];
            Mat4x4Transpose(rows, transposed);
            const Vec3::FloatType transformed = Vec3::Mat3x3TransposeTransformVector(transposed, vector);
            return Vec3::Add(transformed, transposed[3]);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::ConvertToFloat(Int32ArgType value)
        {
            return NeonQuad::ConvertToFloat(value);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::ConvertToInt(FloatArgType value)
        {
            return NeonQuad::ConvertToInt(value);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::ConvertToIntNearest(FloatArgType value)
        {
            return NeonQuad::ConvertToIntNearest(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::CastToFloat(Int32ArgType value)
        {
            return NeonQuad::CastToFloat(value);
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::CastToInt(FloatArgType value)
        {
            return NeonQuad::CastToInt(value);
        }

        AZ_MATH_INLINE Vec4::FloatType Vec4::ZeroFloat()
        {
            return NeonQuad::ZeroFloat();
        }

        AZ_MATH_INLINE Vec4::Int32Type Vec4::ZeroInt()
        {
            return NeonQuad::ZeroInt();
        }
    }
}
