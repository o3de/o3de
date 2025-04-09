/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Internal/MathTypes.h>

namespace AZ
{
    namespace Simd
    {
        struct Vec4
        {
            static constexpr int32_t ElementCount = 4;

#if   AZ_TRAIT_USE_PLATFORM_SIMD_SSE
            using FloatType = __m128;
            using Int32Type = __m128i;
            using FloatArgType = FloatType;
            using Int32ArgType = Int32Type;
#elif AZ_TRAIT_USE_PLATFORM_SIMD_NEON
            using FloatType = float32x4_t;
            using Int32Type = int32x4_t;
            using FloatArgType = FloatType;
            using Int32ArgType = Int32Type;
#elif AZ_TRAIT_USE_PLATFORM_SIMD_SCALAR
            using FloatType = struct { float   v[ElementCount]; };
            using Int32Type = struct { int32_t v[ElementCount]; };
            using FloatArgType = const FloatType&;
            using Int32ArgType = const Int32Type&;
#endif

            static Vec1::FloatType ToVec1(FloatArgType value);
            static Vec2::FloatType ToVec2(FloatArgType value);
            static Vec3::FloatType ToVec3(FloatArgType value);
            static FloatType FromVec1(Vec1::FloatArgType value); // Generates Vec4 {Vec1.x, Vec1.x, Vec1.x, Vec1.x}
            static FloatType FromVec2(Vec2::FloatArgType value); // Generates Vec4 {Vec2.x, Vec2.y, 0.0f, 0.0f}
            static FloatType FromVec3(Vec3::FloatArgType value); // Generates Vec4 {Vec3.x, Vec3.y, Vec3.z, 0.0f}

            static FloatType LoadAligned(const float* __restrict addr); // addr *must* be 16-byte aligned
            static Int32Type LoadAligned(const int32_t* __restrict addr); // addr *must* be 16-byte aligned
            static FloatType LoadUnaligned(const float* __restrict addr);
            static Int32Type LoadUnaligned(const int32_t* __restrict addr);

            static void StoreAligned(float* __restrict addr, FloatArgType value); // addr *must* be 16-byte aligned
            static void StoreAligned(int32_t* __restrict addr, Int32ArgType value); // addr *must* be 16-byte aligned
            static void StoreUnaligned(float* __restrict addr, FloatArgType value);
            static void StoreUnaligned(int32_t* __restrict addr, Int32ArgType value);

            static void StreamAligned(float* __restrict addr, FloatArgType value); // addr *must* be 16-byte aligned
            static void StreamAligned(int32_t* __restrict addr, Int32ArgType value); // addr *must* be 16-byte aligned

            static float SelectIndex0(FloatArgType value);
            static float SelectIndex1(FloatArgType value);
            static float SelectIndex2(FloatArgType value);
            static float SelectIndex3(FloatArgType value);
            static float AZ_MATH_INLINE SelectFirst (FloatArgType value) { return SelectIndex0(value); } // O3DE_DEPRECATION_NOTICE(PR-16251)
            static float AZ_MATH_INLINE SelectSecond(FloatArgType value) { return SelectIndex1(value); } // O3DE_DEPRECATION_NOTICE(PR-16251)
            static float AZ_MATH_INLINE SelectThird (FloatArgType value) { return SelectIndex2(value); } // O3DE_DEPRECATION_NOTICE(PR-16251)
            static float AZ_MATH_INLINE SelectFourth(FloatArgType value) { return SelectIndex3(value); } // O3DE_DEPRECATION_NOTICE(PR-16251)

            static FloatType Splat(float value);
            static Int32Type Splat(int32_t value);

