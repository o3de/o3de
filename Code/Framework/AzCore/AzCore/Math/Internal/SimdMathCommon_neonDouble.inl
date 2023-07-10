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
        namespace NeonDouble
        {
            AZ_MATH_INLINE float32x2_t LoadAligned(const float* __restrict addr)
            {
                AZ_MATH_ASSERT(IsAligned<16>(addr), "Alignment failure");
                return vld1_f32(addr);
            }

            AZ_MATH_INLINE int32x2_t LoadAligned(const int32_t* __restrict addr)
            {
                AZ_MATH_ASSERT(IsAligned<16>(addr), "Alignment failure");
                return vld1_s32(addr);
            }

            AZ_MATH_INLINE float32x2_t LoadUnaligned(const float* __restrict addr)
            {
                return vld1_f32(addr);
            }

            AZ_MATH_INLINE int32x2_t LoadUnaligned(const int32_t* __restrict addr)
            {
                return vld1_s32(addr);
            }

            AZ_MATH_INLINE void StoreAligned(float* __restrict addr, float32x2_t value)
            {
                AZ_MATH_ASSERT(IsAligned<16>(addr), "Alignment failure");
                vst1_f32(addr, value);
            }

            AZ_MATH_INLINE void StoreAligned(int32_t* __restrict addr, int32x2_t value)
            {
                AZ_MATH_ASSERT(IsAligned<16>(addr), "Alignment failure");
                vst1_s32(addr, value);
            }

            AZ_MATH_INLINE void StoreUnaligned(float* __restrict addr, float32x2_t value)
            {
                vst1_f32(addr, value);
            }

            AZ_MATH_INLINE void StoreUnaligned(int32_t* __restrict addr, int32x2_t value)
            {
                vst1_s32(addr, value);
            }

            AZ_MATH_INLINE void StreamAligned(float* __restrict addr, float32x2_t value)
            {
                AZ_MATH_ASSERT(IsAligned<16>(addr), "Alignment failure");
                vst1_f32(addr, value);
            }

            AZ_MATH_INLINE void StreamAligned(int32_t* __restrict addr, int32x2_t value)
            {
                AZ_MATH_ASSERT(IsAligned<16>(addr), "Alignment failure");
                vst1_s32(addr, value);
            }

            AZ_MATH_INLINE float SelectIndex0(float32x2_t value)
            {
                return vget_lane_f32(value, 0);
            }

            AZ_MATH_INLINE float SelectIndex1(float32x2_t value)
            {
                return vget_lane_f32(value, 1);
            }

            AZ_MATH_INLINE float32x2_t Splat(float value)
            {
                return vdup_n_f32(value);
            }

            AZ_MATH_INLINE int32x2_t Splat(int32_t value)
            {
                return vdup_n_s32(value);
            }

            AZ_MATH_INLINE float32x2_t SplatIndex0(float32x2_t value)
            {
                return vdup_lane_f32(value, 0);
            }

            AZ_MATH_INLINE float32x2_t SplatIndex1(float32x2_t value)
            {
                return vdup_lane_f32(value, 1);
            }

            AZ_MATH_INLINE float32x2_t ReplaceIndex0(float32x2_t a, float b)
            {
                return vset_lane_f32(b, a, 0);
            }

            AZ_MATH_INLINE float32x2_t ReplaceIndex0(float32x2_t a, float32x2_t b)
            {
                return ReplaceIndex0(a, SelectIndex0(b));
            }

            AZ_MATH_INLINE float32x2_t ReplaceIndex1(float32x2_t a, float b)
            {
                return vset_lane_f32(b, a, 1);
            }

            AZ_MATH_INLINE float32x2_t ReplaceIndex1(float32x2_t a, float32x2_t b)
            {
                return ReplaceIndex1(a, SelectIndex1(b));
            }

            AZ_MATH_INLINE float32x2_t LoadImmediate(float x, float y)
            {
                const float values[2] = { x, y };
                return vld1_f32(values);
            }

            AZ_MATH_INLINE int32x2_t LoadImmediate(int32_t x, int32_t y)
            {
                const int32_t values[2] = { x, y };
                return vld1_s32(values);
            }

            AZ_MATH_INLINE float32x2_t Add(float32x2_t arg1, float32x2_t arg2)
            {
                return vadd_f32(arg1, arg2);
            }

            AZ_MATH_INLINE float32x2_t Sub(float32x2_t arg1, float32x2_t arg2)
            {
                return vsub_f32(arg1, arg2);
            }

            AZ_MATH_INLINE float32x2_t Mul(float32x2_t arg1, float32x2_t arg2)
            {
                return vmul_f32(arg1, arg2);
            }

            AZ_MATH_INLINE float32x2_t Madd(float32x2_t mul1, float32x2_t mul2, float32x2_t add)
            {
                return vmla_f32(add, mul1, mul2);
            }

            AZ_MATH_INLINE float32x2_t Div(float32x2_t arg1, float32x2_t arg2)
            {
                return vdiv_f32(arg1, arg2);
            }

            AZ_MATH_INLINE float32x2_t Abs(float32x2_t value)
            {
                return vabs_f32(value);
            }

            AZ_MATH_INLINE int32x2_t Add(int32x2_t arg1, int32x2_t arg2)
            {
                return vadd_s32(arg1, arg2);
            }

            AZ_MATH_INLINE int32x2_t Sub(int32x2_t arg1, int32x2_t arg2)
            {
                return vsub_s32(arg1, arg2);
            }

            AZ_MATH_INLINE int32x2_t Mul(int32x2_t arg1, int32x2_t arg2)
            {
                return vmul_s32(arg1, arg2);
            }

            AZ_MATH_INLINE int32x2_t Madd(int32x2_t mul1, int32x2_t mul2, int32x2_t add)
            {
                return vmla_s32(add, mul1, mul2);
            }

            AZ_MATH_INLINE int32x2_t Abs(int32x2_t value)
            {
                return vabs_s32(value);
            }

            AZ_MATH_INLINE float32x2_t Not(float32x2_t value)
            {
                return vreinterpret_f32_s32(vmvn_s32(vreinterpret_s32_f32(value)));
            }

            AZ_MATH_INLINE float32x2_t And(float32x2_t arg1, float32x2_t arg2)
            {
                return vreinterpret_f32_s32(vand_s32(vreinterpret_s32_f32(arg1), vreinterpret_s32_f32(arg2)));
            }

            AZ_MATH_INLINE float32x2_t AndNot(float32x2_t arg1, float32x2_t arg2)
            {
                return vreinterpret_f32_s32(vand_s32(vmvn_s32(vreinterpret_s32_f32(arg1)), vreinterpret_s32_f32(arg2)));
            }

            AZ_MATH_INLINE float32x2_t Or(float32x2_t arg1, float32x2_t arg2)
            {
                return vreinterpret_f32_s32(vorr_s32(vreinterpret_s32_f32(arg1), vreinterpret_s32_f32(arg2)));
            }

            AZ_MATH_INLINE float32x2_t Xor(float32x2_t arg1, float32x2_t arg2)
            {
                return vreinterpret_f32_s32(veor_s32(vreinterpret_s32_f32(arg1), vreinterpret_s32_f32(arg2)));
            }

            AZ_MATH_INLINE int32x2_t Not(int32x2_t value)
            {
                return vmvn_s32(value);
            }

            AZ_MATH_INLINE int32x2_t And(int32x2_t arg1, int32x2_t arg2)
            {
                return vand_s32(arg1, arg2);
            }

            AZ_MATH_INLINE int32x2_t AndNot(int32x2_t arg1, int32x2_t arg2)
            {
                return vand_s32(vmvn_s32(arg1), arg2);
            }

            AZ_MATH_INLINE int32x2_t Or(int32x2_t arg1, int32x2_t arg2)
            {
                return vorr_s32(arg1, arg2);
            }

            AZ_MATH_INLINE int32x2_t Xor(int32x2_t arg1, int32x2_t arg2)
            {
                return veor_s32(arg1, arg2);
            }

            AZ_MATH_INLINE float32x2_t Floor(float32x2_t value)
            {
                return vrndm_f32(value);
            }

            AZ_MATH_INLINE float32x2_t Ceil(float32x2_t value)
            {
                return vrndp_f32(value);
            }

            AZ_MATH_INLINE float32x2_t Round(float32x2_t value)
            {
                return vrndn_f32(value);
            }

            AZ_MATH_INLINE float32x2_t Truncate(float32x2_t value)
            {
                return vcvt_f32_s32(vcvt_s32_f32(value));
            }

            AZ_MATH_INLINE float32x2_t Min(float32x2_t arg1, float32x2_t arg2)
            {
                return vmin_f32(arg1, arg2);
            }

            AZ_MATH_INLINE float32x2_t Max(float32x2_t arg1, float32x2_t arg2)
            {
                return vmax_f32(arg1, arg2);
            }

            AZ_MATH_INLINE float32x2_t Clamp(float32x2_t value, float32x2_t min, float32x2_t max)
            {
                return vmax_f32(min, vmin_f32(value, max));
            }

            AZ_MATH_INLINE int32x2_t Min(int32x2_t arg1, int32x2_t arg2)
            {
                return vmin_s32(arg1, arg2);
            }

            AZ_MATH_INLINE int32x2_t Max(int32x2_t arg1, int32x2_t arg2)
            {
                return vmax_s32(arg1, arg2);
            }

            AZ_MATH_INLINE int32x2_t Clamp(int32x2_t value, int32x2_t min, int32x2_t max)
            {
                return vmax_s32(min, vmin_s32(value, max));
            }

            AZ_MATH_INLINE float32x2_t CmpEq(float32x2_t arg1, float32x2_t arg2)
            {
                return vreinterpret_f32_s32(vceq_f32(arg1, arg2));
            }

            AZ_MATH_INLINE float32x2_t CmpNeq(float32x2_t arg1, float32x2_t arg2)
            {
                return vreinterpret_f32_s32(vmvn_s32(vceq_f32(arg1, arg2)));
            }

            AZ_MATH_INLINE float32x2_t CmpGt(float32x2_t arg1, float32x2_t arg2)
            {
                return vreinterpret_f32_s32(vcgt_f32(arg1, arg2));
            }

            AZ_MATH_INLINE float32x2_t CmpGtEq(float32x2_t arg1, float32x2_t arg2)
            {
                return vreinterpret_f32_s32(vcge_f32(arg1, arg2));
            }

            AZ_MATH_INLINE float32x2_t CmpLt(float32x2_t arg1, float32x2_t arg2)
            {
                return vreinterpret_f32_s32(vclt_f32(arg1, arg2));
            }

            AZ_MATH_INLINE float32x2_t CmpLtEq(float32x2_t arg1, float32x2_t arg2)
            {
                return vreinterpret_f32_s32(vcle_f32(arg1, arg2));
            }

            AZ_MATH_INLINE bool CmpAllEq(float32x2_t arg1, float32x2_t arg2)
            {
                return Neon::AreAllLanesTrue(vceq_f32(arg1, arg2));
            }

            AZ_MATH_INLINE bool CmpAllLt(float32x2_t arg1, float32x2_t arg2)
            {
                return Neon::AreAllLanesTrue(vclt_f32(arg1, arg2));
            }

            AZ_MATH_INLINE bool CmpAllLtEq(float32x2_t arg1, float32x2_t arg2)
            {
                return Neon::AreAllLanesTrue(vcle_f32(arg1, arg2));
            }

            AZ_MATH_INLINE bool CmpAllGt(float32x2_t arg1, float32x2_t arg2)
            {
                return Neon::AreAllLanesTrue(vcgt_f32(arg1, arg2));
            }

            AZ_MATH_INLINE bool CmpAllGtEq(float32x2_t arg1, float32x2_t arg2)
            {
                return Neon::AreAllLanesTrue(vcge_f32(arg1, arg2));
            }

            AZ_MATH_INLINE int32x2_t CmpEq(int32x2_t arg1, int32x2_t arg2)
            {
                return vceq_s32(arg1, arg2);
            }

            AZ_MATH_INLINE int32x2_t CmpNeq(int32x2_t arg1, int32x2_t arg2)
            {
                return vmvn_s32(vceq_s32(arg1, arg2));
            }

            AZ_MATH_INLINE int32x2_t CmpGt(int32x2_t arg1, int32x2_t arg2)
            {
                return vcgt_s32(arg1, arg2);
            }

            AZ_MATH_INLINE int32x2_t CmpGtEq(int32x2_t arg1, int32x2_t arg2)
            {
                return vcge_s32(arg1, arg2);
            }

            AZ_MATH_INLINE int32x2_t CmpLt(int32x2_t arg1, int32x2_t arg2)
            {
                return vclt_s32(arg1, arg2);
            }

            AZ_MATH_INLINE int32x2_t CmpLtEq(int32x2_t arg1, int32x2_t arg2)
            {
                return vcle_s32(arg1, arg2);
            }

            AZ_MATH_INLINE bool CmpAllEq(int32x2_t arg1, int32x2_t arg2)
            {
                return Neon::AreAllLanesTrue(vceq_s32(arg1, arg2));
            }

            AZ_MATH_INLINE float32x2_t Select(float32x2_t arg1, float32x2_t arg2, float32x2_t mask)
            {
                return vbsl_f32(mask, arg1, arg2);
            }

            AZ_MATH_INLINE int32x2_t Select(int32x2_t arg1, int32x2_t arg2, int32x2_t mask)
            {
                return vbsl_s32(mask, arg1, arg2);
            }

            AZ_MATH_INLINE float32x2_t Reciprocal(float32x2_t value)
            {
                const float32x2_t estimate = vrecpe_f32(value);
                // Do one Newton Raphson iteration to improve precision
                return vmul_f32(vrecps_f32(value, estimate), estimate);
            }

            AZ_MATH_INLINE float32x2_t ReciprocalEstimate(float32x2_t value)
            {
                return vrecpe_f32(value);
            }

            AZ_MATH_INLINE float32x2_t Mod(float32x2_t value, float32x2_t divisor)
            {
                return Sub(value, Mul(Truncate(Div(value, divisor)), divisor));
            }

            AZ_MATH_INLINE float32x2_t Sqrt(float32x2_t value)
            {
                return vsqrt_f32(value);
            }

            AZ_MATH_INLINE float32x2_t SqrtEstimate(float32x2_t value)
            {
                return vrecpe_f32(vrsqrte_f32(value));
            }

            AZ_MATH_INLINE float32x2_t SqrtInv(float32x2_t value)
            {
                const float32x2_t estimate = vrsqrte_f32(value);
                // Do one Newton Raphson iteration to improve precision
                return vmul_f32(vrsqrts_f32(vmul_f32(value, estimate), estimate), estimate);
            }

            AZ_MATH_INLINE float32x2_t SqrtInvEstimate(float32x2_t value)
            {
                return vrsqrte_f32(value); // Faster, but roughly half the precision (12ish bits rather than 23ish)
            }

            AZ_MATH_INLINE float32x2_t ConvertToFloat(int32x2_t value)
            {
                return vcvt_f32_s32(value);
            }

            AZ_MATH_INLINE int32x2_t ConvertToInt(float32x2_t value)
            {
                return vcvt_s32_f32(value);
            }

            AZ_MATH_INLINE int32x2_t ConvertToIntNearest(float32x2_t value)
            {
                return vcvtn_s32_f32(value);
            }

            AZ_MATH_INLINE float32x2_t CastToFloat(int32x2_t value)
            {
                return vreinterpret_f32_s32(value);
            }

            AZ_MATH_INLINE int32x2_t CastToInt(float32x2_t value)
            {
                return vreinterpret_s32_f32(value);
            }

            AZ_MATH_INLINE float32x2_t ZeroFloat()
            {
                return vmov_n_f32(0.0f);
            }

            AZ_MATH_INLINE int32x2_t ZeroInt()
            {
                return vmov_n_s32(0);
            }
        }
    }
}
