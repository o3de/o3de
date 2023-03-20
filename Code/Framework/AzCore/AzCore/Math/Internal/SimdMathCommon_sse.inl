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
        namespace Sse
        {
            AZ_MATH_INLINE __m128 LoadAligned(const float* __restrict addr)
            {
                AZ_MATH_ASSERT(IsAligned<16>(addr), "Alignment failure");
                return _mm_load_ps(addr);
            }


            AZ_MATH_INLINE __m128i LoadAligned(const int32_t* __restrict addr)
            {
                AZ_MATH_ASSERT(IsAligned<16>(addr), "Alignment failure");
                return _mm_load_si128((const __m128i *)addr);
            }


            AZ_MATH_INLINE __m128 LoadUnaligned(const float* __restrict addr)
            {
                return _mm_loadu_ps(addr);
            }


            AZ_MATH_INLINE __m128i LoadUnaligned(const int32_t* __restrict addr)
            {
                return _mm_loadu_si128((const __m128i *)addr);
            }


            AZ_MATH_INLINE void StoreAligned(float* __restrict addr, __m128 value)
            {
                AZ_MATH_ASSERT(IsAligned<16>(addr), "Alignment failure");
                _mm_store_ps(addr, value);
            }


            AZ_MATH_INLINE void StoreAligned(int32_t* __restrict addr, __m128i value)
            {
                AZ_MATH_ASSERT(IsAligned<16>(addr), "Alignment failure");
                _mm_store_si128((__m128i *)addr, value);
            }


            AZ_MATH_INLINE void StoreUnaligned(float* __restrict addr, __m128 value)
            {
                _mm_storeu_ps(addr, value);
            }


            AZ_MATH_INLINE void StoreUnaligned(int32_t* __restrict addr, __m128i value)
            {
                _mm_storeu_si128((__m128i *)addr, value);
            }


            AZ_MATH_INLINE void StreamAligned(float* __restrict addr, __m128 value)
            {
                AZ_MATH_ASSERT(IsAligned<16>(addr), "Alignment failure");
                _mm_stream_ps(addr, value);
            }


            AZ_MATH_INLINE void StreamAligned(int32_t* __restrict addr, __m128i value)
            {
                AZ_MATH_ASSERT(IsAligned<16>(addr), "Alignment failure");
                _mm_stream_si128((__m128i *)addr, value);
            }


            AZ_MATH_INLINE __m128 ConvertToFloat(__m128i value)
            {
                return _mm_cvtepi32_ps(value);
            }


            AZ_MATH_INLINE __m128i ConvertToInt(__m128 value)
            {
                return _mm_cvttps_epi32(value);
            }


            AZ_MATH_INLINE __m128i ConvertToIntNearest(__m128 value)
            {
                return _mm_cvtps_epi32(value);
            }


            AZ_MATH_INLINE __m128 CastToFloat(__m128i value)
            {
                return _mm_castsi128_ps(value);
            }


            AZ_MATH_INLINE __m128i CastToInt(__m128 value)
            {
                return _mm_castps_si128(value);
            }


            AZ_MATH_INLINE __m128i ZeroInt()
            {
                return _mm_setzero_si128();
            }


            AZ_MATH_INLINE __m128 ZeroFloat()
            {
                return CastToFloat(ZeroInt());
            }


            AZ_MATH_INLINE float SelectFirst(__m128 value)
            {
                return _mm_cvtss_f32(value);
            }


            AZ_MATH_INLINE __m128 Splat(float value)
            {
                return _mm_set1_ps(value);
            }


            AZ_MATH_INLINE __m128i Splat(int32_t value)
            {
                return _mm_set1_epi32(value);
            }


            AZ_MATH_INLINE __m128 SplatFirst(__m128 value)
            {
                return _mm_shuffle_ps(value, value, _MM_SHUFFLE(0, 0, 0, 0));
            }


            AZ_MATH_INLINE __m128 SplatSecond(__m128 value)
            {
                return _mm_shuffle_ps(value, value, _MM_SHUFFLE(1, 1, 1, 1));
            }


            AZ_MATH_INLINE __m128 SplatThird(__m128 value)
            {
                return _mm_shuffle_ps(value, value, _MM_SHUFFLE(2, 2, 2, 2));
            }


            AZ_MATH_INLINE __m128 SplatFourth(__m128 value)
            {
                return _mm_shuffle_ps(value, value, _MM_SHUFFLE(3, 3, 3, 3));
            }


            AZ_MATH_INLINE __m128 ReplaceFirst(__m128 a, __m128 b)
            {
                return _mm_blend_ps(a, b, 0b0001);
            }


            AZ_MATH_INLINE __m128 ReplaceFirst(__m128 a, float b)
            {
                return ReplaceFirst(a, Splat(b));
            }


            AZ_MATH_INLINE __m128 ReplaceSecond(__m128 a, __m128 b)
            {
                return _mm_blend_ps(a, b, 0b0010);
            }


            AZ_MATH_INLINE __m128 ReplaceSecond(__m128 a, float b)
            {
                return ReplaceSecond(a, Splat(b));
            }


            AZ_MATH_INLINE __m128 ReplaceThird(__m128 a, __m128 b)
            {
                return _mm_blend_ps(a, b, 0b0100);
            }


            AZ_MATH_INLINE __m128 ReplaceThird(__m128 a, float b)
            {
                return ReplaceThird(a, Splat(b));
            }


            AZ_MATH_INLINE __m128 ReplaceFourth(__m128 a, __m128 b)
            {
                return _mm_blend_ps(a, b, 0b1000);
            }


            AZ_MATH_INLINE __m128 ReplaceFourth(__m128 a, float b)
            {
                return ReplaceFourth(a, Splat(b));
            }


            AZ_MATH_INLINE __m128 LoadImmediate(float x, float y, float z, float w)
            {
                return _mm_set_ps(w, z, y, x);
            }


            AZ_MATH_INLINE __m128i LoadImmediate(int32_t x, int32_t y, int32_t z, int32_t w)
            {
                return _mm_set_epi32(w, z, y, x);
            }


            AZ_MATH_INLINE __m128 Not(__m128 value)
            {
                const __m128i invert = Splat(static_cast<int32_t>(0xFFFFFFFF));
                return _mm_andnot_ps(value, CastToFloat(invert));
            }


            AZ_MATH_INLINE __m128 And(__m128 arg1, __m128 arg2)
            {
                return _mm_and_ps(arg1, arg2);
            }


            AZ_MATH_INLINE __m128 AndNot(__m128 arg1, __m128 arg2)
            {
                return _mm_andnot_ps(arg1, arg2);
            }


            AZ_MATH_INLINE __m128 Or(__m128 arg1, __m128 arg2)
            {
                return _mm_or_ps(arg1, arg2);
            }


            AZ_MATH_INLINE __m128 Xor(__m128 arg1, __m128 arg2)
            {
                return _mm_xor_ps(arg1, arg2);
            }


            AZ_MATH_INLINE __m128i Not(__m128i value)
            {
                return CastToInt(Not(CastToFloat(value)));
            }


            AZ_MATH_INLINE __m128i And(__m128i arg1, __m128i arg2)
            {
                return _mm_and_si128(arg1, arg2);
            }


            AZ_MATH_INLINE __m128i AndNot(__m128i arg1, __m128i arg2)
            {
                return _mm_andnot_si128(arg1, arg2);
            }


            AZ_MATH_INLINE __m128i Or(__m128i arg1, __m128i arg2)
            {
                return _mm_or_si128(arg1, arg2);
            }


            AZ_MATH_INLINE __m128i Xor(__m128i arg1, __m128i arg2)
            {
                return _mm_xor_si128(arg1, arg2);
            }


            AZ_MATH_INLINE __m128 Floor(__m128 value)
            {
                return _mm_floor_ps(value);
            }


            AZ_MATH_INLINE __m128 Ceil(__m128 value)
            {
                return _mm_ceil_ps(value);
            }


            AZ_MATH_INLINE __m128 Round(__m128 value)
            {
                return _mm_round_ps(value, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
            }


            AZ_MATH_INLINE __m128 Truncate(__m128 value)
            {
                return _mm_round_ps(value, _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC);
            }


            AZ_MATH_INLINE __m128 Min(__m128 arg1, __m128 arg2)
            {
                return _mm_min_ps(arg1, arg2);
            }


            AZ_MATH_INLINE __m128 Max(__m128 arg1, __m128 arg2)
            {
                return _mm_max_ps(arg1, arg2);
            }


            AZ_MATH_INLINE __m128 Clamp(__m128 value, __m128 min, __m128 max)
            {
                return Max(min, Min(value, max));
            }


            AZ_MATH_INLINE __m128i Min(__m128i arg1, __m128i arg2)
            {
                return _mm_min_epi32(arg1, arg2);
            }


            AZ_MATH_INLINE __m128i Max(__m128i arg1, __m128i arg2)
            {
                return _mm_max_epi32(arg1, arg2);
            }


            AZ_MATH_INLINE __m128i Clamp(__m128i value, __m128i min, __m128i max)
            {
                return Max(min, Min(value, max));
            }


            AZ_MATH_INLINE __m128 CmpEq(__m128 arg1, __m128 arg2)
            {
                return _mm_cmpeq_ps(arg1, arg2);
            }


            AZ_MATH_INLINE __m128 CmpNeq(__m128 arg1, __m128 arg2)
            {
                return _mm_cmpneq_ps(arg1, arg2);
            }


            AZ_MATH_INLINE __m128 CmpGt(__m128 arg1, __m128 arg2)
            {
                return _mm_cmpgt_ps(arg1, arg2);
            }


            AZ_MATH_INLINE __m128 CmpGtEq(__m128 arg1, __m128 arg2)
            {
                return _mm_cmpge_ps(arg1, arg2);
            }


            AZ_MATH_INLINE __m128 CmpLt(__m128 arg1, __m128 arg2)
            {
                return _mm_cmplt_ps(arg1, arg2);
            }


            AZ_MATH_INLINE __m128 CmpLtEq(__m128 arg1, __m128 arg2)
            {
                return _mm_cmple_ps(arg1, arg2);
            }


            AZ_MATH_INLINE bool CmpAllEq(__m128 arg1, __m128 arg2, int32_t mask)
            {
                const __m128 compare = CmpEq(arg1, arg2);
                return (_mm_movemask_ps(compare) & mask) == mask;
            }


            AZ_MATH_INLINE bool CmpAllLt(__m128 arg1, __m128 arg2, int32_t mask)
            {
                const __m128 compare = CmpLt(arg1, arg2);
                return (_mm_movemask_ps(compare) & mask) == mask;
            }


            AZ_MATH_INLINE bool CmpAllLtEq(__m128 arg1, __m128 arg2, int32_t mask)
            {
                const __m128 compare = CmpLtEq(arg1, arg2);
                return (_mm_movemask_ps(compare) & mask) == mask;
            }


            AZ_MATH_INLINE bool CmpAllGt(__m128 arg1, __m128 arg2, int32_t mask)
            {
                const __m128 compare = CmpGt(arg1, arg2);
                return (_mm_movemask_ps(compare) & mask) == mask;
            }


            AZ_MATH_INLINE bool CmpAllGtEq(__m128 arg1, __m128 arg2, int32_t mask)
            {
                const __m128 compare = CmpGtEq(arg1, arg2);
                return (_mm_movemask_ps(compare) & mask) == mask;
            }


            AZ_MATH_INLINE __m128i CmpEq(__m128i arg1, __m128i arg2)
            {
                return _mm_cmpeq_epi32(arg1, arg2);
            }


            AZ_MATH_INLINE __m128i CmpNeq(__m128i arg1, __m128i arg2)
            {
                const __m128i equal = CmpEq(arg1, arg2);
                return Not(equal);
            }


            AZ_MATH_INLINE __m128i CmpGt(__m128i arg1, __m128i arg2)
            {
                return _mm_cmpgt_epi32(arg1, arg2);
            }


            AZ_MATH_INLINE __m128i CmpGtEq(__m128i arg1, __m128i arg2)
            {
                const __m128i lessThan = _mm_cmplt_epi32(arg1, arg2);
                return Not(lessThan);
            }


            AZ_MATH_INLINE __m128i CmpLt(__m128i arg1, __m128i arg2)
            {
                return _mm_cmplt_epi32(arg1, arg2);
            }


            AZ_MATH_INLINE __m128i CmpLtEq(__m128i arg1, __m128i arg2)
            {
                const __m128i greaterThan = CmpGt(arg1, arg2);
                return Not(greaterThan);
            }


            AZ_MATH_INLINE bool CmpAllEq(__m128i arg1, __m128i arg2, int32_t mask)
            {
                const __m128i compare = CmpNeq(arg1, arg2);
                return (_mm_movemask_epi8(compare) & mask) == 0;
            }


            AZ_MATH_INLINE __m128 Select(__m128 arg1, __m128 arg2, __m128 mask)
            {
                return _mm_blendv_ps(arg2, arg1, mask);
            }


            AZ_MATH_INLINE __m128i Select(__m128i arg1, __m128i arg2, __m128i mask)
            {
                return _mm_blendv_epi8(arg2, arg1, mask);
            }


            AZ_MATH_INLINE bool CmpAllLt(__m128i arg1, __m128i arg2, int32_t mask)
            {
                const __m128i compare = CmpGtEq(arg1, arg2);
                return (_mm_movemask_epi8(compare) & mask) == 0;
            }


            AZ_MATH_INLINE bool CmpAllLtEq(__m128i arg1, __m128i arg2, int32_t mask)
            {
                const __m128i compare = CmpGt(arg1, arg2);
                return (_mm_movemask_epi8(compare) & mask) == 0;
            }


            AZ_MATH_INLINE bool CmpAllGt(__m128i arg1, __m128i arg2, int32_t mask)
            {
                const __m128i compare = CmpLtEq(arg1, arg2);
                return (_mm_movemask_epi8(compare) & mask) == 0;
            }


            AZ_MATH_INLINE bool CmpAllGtEq(__m128i arg1, __m128i arg2, int32_t mask)
            {
                const __m128i compare = CmpLt(arg1, arg2);
                return (_mm_movemask_epi8(compare) & mask) == 0;
            }


            AZ_MATH_INLINE __m128 Add(__m128 arg1, __m128 arg2)
            {
                return _mm_add_ps(arg1, arg2);
            }


            AZ_MATH_INLINE __m128 Sub(__m128 arg1, __m128 arg2)
            {
                return _mm_sub_ps(arg1, arg2);
            }


            AZ_MATH_INLINE __m128 Mul(__m128 arg1, __m128 arg2)
            {
                return _mm_mul_ps(arg1, arg2);
            }


            AZ_MATH_INLINE __m128 Madd(__m128 mul1, __m128 mul2, __m128 add)
            {
#if 0
                return _mm_fmadd_ps(mul1, mul2, add); // Requires FMA CPUID
#else
                return Add(Mul(mul1, mul2), add);
#endif
            }


            AZ_MATH_INLINE __m128 Div(__m128 arg1, __m128 arg2)
            {
                return _mm_div_ps(arg1, arg2);
            }


            AZ_MATH_INLINE __m128 Abs(__m128 value)
            {
                const __m128 signMask = CastToFloat(Splat(0x7FFFFFFF));
                return And(value, signMask);
            }


            AZ_MATH_INLINE __m128i Add(__m128i arg1, __m128i arg2)
            {
                return _mm_add_epi32(arg1, arg2);
            }


            AZ_MATH_INLINE __m128i Sub(__m128i arg1, __m128i arg2)
            {
                return _mm_sub_epi32(arg1, arg2);
            }


            AZ_MATH_INLINE __m128i Mul(__m128i arg1, __m128i arg2)
            {
                return _mm_mullo_epi32(arg1, arg2);
            }


            AZ_MATH_INLINE __m128i Madd(__m128i mul1, __m128i mul2, __m128i add)
            {
                return Add(Mul(mul1, mul2), add);
            }


            AZ_MATH_INLINE __m128i Abs(__m128i value)
            {
                return _mm_abs_epi32(value);
            }


            AZ_MATH_INLINE __m128 ReciprocalEstimate(__m128 value)
            {
                return _mm_rcp_ps(value);
            }


            AZ_MATH_INLINE __m128 Reciprocal(__m128 value)
            {
#if 0
                const __m128 estimate = ReciprocalEstimate(value);
                const __m128 estimateSquare = Mul(estimate, estimate);
                const __m128 estimateDouble = Add(estimate, estimate);
                return Sub(estimateDouble, Mul(value, estimateSquare));
#else
                const __m128 One = Splat(1.0f);
                return Div(One, value);
#endif
            }


            AZ_MATH_INLINE __m128 Sqrt(__m128 value)
            {
                return _mm_sqrt_ps(value);
            }


            AZ_MATH_INLINE __m128 SqrtInvEstimate(__m128 value)
            {
                return _mm_rsqrt_ps(value); // Faster, but roughly half the precision (12ish bits rather than 23ish)
            }


            AZ_MATH_INLINE __m128 SqrtEstimate(__m128 value)
            {
                return ReciprocalEstimate(SqrtInvEstimate(value));
            }


            AZ_MATH_INLINE __m128 SqrtInv(__m128 value)
            {
#if 0
                // Do one Newton Raphson iteration to improve precision (estimate only gives about 11 to 12 bits of accuracy)
                // Every iteration should roughly double precision, so this should give us more than 20 bits of accuracy
                const __m128 Three = Splat(3.0f);
                const __m128 Half  = Splat(0.5f);

                const __m128 estimate   = SqrtInvEstimate(value);
                const __m128 estSqValue = Mul(value, Mul(estimate, estimate));
                return Mul(Mul(Half, estimate), Sub(Three, estSqValue));
#else
                const __m128 One = Splat(1.0f);
                return Div(One, Sqrt(value));
#endif
            }


            AZ_MATH_INLINE __m128 Mod(__m128 value, __m128 divisor)
            {
                return Sub(value, Mul(_mm_round_ps(Div(value, divisor), _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC), divisor));
            }
        }
    }
}
