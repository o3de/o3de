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
        AZ_MATH_INLINE Vec1::FloatType Vec3::ToVec1(FloatArgType value)
        {
            return NeonQuad::ToVec1(value);
        }

        AZ_MATH_INLINE Vec2::FloatType Vec3::ToVec2(FloatArgType value)
        {
            return NeonQuad::ToVec2(value);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::FromVec1(Vec1::FloatArgType value)
        {
            return NeonQuad::FromVec1(value); // {value.x, value.x, value.x, unused}
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::FromVec2(Vec2::FloatArgType value)
        {
            return NeonQuad::FromVec2(value); // {value.x, value.y, 0.0f, unused}
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::LoadAligned(const float* __restrict addr)
        {
            return NeonQuad::LoadAligned(addr);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::LoadAligned(const int32_t* __restrict addr)
        {
            return NeonQuad::LoadAligned(addr);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::LoadUnaligned(const float* __restrict addr)
        {
            return NeonQuad::LoadUnaligned(addr);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::LoadUnaligned(const int32_t* __restrict addr)
        {
            return NeonQuad::LoadUnaligned(addr);
        }

        AZ_MATH_INLINE void Vec3::StoreAligned(float* __restrict addr, FloatArgType value)
        {
            NeonQuad::StoreAligned(addr, value);
        }

        AZ_MATH_INLINE void Vec3::StoreAligned(int32_t* __restrict addr, Int32ArgType value)
        {
            NeonQuad::StoreAligned(addr, value);
        }

        AZ_MATH_INLINE void Vec3::StoreUnaligned(float* __restrict addr, FloatArgType value)
        {
            NeonQuad::StoreUnaligned(addr, value);
        }

        AZ_MATH_INLINE void Vec3::StoreUnaligned(int32_t* __restrict addr, Int32ArgType value)
        {
            NeonQuad::StoreUnaligned(addr, value);
        }

        AZ_MATH_INLINE void Vec3::StreamAligned(float* __restrict addr, FloatArgType value)
        {
            NeonQuad::StreamAligned(addr, value);
        }

        AZ_MATH_INLINE void Vec3::StreamAligned(int32_t* __restrict addr, Int32ArgType value)
        {
            NeonQuad::StreamAligned(addr, value);
        }

        AZ_MATH_INLINE float Vec3::SelectIndex0(FloatArgType value)
        {
            return NeonQuad::SelectIndex0(value);
        }

        AZ_MATH_INLINE float Vec3::SelectIndex1(FloatArgType value)
        {
            return NeonQuad::SelectIndex1(value);
        }

        AZ_MATH_INLINE float Vec3::SelectIndex2(FloatArgType value)
        {
            return NeonQuad::SelectIndex2(value);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Splat(float value)
        {
            return NeonQuad::Splat(value);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::Splat(int32_t value)
        {
            return NeonQuad::Splat(value);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::SplatIndex0(FloatArgType value)
        {
            return NeonQuad::SplatIndex0(value);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::SplatIndex1(FloatArgType value)
        {
            return NeonQuad::SplatIndex1(value);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::SplatIndex2(FloatArgType value)
        {
            return NeonQuad::SplatIndex2(value);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::ReplaceIndex0(FloatArgType a, float b)
        {
            return NeonQuad::ReplaceIndex0(a, b);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::ReplaceIndex0(FloatArgType a, FloatArgType b)
        {
            return NeonQuad::ReplaceIndex0(a, b);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::ReplaceIndex1(FloatArgType a, float b)
        {
            return NeonQuad::ReplaceIndex1(a, b);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::ReplaceIndex1(FloatArgType a, FloatArgType b)
        {
            return NeonQuad::ReplaceIndex1(a, b);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::ReplaceIndex2(FloatArgType a, float b)
        {
            return NeonQuad::ReplaceIndex2(a, b);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::ReplaceIndex2(FloatArgType a, FloatArgType b)
        {
            return NeonQuad::ReplaceIndex2(a, b);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::LoadImmediate(float x, float y, float z)
        {
            return NeonQuad::LoadImmediate(x, y, z, 0.0f);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::LoadImmediate(int32_t x, int32_t y, int32_t z)
        {
            return NeonQuad::LoadImmediate(x, y, z, 0);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Add(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonQuad::Add(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Sub(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonQuad::Sub(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Mul(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonQuad::Mul(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Madd(FloatArgType mul1, FloatArgType mul2, FloatArgType add)
        {
            return NeonQuad::Madd(mul1, mul2, add);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Div(FloatArgType arg1, FloatArgType arg2)
        {
            // In Vec3 the last element can be zero, avoid doing division by zero
            arg2 = NeonQuad::ReplaceIndex3(arg2, 1.0f);
            return NeonQuad::Div(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Abs(FloatArgType value)
        {
            return NeonQuad::Abs(value);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::Add(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonQuad::Add(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::Sub(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonQuad::Sub(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::Mul(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonQuad::Mul(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::Madd(Int32ArgType mul1, Int32ArgType mul2, Int32ArgType add)
        {
            return NeonQuad::Madd(mul1, mul2, add);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::Abs(Int32ArgType value)
        {
            return NeonQuad::Abs(value);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Not(FloatArgType value)
        {
            return NeonQuad::Not(value);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::And(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonQuad::And(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::AndNot(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonQuad::AndNot(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Or(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonQuad::Or(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Xor(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonQuad::Xor(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::Not(Int32ArgType value)
        {
            return NeonQuad::Not(value);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::And(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonQuad::And(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::AndNot(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonQuad::AndNot(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::Or(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonQuad::Or(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::Xor(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonQuad::Xor(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Floor(FloatArgType value)
        {
            return NeonQuad::Floor(value);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Ceil(FloatArgType value)
        {
            return NeonQuad::Ceil(value);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Round(FloatArgType value)
        {
            return NeonQuad::Round(value);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Truncate(FloatArgType value)
        {
            return NeonQuad::Truncate(value);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Min(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonQuad::Min(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Max(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonQuad::Max(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Clamp(FloatArgType value, FloatArgType min, FloatArgType max)
        {
            return NeonQuad::Clamp(value, min, max);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::Min(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonQuad::Min(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::Max(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonQuad::Max(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::Clamp(Int32ArgType value, Int32ArgType min, Int32ArgType max)
        {
            return NeonQuad::Clamp(value, min, max);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::CmpEq(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonQuad::CmpEq(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::CmpNeq(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonQuad::CmpNeq(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::CmpGt(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonQuad::CmpGt(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::CmpGtEq(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonQuad::CmpGtEq(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::CmpLt(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonQuad::CmpLt(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::CmpLtEq(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonQuad::CmpLtEq(arg1, arg2);
        }

        AZ_MATH_INLINE bool Vec3::CmpAllEq(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonQuad::CmpFirstThreeEq(arg1, arg2);
        }

        AZ_MATH_INLINE bool Vec3::CmpAllLt(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonQuad::CmpFirstThreeLt(arg1, arg2);
        }

        AZ_MATH_INLINE bool Vec3::CmpAllLtEq(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonQuad::CmpFirstThreeLtEq(arg1, arg2);
        }

        AZ_MATH_INLINE bool Vec3::CmpAllGt(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonQuad::CmpFirstThreeGt(arg1, arg2);
        }

        AZ_MATH_INLINE bool Vec3::CmpAllGtEq(FloatArgType arg1, FloatArgType arg2)
        {
            return NeonQuad::CmpFirstThreeGtEq(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::CmpEq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonQuad::CmpEq(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::CmpNeq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonQuad::CmpNeq(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::CmpGt(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonQuad::CmpGt(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::CmpGtEq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonQuad::CmpGtEq(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::CmpLt(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonQuad::CmpLt(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::CmpLtEq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonQuad::CmpLtEq(arg1, arg2);
        }

        AZ_MATH_INLINE bool Vec3::CmpAllEq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return NeonQuad::CmpFirstThreeEq(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Select(FloatArgType arg1, FloatArgType arg2, FloatArgType mask)
        {
            return NeonQuad::Select(arg1, arg2, mask);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::Select(Int32ArgType arg1, Int32ArgType arg2, Int32ArgType mask)
        {
            return NeonQuad::Select(arg1, arg2, mask);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Reciprocal(FloatArgType value)
        {
            // In Vec3 the last element can be garbage or 0
            // Using (value.x, value.y, value.z, 1) to avoid divisions by 0.
            return NeonQuad::Reciprocal(
                NeonQuad::ReplaceIndex3(value, 1.0f));
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::ReciprocalEstimate(FloatArgType value)
        {
            // In Vec3 the last element can be garbage or 0
            // Using (value.x, value.y, value.z, 1) to avoid divisions by 0.
            return NeonQuad::ReciprocalEstimate(
                NeonQuad::ReplaceIndex3(value, 1.0f));
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Mod(FloatArgType value, FloatArgType divisor)
        {
            return NeonQuad::Mod(value, divisor);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Wrap(FloatArgType value, FloatArgType minValue, FloatArgType maxValue)
        {
            return Common::Wrap<Vec3>(value, minValue, maxValue);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::AngleMod(FloatArgType value)
        {
            return Common::AngleMod<Vec3>(value);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Sqrt(FloatArgType value)
        {
            return NeonQuad::Sqrt(value);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::SqrtEstimate(FloatArgType value)
        {
            return NeonQuad::SqrtEstimate(value);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::SqrtInv(FloatArgType value)
        {
            return NeonQuad::SqrtInv(value);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::SqrtInvEstimate(FloatArgType value)
        {
            return NeonQuad::SqrtInvEstimate(value);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Sin(FloatArgType value)
        {
            return Common::Sin<Vec3>(value);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Cos(FloatArgType value)
        {
            return Common::Cos<Vec3>(value);
        }

        AZ_MATH_INLINE void Vec3::SinCos(FloatArgType value, FloatType& sin, FloatType& cos)
        {
            Common::SinCos<Vec3>(value, sin, cos);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Acos(FloatArgType value)
        {
            return Common::Acos<Vec3>(value);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Atan(FloatArgType value)
        {
            return Common::Atan<Vec3>(value);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Atan2(FloatArgType y, FloatArgType x)
        {
            return Common::Atan2<Vec3>(y, x);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::ExpEstimate(FloatArgType x)
        {
            return Common::ExpEstimate<Vec3>(x);
        }

        AZ_MATH_INLINE Vec1::FloatType Vec3::Dot(FloatArgType arg1, FloatArgType arg2)
        {
            // Ensure we zero out the last element before summing them.
            return Vec1::LoadImmediate(vaddvq_f32(vsetq_lane_f32(0.0f, vmulq_f32(arg1, arg2), 3)));
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Cross(FloatArgType arg1, FloatArgType arg2)
        {
            // We need to return:
            //
            // Vec3(arg1.y * arg2.z - arg1.z * arg2.y,
            //      arg1.z * arg2.x - arg1.x * arg2.z,
            //      arg1.x * arg2.y - arg1.y * arg2.x);
            //
            // but there are no Neon intrinsics to shuffle vector elements,
            // so we must use a combination of other intrinsic instructions
            // to manipulate each input argument vector in order to obtain:
            //
            // arg1 -> (y, z, x, _)
            // arg2 -> (z, x, y, _)
            // arg1 -> (z, x, y, _)
            // arg2 -> (y, z, x, _)

            // Use the transpose and extract vector intrinsics with arg1
            const FloatType arg1_xxzz = vtrn1q_f32(arg1, arg1);
            const FloatType arg1_zxyz = vextq_f32(arg1_xxzz, arg1, 3);
            const FloatType arg1_yzxy = vextq_f32(arg1_zxyz, arg1, 2);
            
            // Use the transpose and extract vector intrinsics with arg2
            const FloatType arg2_xxzz = vtrn1q_f32(arg2, arg2);
            const FloatType arg2_zxyz = vextq_f32(arg2_xxzz, arg2, 3);
            const FloatType arg2_yzxy = vextq_f32(arg2_zxyz, arg2, 2);

            // We can now calculate the cross product,
            // ensuring to zero out the last element.
            const FloatType minuend = vmulq_f32(arg1_yzxy, arg2_zxyz);
            const FloatType crossProduct = vmlsq_f32(minuend, arg1_zxyz, arg2_yzxy);
            return vsetq_lane_f32(0.0f, crossProduct, 3);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Normalize(FloatArgType value)
        {
            return Common::Normalize<Vec3>(value);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::NormalizeEstimate(FloatArgType value)
        {
            return Common::NormalizeEstimate<Vec3>(value);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::NormalizeSafe(FloatArgType value, float tolerance)
        {
            return Common::NormalizeSafe<Vec3>(value, tolerance);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::NormalizeSafeEstimate(FloatArgType value, float tolerance)
        {
            return Common::NormalizeSafeEstimate<Vec3>(value, tolerance);
        }

        AZ_MATH_INLINE void Vec3::Mat3x3Inverse(const FloatType* rows, FloatType* out)
        {
            FloatType cols[3] = { Cross(rows[1], rows[2]),
                                  Cross(rows[2], rows[0]),
                                  Cross(rows[0], rows[1]) };
            const FloatType det = vdupq_lane_f32(Dot(rows[0], cols[0]), 0);

            Mat3x3Transpose(cols, out);

            out[0] = Div(out[0], det);
            out[1] = Div(out[1], det);
            out[2] = Div(out[2], det);
        }

        AZ_MATH_INLINE void Vec3::Mat3x3Adjugate(const FloatType* rows, FloatType* out)
        {
            FloatType cols[3] = { Cross(rows[1], rows[2]),
                                  Cross(rows[2], rows[0]),
                                  Cross(rows[0], rows[1]) };
            Mat3x3Transpose(cols, out);
        }

        AZ_MATH_INLINE void Vec3::Mat3x3Transpose(const FloatType* rows, FloatType* out)
        {
            // abc_    aecg    aei_
            // efg_ -> bf__ -> bfj_
            // ijk_    i_k_    cgk_
            // ____    j___    ____
            const FloatType row3 = vld1q_f32(g_vec0001);
            const FloatType tmp0 = vtrn1q_f32(rows[0], rows[1]);
            const FloatType tmp1 = vtrn2q_f32(rows[0], rows[1]);
            const FloatType tmp2 = vtrn1q_f32(rows[2], row3);
            const FloatType tmp3 = vtrn2q_f32(rows[2], row3);
            out[0] = vtrn1q_f64(tmp0, tmp2);
            out[1] = vtrn1q_f64(tmp1, tmp3);
            out[2] = vtrn2q_f64(tmp0, tmp2);
        }

        AZ_MATH_INLINE void Vec3::Mat3x3Multiply(const FloatType* __restrict rowsA, const FloatType* __restrict rowsB, FloatType* __restrict out)
        {
            Common::Mat3x3Multiply<Vec3>(rowsA, rowsB, out);
        }

        AZ_MATH_INLINE void Vec3::Mat3x3TransposeMultiply(const FloatType* __restrict rowsA, const FloatType* __restrict rowsB, FloatType* __restrict out)
        {
            Common::Mat3x3TransposeMultiply<Vec3>(rowsA, rowsB, out);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Mat3x3TransformVector(const FloatType* __restrict rows, FloatArgType vector)
        {
            return Common::Mat3x3TransformVector<Vec3>(rows, vector);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Mat3x3TransposeTransformVector(const FloatType* __restrict rows, FloatArgType vector)
        {
            return Common::Mat3x3TransposeTransformVector<Vec3>(rows, vector);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::ConvertToFloat(Int32ArgType value)
        {
            return NeonQuad::ConvertToFloat(value);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::ConvertToInt(FloatArgType value)
        {
            return NeonQuad::ConvertToInt(value);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::ConvertToIntNearest(FloatArgType value)
        {
            return NeonQuad::ConvertToIntNearest(value);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::CastToFloat(Int32ArgType value)
        {
            return NeonQuad::CastToFloat(value);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::CastToInt(FloatArgType value)
        {
            return NeonQuad::CastToInt(value);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::ZeroFloat()
        {
            return NeonQuad::ZeroFloat();
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::ZeroInt()
        {
            return NeonQuad::ZeroInt();
        }
    }
}