            static FloatType SplatIndex0(FloatArgType value);
            static FloatType SplatIndex1(FloatArgType value);
            static FloatType SplatIndex2(FloatArgType value);
            static FloatType SplatIndex3(FloatArgType value);
            static FloatType AZ_MATH_INLINE SplatFirst (FloatArgType value) { return SplatIndex0(value); } // O3DE_DEPRECATION_NOTICE(PR-16251)
            static FloatType AZ_MATH_INLINE SplatSecond(FloatArgType value) { return SplatIndex1(value); } // O3DE_DEPRECATION_NOTICE(PR-16251)
            static FloatType AZ_MATH_INLINE SplatThird (FloatArgType value) { return SplatIndex2(value); } // O3DE_DEPRECATION_NOTICE(PR-16251)
            static FloatType AZ_MATH_INLINE SplatFourth(FloatArgType value) { return SplatIndex3(value); } // O3DE_DEPRECATION_NOTICE(PR-16251)

            static FloatType ReplaceIndex0(FloatArgType a, float b);
            static FloatType ReplaceIndex0(FloatArgType a, FloatArgType b);
            static FloatType ReplaceIndex1(FloatArgType a, float b);
            static FloatType ReplaceIndex1(FloatArgType a, FloatArgType b);
            static FloatType ReplaceIndex2(FloatArgType a, float b);
            static FloatType ReplaceIndex2(FloatArgType a, FloatArgType b);
            static FloatType ReplaceIndex3(FloatArgType a, float b);
            static FloatType ReplaceIndex3(FloatArgType a, FloatArgType b);
            static FloatType AZ_MATH_INLINE ReplaceFirst (FloatArgType a,        float b) { return ReplaceIndex0(a, b); } // O3DE_DEPRECATION_NOTICE(PR-16251)
            static FloatType AZ_MATH_INLINE ReplaceFirst (FloatArgType a, FloatArgType b) { return ReplaceIndex0(a, b); } // O3DE_DEPRECATION_NOTICE(PR-16251)
            static FloatType AZ_MATH_INLINE ReplaceSecond(FloatArgType a,        float b) { return ReplaceIndex1(a, b); } // O3DE_DEPRECATION_NOTICE(PR-16251)
            static FloatType AZ_MATH_INLINE ReplaceSecond(FloatArgType a, FloatArgType b) { return ReplaceIndex1(a, b); } // O3DE_DEPRECATION_NOTICE(PR-16251)
            static FloatType AZ_MATH_INLINE ReplaceThird (FloatArgType a,        float b) { return ReplaceIndex2(a, b); } // O3DE_DEPRECATION_NOTICE(PR-16251)
            static FloatType AZ_MATH_INLINE ReplaceThird (FloatArgType a, FloatArgType b) { return ReplaceIndex2(a, b); } // O3DE_DEPRECATION_NOTICE(PR-16251)
            static FloatType AZ_MATH_INLINE ReplaceFourth(FloatArgType a,        float b) { return ReplaceIndex3(a, b); } // O3DE_DEPRECATION_NOTICE(PR-16251)
            static FloatType AZ_MATH_INLINE ReplaceFourth(FloatArgType a, FloatArgType b) { return ReplaceIndex3(a, b); } // O3DE_DEPRECATION_NOTICE(PR-16251)

            static FloatType LoadImmediate(float x, float y, float z, float w);
            static Int32Type LoadImmediate(int32_t x, int32_t y, int32_t z, int32_t w);

            static FloatType Add(FloatArgType arg1, FloatArgType arg2);
            static FloatType Sub(FloatArgType arg1, FloatArgType arg2);
            static FloatType Mul(FloatArgType arg1, FloatArgType arg2);
            static FloatType Madd(FloatArgType mul1, FloatArgType mul2, FloatArgType add);
            static FloatType Div(FloatArgType arg1, FloatArgType arg2);
            static FloatType Abs(FloatArgType value);

            static Int32Type Add(Int32ArgType arg1, Int32ArgType arg2);
            static Int32Type Sub(Int32ArgType arg1, Int32ArgType arg2);
            static Int32Type Mul(Int32ArgType arg1, Int32ArgType arg2);
            static Int32Type Madd(Int32ArgType mul1, Int32ArgType mul2, Int32ArgType add);
            static Int32Type Abs(Int32ArgType value);

            static FloatType Not(FloatArgType value);
            static FloatType And(FloatArgType arg1, FloatArgType arg2);
            static FloatType AndNot(FloatArgType arg1, FloatArgType arg2);
            static FloatType Or(FloatArgType arg1, FloatArgType arg2);
            static FloatType Xor(FloatArgType arg1, FloatArgType arg2);

