/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AZ
{
    namespace Simd
    {
        namespace NeonQuad
        {
            AZ_MATH_INLINE float32x2_t ToVec1(float32x4_t value)
            {
                return vget_low_f32(value);
            }

            AZ_MATH_INLINE float32x2_t ToVec2(float32x4_t value)
            {
                return vget_low_f32(value);
            }

            AZ_MATH_INLINE float32x4_t FromVec1(float32x2_t value)
            {
                value = NeonDouble::SplatIndex0(value);
                return vcombine_f32(value, value); // {value.x, value.x, value.x, value.x}
            }

            AZ_MATH_INLINE float32x4_t FromVec2(float32x2_t value)
            {
                return vcombine_f32(value, vmov_n_f32(0.0f)); // {value.x, value.y, 0.0f, 0.0f}
            }

            AZ_MATH_INLINE float32x4_t LoadAligned(const float* __restrict addr)
            {
                AZ_MATH_ASSERT(IsAligned<16>(addr), "Alignment failure");
                return vld1q_f32(addr);
            }

            AZ_MATH_INLINE int32x4_t LoadAligned(const int32_t* __restrict addr)
            {
                AZ_MATH_ASSERT(IsAligned<16>(addr), "Alignment failure");
                return vld1q_s32(addr);
            }

            AZ_MATH_INLINE float32x4_t LoadUnaligned(const float* __restrict addr)
            {
                return vld1q_f32(addr);
            }

            AZ_MATH_INLINE int32x4_t LoadUnaligned(const int32_t* __restrict addr)
            {
                return vld1q_s32(addr);
            }

            AZ_MATH_INLINE void StoreAligned(float* __restrict addr, float32x4_t value)
            {
                AZ_MATH_ASSERT(IsAligned<16>(addr), "Alignment failure");
                vst1q_f32(addr, value);
            }

            AZ_MATH_INLINE void StoreAligned(int32_t* __restrict addr, int32x4_t value)
            {
                AZ_MATH_ASSERT(IsAligned<16>(addr), "Alignment failure");
                vst1q_s32(addr, value);
            }

            AZ_MATH_INLINE void StoreUnaligned(float* __restrict addr, float32x4_t value)
            {
                vst1q_f32(addr, value);
            }

            AZ_MATH_INLINE void StoreUnaligned(int32_t* __restrict addr, int32x4_t value)
            {
                vst1q_s32(addr, value);
            }

            AZ_MATH_INLINE void StreamAligned(float* __restrict addr, float32x4_t value)
            {
                AZ_MATH_ASSERT(IsAligned<16>(addr), "Alignment failure");
                vst1q_f32(addr, value);
            }

            AZ_MATH_INLINE void StreamAligned(int32_t* __restrict addr, int32x4_t value)
            {
                AZ_MATH_ASSERT(IsAligned<16>(addr), "Alignment failure");
                vst1q_s32(addr, value);
            }

            AZ_MATH_INLINE float SelectIndex0(float32x4_t value)
            {
                return vgetq_lane_f32(value, 0);
            }

            AZ_MATH_INLINE float SelectIndex1(float32x4_t value)
            {
                return vgetq_lane_f32(value, 1);
            }

            AZ_MATH_INLINE float SelectIndex2(float32x4_t value)
            {
                return vgetq_lane_f32(value, 2);
            }

            AZ_MATH_INLINE float SelectIndex3(float32x4_t value)
            {
                return vgetq_lane_f32(value, 3);
            }

            AZ_MATH_INLINE float32x4_t Splat(float value)
            {
                return vdupq_n_f32(value);
            }

            AZ_MATH_INLINE int32x4_t Splat(int32_t value)
            {
                return vdupq_n_s32(value);
            }

            AZ_MATH_INLINE float32x4_t SplatIndex0(float32x4_t value)
            {
                return vdupq_laneq_f32(value, 0);
            }

            AZ_MATH_INLINE float32x4_t SplatIndex1(float32x4_t value)
            {
                return vdupq_laneq_f32(value, 1);
            }

            AZ_MATH_INLINE float32x4_t SplatIndex2(float32x4_t value)
            {
                return vdupq_laneq_f32(value, 2);
            }

            AZ_MATH_INLINE float32x4_t SplatIndex3(float32x4_t value)
            {
                return vdupq_laneq_f32(value, 3);
            }

            AZ_MATH_INLINE float32x4_t ReplaceIndex0(float32x4_t a, float b)
            {
                return vsetq_lane_f32(b, a, 0);
            }

            AZ_MATH_INLINE float32x4_t ReplaceIndex0(float32x4_t a, float32x4_t b)
            {
                return ReplaceIndex0(a, SelectIndex0(b));
            }

            AZ_MATH_INLINE float32x4_t ReplaceIndex1(float32x4_t a, float b)
            {
                return vsetq_lane_f32(b, a, 1);
            }

            AZ_MATH_INLINE float32x4_t ReplaceIndex1(float32x4_t a, float32x4_t b)
            {
                return ReplaceIndex1(a, SelectIndex1(b));
            }

            AZ_MATH_INLINE float32x4_t ReplaceIndex2(float32x4_t a, float b)
            {
                return vsetq_lane_f32(b, a, 2);
            }

            AZ_MATH_INLINE float32x4_t ReplaceIndex2(float32x4_t a, float32x4_t b)
            {
                return ReplaceIndex2(a, SelectIndex2(b));
            }

            AZ_MATH_INLINE float32x4_t ReplaceIndex3(float32x4_t a, float b)
            {
                return vsetq_lane_f32(b, a, 3);
            }

            AZ_MATH_INLINE float32x4_t ReplaceIndex3(float32x4_t a, float32x4_t b)
            {
                return ReplaceIndex3(a, SelectIndex3(b));
            }

            AZ_MATH_INLINE float32x4_t LoadImmediate(float x, float y, float z, float w)
            {
                const float values[4] = { x, y, z, w };
                return vld1q_f32(values);
            }

            AZ_MATH_INLINE int32x4_t LoadImmediate(int32_t x, int32_t y, int32_t z, int32_t w)
            {
                const int32_t values[4] = { x, y, z, w };
                return vld1q_s32(values);
            }

            AZ_MATH_INLINE float32x4_t Add(float32x4_t arg1, float32x4_t arg2)
            {
                return vaddq_f32(arg1, arg2);
            }

            AZ_MATH_INLINE float32x4_t Sub(float32x4_t arg1, float32x4_t arg2)
            {
                return vsubq_f32(arg1, arg2);
            }

            AZ_MATH_INLINE float32x4_t Mul(float32x4_t arg1, float32x4_t arg2)
            {
                return vmulq_f32(arg1, arg2);
            }

            AZ_MATH_INLINE float32x4_t Madd(float32x4_t mul1, float32x4_t mul2, float32x4_t add)
            {
                return vmlaq_f32(add, mul1, mul2);
            }

            AZ_MATH_INLINE float32x4_t Div(float32x4_t arg1, float32x4_t arg2)
            {
                return vdivq_f32(arg1, arg2);
            }

            AZ_MATH_INLINE float32x4_t Abs(float32x4_t value)
            {
                return vabsq_f32(value);
            }

            AZ_MATH_INLINE int32x4_t Add(int32x4_t arg1, int32x4_t arg2)
            {
                return vaddq_s32(arg1, arg2);
            }

            AZ_MATH_INLINE int32x4_t Sub(int32x4_t arg1, int32x4_t arg2)
            {
                return vsubq_s32(arg1, arg2);
            }

            AZ_MATH_INLINE int32x4_t Mul(int32x4_t arg1, int32x4_t arg2)
            {
                return vmulq_s32(arg1, arg2);
            }

            AZ_MATH_INLINE int32x4_t Madd(int32x4_t mul1, int32x4_t mul2, int32x4_t add)
            {
                return vmlaq_s32(add, mul1, mul2);
            }

            AZ_MATH_INLINE int32x4_t Abs(int32x4_t value)
            {
                return vabsq_s32(value);
            }

            AZ_MATH_INLINE float32x4_t Not(float32x4_t value)
            {
                return vreinterpretq_f32_s32(vmvnq_s32(vreinterpretq_s32_f32(value)));
            }

            AZ_MATH_INLINE float32x4_t And(float32x4_t arg1, float32x4_t arg2)
            {
                return vreinterpretq_f32_s32(vandq_s32(vreinterpretq_s32_f32(arg1), vreinterpretq_s32_f32(arg2)));
            }

            AZ_MATH_INLINE float32x4_t AndNot(float32x4_t arg1, float32x4_t arg2)
            {
                return vreinterpretq_f32_s32(vandq_s32(vmvnq_s32(vreinterpretq_s32_f32(arg1)), vreinterpretq_s32_f32(arg2)));
            }

            AZ_MATH_INLINE float32x4_t Or(float32x4_t arg1, float32x4_t arg2)
            {
                return vreinterpretq_f32_s32(vorrq_s32(vreinterpretq_s32_f32(arg1), vreinterpretq_s32_f32(arg2)));
            }

            AZ_MATH_INLINE float32x4_t Xor(float32x4_t arg1, float32x4_t arg2)
            {
                return vreinterpretq_f32_s32(veorq_s32(vreinterpretq_s32_f32(arg1), vreinterpretq_s32_f32(arg2)));
            }

            AZ_MATH_INLINE int32x4_t Not(int32x4_t value)
            {
                return vmvnq_s32(value);
            }

            AZ_MATH_INLINE int32x4_t And(int32x4_t arg1, int32x4_t arg2)
            {
                return vandq_s32(arg1, arg2);
            }

            AZ_MATH_INLINE int32x4_t AndNot(int32x4_t arg1, int32x4_t arg2)
            {
                return And(Not(arg1), arg2);
            }

            AZ_MATH_INLINE int32x4_t Or(int32x4_t arg1, int32x4_t arg2)
            {
                return vorrq_s32(arg1, arg2);
            }

            AZ_MATH_INLINE int32x4_t Xor(int32x4_t arg1, int32x4_t arg2)
            {
                return veorq_s32(arg1, arg2);
            }

            AZ_MATH_INLINE float32x4_t Floor(float32x4_t value)
            {
                return vrndmq_f32(value);
            }

            AZ_MATH_INLINE float32x4_t Ceil(float32x4_t value)
            {
                return vrndpq_f32(value);
            }

            AZ_MATH_INLINE float32x4_t Round(float32x4_t value)
            {
                return vrndnq_f32(value);
            }

            AZ_MATH_INLINE float32x4_t Truncate(float32x4_t value)
            {
                return vcvtq_f32_s32(vcvtq_s32_f32(value));
            }

            AZ_MATH_INLINE float32x4_t Min(float32x4_t arg1, float32x4_t arg2)
            {
                return vminq_f32(arg1, arg2);
            }

            AZ_MATH_INLINE float32x4_t Max(float32x4_t arg1, float32x4_t arg2)
            {
                return vmaxq_f32(arg1, arg2);
            }

            AZ_MATH_INLINE float32x4_t Clamp(float32x4_t value, float32x4_t min, float32x4_t max)
            {
                return vmaxq_f32(min, vminq_f32(value, max));
            }

            AZ_MATH_INLINE int32x4_t Min(int32x4_t arg1, int32x4_t arg2)
            {
                return vminq_s32(arg1, arg2);
            }

            AZ_MATH_INLINE int32x4_t Max(int32x4_t arg1, int32x4_t arg2)
            {
                return vmaxq_s32(arg1, arg2);
            }

            AZ_MATH_INLINE int32x4_t Clamp(int32x4_t value, int32x4_t min, int32x4_t max)
            {
                return vmaxq_s32(min, vminq_s32(value, max));
            }

            AZ_MATH_INLINE float32x4_t CmpEq(float32x4_t arg1, float32x4_t arg2)
            {
                return vreinterpretq_f32_s32(vceqq_f32(arg1, arg2));
            }

            AZ_MATH_INLINE float32x4_t CmpNeq(float32x4_t arg1, float32x4_t arg2)
            {
                return vreinterpretq_f32_s32(vmvnq_s32(vceqq_f32(arg1, arg2)));
            }

            AZ_MATH_INLINE float32x4_t CmpGt(float32x4_t arg1, float32x4_t arg2)
            {
                return vreinterpretq_f32_s32(vcgtq_f32(arg1, arg2));
            }

            AZ_MATH_INLINE float32x4_t CmpGtEq(float32x4_t arg1, float32x4_t arg2)
            {
                return vreinterpretq_f32_s32(vcgeq_f32(arg1, arg2));
            }

            AZ_MATH_INLINE float32x4_t CmpLt(float32x4_t arg1, float32x4_t arg2)
            {
                return vreinterpretq_f32_s32(vcltq_f32(arg1, arg2));
            }

            AZ_MATH_INLINE float32x4_t CmpLtEq(float32x4_t arg1, float32x4_t arg2)
            {
                return vreinterpretq_f32_s32(vcleq_f32(arg1, arg2));
            }

            AZ_MATH_INLINE bool CmpAllEq(float32x4_t arg1, float32x4_t arg2)
            {
                return Neon::AreAllLanesTrue(vceqq_f32(arg1, arg2));
            }

            AZ_MATH_INLINE bool CmpAllLt(float32x4_t arg1, float32x4_t arg2)
            {
                return Neon::AreAllLanesTrue(vcltq_f32(arg1, arg2));
            }

            AZ_MATH_INLINE bool CmpAllLtEq(float32x4_t arg1, float32x4_t arg2)
            {
                return Neon::AreAllLanesTrue(vcleq_f32(arg1, arg2));
            }

            AZ_MATH_INLINE bool CmpAllGt(float32x4_t arg1, float32x4_t arg2)
            {
                return Neon::AreAllLanesTrue(vcgtq_f32(arg1, arg2));
            }

            AZ_MATH_INLINE bool CmpAllGtEq(float32x4_t arg1, float32x4_t arg2)
            {
                return Neon::AreAllLanesTrue(vcgeq_f32(arg1, arg2));
            }

            AZ_MATH_INLINE bool CmpFirstThreeEq(float32x4_t arg1, float32x4_t arg2)
            {
                return Neon::AreFirstThreeLanesTrue(vceqq_f32(arg1, arg2));
            }

            AZ_MATH_INLINE bool CmpFirstThreeLt(float32x4_t arg1, float32x4_t arg2)
            {
                return Neon::AreFirstThreeLanesTrue(vcltq_f32(arg1, arg2));
            }

            AZ_MATH_INLINE bool CmpFirstThreeLtEq(float32x4_t arg1, float32x4_t arg2)
            {
                return Neon::AreFirstThreeLanesTrue(vcleq_f32(arg1, arg2));
            }

            AZ_MATH_INLINE bool CmpFirstThreeGt(float32x4_t arg1, float32x4_t arg2)
            {
                return Neon::AreFirstThreeLanesTrue(vcgtq_f32(arg1, arg2));
            }

            AZ_MATH_INLINE bool CmpFirstThreeGtEq(float32x4_t arg1, float32x4_t arg2)
            {
                return Neon::AreFirstThreeLanesTrue(vcgeq_f32(arg1, arg2));
            }

            AZ_MATH_INLINE int32x4_t CmpEq(int32x4_t arg1, int32x4_t arg2)
            {
                return vceqq_s32(arg1, arg2);
            }

            AZ_MATH_INLINE int32x4_t CmpNeq(int32x4_t arg1, int32x4_t arg2)
            {
                return vmvnq_s32(vceqq_s32(arg1, arg2));
            }

            AZ_MATH_INLINE int32x4_t CmpGt(int32x4_t arg1, int32x4_t arg2)
            {
                return vcgtq_s32(arg1, arg2);
            }

            AZ_MATH_INLINE int32x4_t CmpGtEq(int32x4_t arg1, int32x4_t arg2)
            {
                return vcgeq_s32(arg1, arg2);
            }

            AZ_MATH_INLINE int32x4_t CmpLt(int32x4_t arg1, int32x4_t arg2)
            {
                return vcltq_s32(arg1, arg2);
            }

            AZ_MATH_INLINE int32x4_t CmpLtEq(int32x4_t arg1, int32x4_t arg2)
            {
                return vcleq_s32(arg1, arg2);
            }

            AZ_MATH_INLINE bool CmpAllEq(int32x4_t arg1, int32x4_t arg2)
            {
                return Neon::AreAllLanesTrue(vceqq_s32(arg1, arg2));
            }

            AZ_MATH_INLINE bool CmpFirstThreeEq(int32x4_t arg1, int32x4_t arg2)
            {
                return Neon::AreFirstThreeLanesTrue(vceqq_s32(arg1, arg2));
            }

            AZ_MATH_INLINE float32x4_t Select(float32x4_t arg1, float32x4_t arg2, float32x4_t mask)
            {
                return vbslq_f32(mask, arg1, arg2);
            }

            AZ_MATH_INLINE int32x4_t Select(int32x4_t arg1, int32x4_t arg2, int32x4_t mask)
            {
                return vbslq_s32(mask, arg1, arg2);
            }

            AZ_MATH_INLINE float32x4_t Reciprocal(float32x4_t value)
            {
                const float32x4_t estimate = vrecpeq_f32(value);
                // Do one Newton Raphson iteration to improve precision
                return vmulq_f32(vrecpsq_f32(value, estimate), estimate);
            }

            AZ_MATH_INLINE float32x4_t ReciprocalEstimate(float32x4_t value)
            {
                return vrecpeq_f32(value);
            }

            AZ_MATH_INLINE float32x4_t Mod(float32x4_t value, float32x4_t divisor)
            {
                return Sub(value, Mul(Truncate(Div(value, divisor)), divisor));
            }

            AZ_MATH_INLINE float32x4_t Sqrt(float32x4_t value)
            {
                return vsqrtq_f32(value);
            }

            AZ_MATH_INLINE float32x4_t SqrtEstimate(float32x4_t value)
            {
                return vrecpeq_f32(vrsqrteq_f32(value));
            }

            AZ_MATH_INLINE float32x4_t SqrtInv(float32x4_t value)
            {
                const float32x4_t estimate = vrsqrteq_f32(value);
                // Do one Newton Raphson iteration to improve precision
                return vmulq_f32(vrsqrtsq_f32(vmulq_f32(value, estimate), estimate), estimate);
            }

            AZ_MATH_INLINE float32x4_t SqrtInvEstimate(float32x4_t value)
            {
                return vrsqrteq_f32(value); // Faster, but roughly half the precision (12ish bits rather than 23ish)
            }

            AZ_MATH_INLINE float32x4_t ConvertToFloat(int32x4_t value)
            {
                return vcvtq_f32_s32(value);
            }

            AZ_MATH_INLINE int32x4_t ConvertToInt(float32x4_t value)
            {
                return vcvtq_s32_f32(value);
            }

            AZ_MATH_INLINE int32x4_t ConvertToIntNearest(float32x4_t value)
            {
                return vcvtnq_s32_f32(value);
            }

            AZ_MATH_INLINE float32x4_t CastToFloat(int32x4_t value)
            {
                return vreinterpretq_f32_s32(value);
            }

            AZ_MATH_INLINE int32x4_t CastToInt(float32x4_t value)
            {
                return vreinterpretq_s32_f32(value);
            }

            AZ_MATH_INLINE float32x4_t ZeroFloat()
            {
                return vmovq_n_f32(0.0f);
            }

            AZ_MATH_INLINE int32x4_t ZeroInt()
            {
                return vmovq_n_s32(0);
            }
        }
    }
}
