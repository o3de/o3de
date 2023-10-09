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
        AZ_MATH_INLINE Vec1::FloatType Vec3::ToVec1(FloatArgType value)
        {
            return value;
        }

        AZ_MATH_INLINE Vec2::FloatType Vec3::ToVec2(FloatArgType value)
        {
            return value;
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::FromVec1(Vec1::FloatArgType value)
        {
            // Coming from a Vec1 the last 3 elements could be garbage.
            return Sse::SplatIndex0(value); // {value.x, value.x, value.x, value.x}
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::FromVec2(Vec2::FloatArgType value)
        {
            // Coming from a Vec2 the last 2 elements could be garbage.
            return Sse::ReplaceIndex2(value, 0.0f); // {value.x, value.y, 0.0f, unused}
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::LoadAligned(const float* __restrict addr)
        {
            return Sse::LoadAligned(addr);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::LoadAligned(const int32_t* __restrict addr)
        {
            return Sse::LoadAligned(addr);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::LoadUnaligned(const float* __restrict addr)
        {
            return Sse::LoadUnaligned(addr);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::LoadUnaligned(const int32_t* __restrict addr)
        {
            return Sse::LoadUnaligned(addr);
        }

        AZ_MATH_INLINE void Vec3::StoreAligned(float* __restrict addr, FloatArgType value)
        {
            return Sse::StoreAligned(addr, value);
        }

        AZ_MATH_INLINE void Vec3::StoreAligned(int32_t* __restrict addr, Int32ArgType value)
        {
            return Sse::StoreAligned(addr, value);
        }

        AZ_MATH_INLINE void Vec3::StoreUnaligned(float* __restrict addr, FloatArgType value)
        {
            return Sse::StoreUnaligned(addr, value);
        }

        AZ_MATH_INLINE void Vec3::StoreUnaligned(int32_t* __restrict addr, Int32ArgType value)
        {
            return Sse::StoreUnaligned(addr, value);
        }

        AZ_MATH_INLINE void Vec3::StreamAligned(float* __restrict addr, FloatArgType value)
        {
            return Sse::StreamAligned(addr, value);
        }

        AZ_MATH_INLINE void Vec3::StreamAligned(int32_t* __restrict addr, Int32ArgType value)
        {
            return Sse::StreamAligned(addr, value);
        }

        AZ_MATH_INLINE float Vec3::SelectIndex0(FloatArgType value)
        {
            return Sse::SelectIndex0(value);
        }

        AZ_MATH_INLINE float Vec3::SelectIndex1(FloatArgType value)
        {
            return Sse::SelectIndex0(Sse::SplatIndex1(value));
        }

        AZ_MATH_INLINE float Vec3::SelectIndex2(FloatArgType value)
        {
            return Sse::SelectIndex0(Sse::SplatIndex2(value));
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Splat(float value)
        {
            return Sse::Splat(value);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::Splat(int32_t value)
        {
            return Sse::Splat(value);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::SplatIndex0(FloatArgType value)
        {
            return Sse::SplatIndex0(value);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::SplatIndex1(FloatArgType value)
        {
            return Sse::SplatIndex1(value);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::SplatIndex2(FloatArgType value)
        {
            return Sse::SplatIndex2(value);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::ReplaceIndex0(FloatArgType a, float b)
        {
            return Sse::ReplaceIndex0(a, b);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::ReplaceIndex0(FloatArgType a, FloatArgType b)
        {
            return Sse::ReplaceIndex0(a, b);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::ReplaceIndex1(FloatArgType a, float b)
        {
            return Sse::ReplaceIndex1(a, b);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::ReplaceIndex1(FloatArgType a, FloatArgType b)
        {
            return Sse::ReplaceIndex1(a, b);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::ReplaceIndex2(FloatArgType a, float b)
        {
            return Sse::ReplaceIndex2(a, b);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::ReplaceIndex2(FloatArgType a, FloatArgType b)
        {
            return Sse::ReplaceIndex2(a, b);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::LoadImmediate(float x, float y, float z)
        {
            return Sse::LoadImmediate(x, y, z, 0.0f);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::LoadImmediate(int32_t x, int32_t y, int32_t z)
        {
            return Sse::LoadImmediate(x, y, z, 0);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Add(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::Add(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Sub(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::Sub(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Mul(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::Mul(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Madd(FloatArgType mul1, FloatArgType mul2, FloatArgType add)
        {
            return Sse::Madd(mul1, mul2, add);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Div(FloatArgType arg1, FloatArgType arg2)
        {
            // In Vec3 the last element can be zero, avoid doing division by zero
            arg2 = Sse::ReplaceIndex3(arg2, 1.0f);
            return Sse::Div(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Abs(FloatArgType value)
        {
            return Sse::Abs(value);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::Add(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::Add(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::Sub(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::Sub(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::Mul(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::Mul(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::Madd(Int32ArgType mul1, Int32ArgType mul2, Int32ArgType add)
        {
            return Sse::Madd(mul1, mul2, add);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::Abs(Int32ArgType value)
        {
            return Sse::Abs(value);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Not(FloatArgType value)
        {
            return Sse::Not(value);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::And(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::And(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::AndNot(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::AndNot(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Or (FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::Or(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Xor(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::Xor(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::Not(Int32ArgType value)
        {
            return Sse::Not(value);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::And(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::And(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::AndNot(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::AndNot(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::Or(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::Or(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::Xor(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::Xor(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Floor(FloatArgType value)
        {
            return Sse::Floor(value);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Ceil(FloatArgType value)
        {
            return Sse::Ceil(value);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Round(FloatArgType value)
        {
            return Sse::Round(value);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Truncate(FloatArgType value)
        {
            return Sse::Truncate(value);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Min(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::Min(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Max(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::Max(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Clamp(FloatArgType value, FloatArgType min, FloatArgType max)
        {
            return Sse::Clamp(value, min, max);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::Min(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::Min(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::Max(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::Max(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::Clamp(Int32ArgType value, Int32ArgType min, Int32ArgType max)
        {
            return Sse::Clamp(value, min, max);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::CmpEq(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::CmpEq(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::CmpNeq(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::CmpNeq(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::CmpGt(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::CmpGt(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::CmpGtEq(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::CmpGtEq(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::CmpLt(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::CmpLt(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::CmpLtEq(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::CmpLtEq(arg1, arg2);
        }

        AZ_MATH_INLINE bool Vec3::CmpAllEq(FloatArgType arg1, FloatArgType arg2)
        {
            // Only check the first three bits for Vector3
            return Sse::CmpAllEq(arg1, arg2, 0b0111);
        }

        AZ_MATH_INLINE bool Vec3::CmpAllLt(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::CmpAllLt(arg1, arg2, 0b0111);
        }

        AZ_MATH_INLINE bool Vec3::CmpAllLtEq(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::CmpAllLtEq(arg1, arg2, 0b0111);
        }

        AZ_MATH_INLINE bool Vec3::CmpAllGt(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::CmpAllGt(arg1, arg2, 0b0111);
        }

        AZ_MATH_INLINE bool Vec3::CmpAllGtEq(FloatArgType arg1, FloatArgType arg2)
        {
            return Sse::CmpAllGtEq(arg1, arg2, 0b0111);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::CmpEq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::CmpEq(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::CmpNeq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::CmpNeq(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::CmpGt(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::CmpGt(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::CmpGtEq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::CmpGtEq(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::CmpLt(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::CmpLt(arg1, arg2);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::CmpLtEq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::CmpLtEq(arg1, arg2);
        }

        AZ_MATH_INLINE bool Vec3::CmpAllEq(Int32ArgType arg1, Int32ArgType arg2)
        {
            return Sse::CmpAllEq(arg1, arg2, 0b0111);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Select(FloatArgType arg1, FloatArgType arg2, FloatArgType mask)
        {
            return Sse::Select(arg1, arg2, mask);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::Select(Int32ArgType arg1, Int32ArgType arg2, Int32ArgType mask)
        {
            return Sse::Select(arg1, arg2, mask);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Reciprocal(FloatArgType value)
        {
            // In Vec3 the last element (w) can be garbage or 0
            // Using (value.x, value.y, value.z, 1) to avoid divisions by 0.
            return Sse::Reciprocal(
                Sse::ReplaceIndex3(value, 1.0f));
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::ReciprocalEstimate(FloatArgType value)
        {
            // In Vec3 the last element (w) can be garbage or 0
            // Using (value.x, value.y, value.z, 1) to avoid divisions by 0.
            return Sse::ReciprocalEstimate(
                Sse::ReplaceIndex3(value, 1.0f)
            );
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Mod(FloatArgType value, FloatArgType divisor)
        {
            return Sse::Mod(value, divisor);
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
            return Sse::Sqrt(value);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::SqrtEstimate(FloatArgType value)
        {
            return Sse::SqrtEstimate(value);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::SqrtInv(FloatArgType value)
        {
            value = Sse::ReplaceIndex3(value, 1.0f);
            return Sse::SqrtInv(value);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::SqrtInvEstimate(FloatArgType value)
        {
            return Sse::SqrtInvEstimate(value);
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
#if 0
            return _mm_dp_ps(arg1, arg2, 0x77);
#else
            const FloatType x2  = Mul(arg1, arg2);
            const FloatType xy  = Add(SplatIndex1(x2), x2);
            const FloatType xyz = Add(SplatIndex2(x2), xy);
            return SplatIndex0(xyz);
#endif
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::Cross(FloatArgType arg1, FloatArgType arg2)
        {
            // Vec3(y * vector.z - z * vector.y, z * vector.x - x * vector.z, x * vector.y - y * a_Vector.x);
            const FloatType arg1_yzx = _mm_shuffle_ps(arg1, arg1, _MM_SHUFFLE(3, 0, 2, 1));
            const FloatType arg2_yzx = _mm_shuffle_ps(arg2, arg2, _MM_SHUFFLE(3, 0, 2, 1));
            const FloatType partial = Sub(Mul(arg1, arg2_yzx), Mul(arg1_yzx, arg2));
            return _mm_shuffle_ps(partial, partial, _MM_SHUFFLE(3, 0, 2, 1));
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

        AZ_MATH_INLINE void Vec3::Mat3x3Inverse(const FloatType* __restrict rows, FloatType* __restrict out)
        {
            const FloatType row0YZX = _mm_shuffle_ps(rows[0], rows[0], _MM_SHUFFLE(0, 0, 2, 1));
            const FloatType row0ZXY = _mm_shuffle_ps(rows[0], rows[0], _MM_SHUFFLE(0, 1, 0, 2));
            const FloatType row1YZX = _mm_shuffle_ps(rows[1], rows[1], _MM_SHUFFLE(0, 0, 2, 1));
            const FloatType row1ZXY = _mm_shuffle_ps(rows[1], rows[1], _MM_SHUFFLE(0, 1, 0, 2));
            const FloatType row2YZX = _mm_shuffle_ps(rows[2], rows[2], _MM_SHUFFLE(0, 0, 2, 1));
            const FloatType row2ZXY = _mm_shuffle_ps(rows[2], rows[2], _MM_SHUFFLE(0, 1, 0, 2));

            FloatType cols[3] = { Sub(Mul(row1YZX, row2ZXY), Mul(row1ZXY, row2YZX))
                                , Sub(Mul(row2YZX, row0ZXY), Mul(row2ZXY, row0YZX))
                                , Sub(Mul(row0YZX, row1ZXY), Mul(row0ZXY, row1YZX)) };

            const FloatType detXYZ = Mul(rows[0], cols[0]);
            const FloatType detTmp = Add(detXYZ, _mm_shuffle_ps(detXYZ, detXYZ, _MM_SHUFFLE(0, 1, 0, 1)));
            const FloatType det    = Add(detTmp, _mm_shuffle_ps(detXYZ, detXYZ, _MM_SHUFFLE(0, 0, 2, 2)));

            Mat3x3Transpose(cols, cols);

            out[0] = Div(cols[0], det);
            out[1] = Div(cols[1], det);
            out[2] = Div(cols[2], det);
        }

        AZ_MATH_INLINE void Vec3::Mat3x3Adjugate(const FloatType* __restrict rows, FloatType* __restrict out)
        {
            const FloatType row0YZX = _mm_shuffle_ps(rows[0], rows[0], _MM_SHUFFLE(0, 0, 2, 1));
            const FloatType row0ZXY = _mm_shuffle_ps(rows[0], rows[0], _MM_SHUFFLE(0, 1, 0, 2));
            const FloatType row1YZX = _mm_shuffle_ps(rows[1], rows[1], _MM_SHUFFLE(0, 0, 2, 1));
            const FloatType row1ZXY = _mm_shuffle_ps(rows[1], rows[1], _MM_SHUFFLE(0, 1, 0, 2));
            const FloatType row2YZX = _mm_shuffle_ps(rows[2], rows[2], _MM_SHUFFLE(0, 0, 2, 1));
            const FloatType row2ZXY = _mm_shuffle_ps(rows[2], rows[2], _MM_SHUFFLE(0, 1, 0, 2));

            FloatType cols[3] = { Sub(Mul(row1YZX, row2ZXY), Mul(row1ZXY, row2YZX))
                                , Sub(Mul(row2YZX, row0ZXY), Mul(row2ZXY, row0YZX))
                                , Sub(Mul(row0YZX, row1ZXY), Mul(row0ZXY, row1YZX)) };

            Mat3x3Transpose(cols, out);
        }

        AZ_MATH_INLINE void Vec3::Mat3x3Transpose(const FloatType* __restrict rows, FloatType* __restrict out)
        {
            const FloatType tmp0 = _mm_unpacklo_ps(rows[0], rows[1]);
            const FloatType tmp1 = _mm_unpackhi_ps(rows[0], rows[1]);
            out[0] = _mm_movelh_ps (tmp0, rows[2]);
            out[1] = _mm_shuffle_ps(tmp0, rows[2], _MM_SHUFFLE(3, 1, 3, 2));
            out[2] = _mm_shuffle_ps(tmp1, rows[2], _MM_SHUFFLE(3, 2, 1, 0));
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
            return Sse::ConvertToFloat(value);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::ConvertToInt(FloatArgType value)
        {
            return Sse::ConvertToInt(value);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::ConvertToIntNearest(FloatArgType value)
        {
            return Sse::ConvertToIntNearest(value);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::CastToFloat(Int32ArgType value)
        {
            return Sse::CastToFloat(value);
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::CastToInt(FloatArgType value)
        {
            return Sse::CastToInt(value);
        }

        AZ_MATH_INLINE Vec3::FloatType Vec3::ZeroFloat()
        {
            return Sse::ZeroFloat();
        }

        AZ_MATH_INLINE Vec3::Int32Type Vec3::ZeroInt()
        {
            return Sse::ZeroInt();
        }
    }
}