            static Int32Type Not(Int32ArgType value);
            static Int32Type And(Int32ArgType arg1, Int32ArgType arg2);
            static Int32Type AndNot(Int32ArgType arg1, Int32ArgType arg2);
            static Int32Type Or(Int32ArgType arg1, Int32ArgType arg2);
            static Int32Type Xor(Int32ArgType arg1, Int32ArgType arg2);

            static FloatType Floor(FloatArgType value);
            static FloatType Ceil(FloatArgType value);
            static FloatType Round(FloatArgType value); // Ties to even (banker's rounding)
            static FloatType Truncate(FloatArgType value);
            static FloatType Min(FloatArgType arg1, FloatArgType arg2);
            static FloatType Max(FloatArgType arg1, FloatArgType arg2);
            static FloatType Clamp(FloatArgType value, FloatArgType min, FloatArgType max);

            static Int32Type Min(Int32ArgType arg1, Int32ArgType arg2);
            static Int32Type Max(Int32ArgType arg1, Int32ArgType arg2);
            static Int32Type Clamp(Int32ArgType value, Int32ArgType min, Int32ArgType max);

            static FloatType CmpEq(FloatArgType arg1, FloatArgType arg2);
            static FloatType CmpNeq(FloatArgType arg1, FloatArgType arg2);
            static FloatType CmpGt(FloatArgType arg1, FloatArgType arg2);
            static FloatType CmpGtEq(FloatArgType arg1, FloatArgType arg2);
            static FloatType CmpLt(FloatArgType arg1, FloatArgType arg2);
            static FloatType CmpLtEq(FloatArgType arg1, FloatArgType arg2);

            static bool CmpAllEq(FloatArgType arg1, FloatArgType arg2);
            static bool CmpAllLt(FloatArgType arg1, FloatArgType arg2);
            static bool CmpAllLtEq(FloatArgType arg1, FloatArgType arg2);
            static bool CmpAllGt(FloatArgType arg1, FloatArgType arg2);
            static bool CmpAllGtEq(FloatArgType arg1, FloatArgType arg2);

            static Int32Type CmpEq(Int32ArgType arg1, Int32ArgType arg2);
            static Int32Type CmpNeq(Int32ArgType arg1, Int32ArgType arg2);
            static Int32Type CmpGt(Int32ArgType arg1, Int32ArgType arg2);
            static Int32Type CmpGtEq(Int32ArgType arg1, Int32ArgType arg2);
            static Int32Type CmpLt(Int32ArgType arg1, Int32ArgType arg2);
            static Int32Type CmpLtEq(Int32ArgType arg1, Int32ArgType arg2);

            static bool CmpAllEq(Int32ArgType arg1, Int32ArgType arg2);

            static FloatType Select(FloatArgType arg1, FloatArgType arg2, FloatArgType mask);
            static Int32Type Select(Int32ArgType arg1, Int32ArgType arg2, Int32ArgType mask);

            static FloatType Reciprocal(FloatArgType value); // Slow, but full accuracy
            static FloatType ReciprocalEstimate(FloatArgType value); // Fastest, but roughly half precision on supported platforms

            static FloatType Mod(FloatArgType value, FloatArgType divisor);
            static FloatType Wrap(FloatArgType value, FloatArgType minValue, FloatArgType maxValue);
            static FloatType AngleMod(FloatArgType value);

            static FloatType Sqrt(FloatArgType value); // Slow, but full accuracy
            static FloatType SqrtEstimate(FloatArgType value); // Fastest, but roughly half precision on supported platforms
            static FloatType SqrtInv(FloatArgType value); // Slow, but full accuracy
            static FloatType SqrtInvEstimate(FloatArgType value); // Fastest, but roughly half precision on supported platforms

            static FloatType Sin(FloatArgType value);
            static FloatType Cos(FloatArgType value);
            static void SinCos(FloatArgType value, FloatType& sin, FloatType& cos);
            static FloatType SinCos(FloatArgType angles); // sin(angles[0]), cos(angles[1]), sin(angles[2]), cos(angles[3])
            static FloatType Acos(FloatArgType value);
            static FloatType Atan(FloatArgType value);
            static FloatType Atan2(FloatArgType y, FloatArgType x);
            static FloatType ExpEstimate(FloatArgType value);

            // Vector ops
            static Vec1::FloatType Dot(FloatArgType arg1, FloatArgType arg2);

            static FloatType Normalize(FloatArgType value); // Slow, but full accuracy
            static FloatType NormalizeEstimate(FloatArgType value); // Fastest, but roughly half precision on supported platforms
            static FloatType NormalizeSafe(FloatArgType value, float tolerance); // Slow, but full accuracy
            static FloatType NormalizeSafeEstimate(FloatArgType value, float tolerance); // Fastest, but roughly half precision on supported platforms

            // Quaternion ops
            static FloatType QuaternionMultiply(FloatArgType arg1, FloatArgType arg2); // Multiplies two FloatType's as if they were quaternions stored in x/y/z/w notation
            static Vec3::FloatType QuaternionTransform(FloatArgType quat, Vec3::FloatArgType vec3); // Transforms a Vec3 by a quaternion stored in x/y/z/w notation

            // Plane and frustum ops
            static FloatType ConstructPlane(Vec3::FloatArgType normal, Vec3::FloatArgType point);
            static Vec1::FloatType PlaneDistance(FloatArgType plane, Vec3::FloatArgType point);

            // Matrix ops
            static void Mat3x4InverseFast(const FloatType* __restrict rows, FloatType* __restrict out);
            static void Mat3x4Transpose(const FloatType* __restrict rows, FloatType* __restrict out);
            static void Mat3x4Multiply(const FloatType* __restrict rowsA, const FloatType* __restrict rowsB, FloatType* __restrict out);

            static void Mat4x4InverseFast(const FloatType* __restrict rows, FloatType* __restrict out);
            static void Mat4x4Transpose(const FloatType* __restrict rows, FloatType* __restrict out);
            static void Mat4x4Multiply(const FloatType* __restrict rowsA, const FloatType* __restrict rowsB, FloatType* __restrict out); // Basic 4x4 matrix multiplication
            static void Mat4x4MultiplyAdd(const FloatType* __restrict rowsA, const FloatType* __restrict rowsB, const FloatType* __restrict add, FloatType* __restrict out); // Same as Mat4x4Multiply, but adds the input add into the results
            static void Mat4x4TransposeMultiply(const FloatType* __restrict rowsA, const FloatType* __restrict rowsB, FloatType* __restrict out); // Multiplies two four by four matrices, assuming they are transposed to column major
            static FloatType Mat4x4TransformVector(const FloatType* __restrict rows, FloatArgType vector);
            static FloatType Mat4x4TransposeTransformVector(const FloatType* __restrict rows, FloatArgType vector);
            static Vec3::FloatType Mat4x4TransformPoint3(const FloatType* __restrict rows, Vec3::FloatArgType vector);

            static FloatType ConvertToFloat(Int32ArgType value);
            static Int32Type ConvertToInt(FloatArgType value); // Truncates
            static Int32Type ConvertToIntNearest(FloatArgType value); // Ronuds to nearest int with ties to even (banker's rounding)

            static FloatType CastToFloat(Int32ArgType value);
            static Int32Type CastToInt(FloatArgType value);

            static FloatType ZeroFloat();
            static Int32Type ZeroInt();
        };
    }
}


#if   AZ_TRAIT_USE_PLATFORM_SIMD_SSE
#   include <AzCore/Math/Internal/SimdMathVec4_sse.inl>
#elif AZ_TRAIT_USE_PLATFORM_SIMD_NEON
#   include <AzCore/Math/Internal/SimdMathVec4_neon.inl>
#elif AZ_TRAIT_USE_PLATFORM_SIMD_SCALAR
#   include <AzCore/Math/Internal/SimdMathVec4_scalar.inl>
#endif
