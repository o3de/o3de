/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifdef _MSC_VER
#pragma warning(push)
#endif

namespace SimuCore {
    namespace detail {
        template <typename T>
        struct Calculator;

#if ((SIMUCORE_PLATFORM_INFO & SIMUCORE_SIMD_SSE41) != 0)
        template <>
        struct Calculator<__m128> {
            static inline __m128 Add(const __m128& lhs, const __m128& rhs)
            {
                return _mm_add_ps(lhs, rhs);
            }

            static inline __m128 Sub(const __m128& lhs, const __m128& rhs)
            {
                return _mm_sub_ps(lhs, rhs);
            }

            static inline __m128 Mul(const __m128& lhs, const __m128& rhs)
            {
                return _mm_mul_ps(lhs, rhs);
            }

            static inline __m128 Div(const __m128& lhs, const __m128& rhs)
            {
                return _mm_div_ps(lhs, rhs);
            }

            static inline float Dot(const __m128& lhs, const __m128& rhs)
            {
                return _mm_cvtss_f32(_mm_dp_ps(lhs, rhs, 0xff));
            }

            static inline float Length(const __m128& v)
            {
                return _mm_cvtss_f32(_mm_sqrt_ps(_mm_dp_ps(v, v, 0xff)));
            }

            static inline float Distance(const __m128& lhs, const __m128& rhs)
            {
                auto dir = _mm_sub_ps(lhs, rhs);
                return _mm_cvtss_f32(_mm_sqrt_ps(_mm_dp_ps(dir, dir, 0xff)));
            }

            static inline __m128 Cross(const __m128& lhs, const __m128& rhs)
            {
                //    v0   v1   v2   v3
                // x: l1 * r2 - l2 * r1
                // y: l2 * r0 - l0 * r2
                // z: l0 * r1 - l1 * r0
                // w: l3   r3   l3   r3
                auto v0 = _mm_shuffle_ps(lhs, lhs, _MM_SHUFFLE(3, 0, 2, 1));
                auto v1 = _mm_shuffle_ps(rhs, rhs, _MM_SHUFFLE(3, 1, 0, 2));
                auto v2 = _mm_shuffle_ps(lhs, lhs, _MM_SHUFFLE(3, 1, 0, 2));
                auto v3 = _mm_shuffle_ps(rhs, rhs, _MM_SHUFFLE(3, 0, 2, 1));
                return _mm_sub_ps(_mm_mul_ps(v0, v1), _mm_mul_ps(v2, v3));
            }

            static inline __m128 Madd(const __m128& mul1, const __m128& mul2, const __m128& add)
            {
                return Add(Mul(mul1, mul2), add);
            }

            static inline __m128 ComponentMin(const __m128& lhs, const __m128& rhs)
            {
                return _mm_min_ps(lhs, rhs);
            }

            static inline __m128 ComponentMax(const __m128& lhs, const __m128& rhs)
            {
                return _mm_max_ps(lhs, rhs);
            }

            static inline __m128 Lerp(const __m128& src, const __m128& dest, const __m128& alpha)
            {
                return Madd(Sub(dest, src), alpha, src);
            }

            static inline __m128i CastToInt(__m128 value)
            {
                return _mm_castps_si128(value);
            }

            static inline __m128 CmpGt(const __m128 arg1, const __m128 arg2)
            {
                return _mm_cmpgt_ps(arg1, arg2);
            }

            static inline __m128 CmpGtEq(const __m128& arg1, const __m128& arg2)
            {
                return _mm_cmpge_ps(arg1, arg2);
            }

            static inline __m128 CmpLt(const __m128& arg1, const __m128& arg2)
            {
                return _mm_cmplt_ps(arg1, arg2);
            }

            static inline __m128 CmpLtEq(const __m128& arg1, const __m128& arg2)
            {
                return _mm_cmple_ps(arg1, arg2);
            }

            static inline __m128 CmpEq(const __m128& arg1, const __m128& arg2)
            {
                return _mm_cmpeq_ps(arg1, arg2);
            }

            static inline bool CmpAllLt(const __m128 arg1, const __m128 arg2)
            {
                const __m128i compare = CastToInt(CmpLt(arg1, arg2));
                return (_mm_movemask_epi8(compare) & 0xFFFF) != 0;
            }

            static inline bool CmpAllLtEq(const __m128& arg1, const __m128& arg2)
            {
                const __m128i compare = CastToInt(CmpLtEq(arg1, arg2));
                return (_mm_movemask_epi8(compare) & 0xFFFF) != 0;
            }

            static inline bool CmpAllGt(const __m128& arg1, const __m128& arg2)
            {
                const __m128i compare = CastToInt(CmpGt(arg1, arg2));
                return (_mm_movemask_epi8(compare) & 0xFFFF) != 0;
            }

            static inline bool CmpAllGtEq(const __m128& arg1, const __m128& arg2)
            {
                const __m128i compare = CastToInt(CmpGtEq(arg1, arg2));
                return (_mm_movemask_epi8(compare) & 0xFFFF) != 0;
            }

            static inline bool CmpAllEq(const __m128& arg1, const __m128& arg2)
            {
                const __m128i compare = CastToInt(CmpEq(arg1, arg2));
                return (_mm_movemask_epi8(compare) & 0xFFFF) == 0xFFFF;
            }

            static inline __m128 Normalize(const __m128& lhs)
            {
                auto dot = _mm_dp_ps(lhs, lhs, 0xff);
                auto sqrt = _mm_sqrt_ps(dot);
                return _mm_div_ps(lhs, sqrt);
            }

            static inline void SetValue(__m128& value, float a)
            {
                value = _mm_set_ps1(a);
            }

            static inline void SetValue(__m128& value, float a, float b, float c, float d)
            {
                value = _mm_set_ps(d, c, b, a);
            }

            static inline __m128i LoadAligned(const int32_t* __restrict addr)
            {
                return _mm_load_si128(reinterpret_cast<const __m128i *>(addr));
            }

            static inline __m128 CastToFloat(__m128i value)
            {
                return _mm_castsi128_ps(value);
            }

            static inline __m128 Xor(const __m128& lhs, const __m128& rhs)
            {
                return _mm_xor_ps(lhs, rhs);
            }

            static inline __m128 GetConjugate(const  __m128& lhs)
            {
                const __m128i mask = LoadAligned(reinterpret_cast<const int32_t*>(&Math::NEGATE_XYZ_MASK));
                return Xor(lhs, CastToFloat(mask));
            }
        };
#else
        /*********************************Vector2*************************************/
        // vector2 has 2 member.
        template<>
        struct Calculator<VecValue<2>> {
            static inline void SetValue(VecValue<2>& value, float a, float b, float, float)
            {
                value.value[VEC_X] = a;
                value.value[VEC_Y] = b;
            }

            static inline float Length(const VecValue<2>& v)
            {
                return static_cast<float>(sqrt(v.value[VEC_X] * v.value[VEC_X] + v.value[VEC_Y] * v.value[VEC_Y]));
            }

            static inline VecValue<2> ComponentMin(const VecValue<2>& lhs, const VecValue<2>& rhs)
            {
                VecValue<2> result = {};
                result.value[VEC_X] = fmin(lhs.value[VEC_X], rhs.value[VEC_X]);
                result.value[VEC_Y] = fmin(lhs.value[VEC_Y], rhs.value[VEC_Y]);
                return result;
            }

            static inline VecValue<2> ComponentMax(const VecValue<2>& lhs, const VecValue<2>& rhs)
            {
                VecValue<2> result = {};
                result.value[VEC_X] = fmax(lhs.value[VEC_X], rhs.value[VEC_X]);
                result.value[VEC_Y] = fmax(lhs.value[VEC_Y], rhs.value[VEC_Y]);
                return result;
            }

            static inline bool CmpAllEq(const VecValue<2>& lhs, const VecValue<2>& rhs)
            {
                return (Math::Equal(lhs.value[VEC_X], rhs.value[VEC_X]) &&
                    Math::Equal(lhs.value[VEC_Y], rhs.value[VEC_Y]));
            }

            static inline bool CmpAllLt(const VecValue<2>& lhs, const VecValue<2>& rhs)
            {
                return (lhs.value[VEC_X] < rhs.value[VEC_X]) || (lhs.value[VEC_Y] < rhs.value[VEC_Y]);
            }

            static inline bool CmpAllLtEq(const VecValue<2>& lhs, const VecValue<2>& rhs)
            {
                return (lhs.value[VEC_X] <= rhs.value[VEC_X]) || (lhs.value[VEC_Y] <= rhs.value[VEC_Y]);
            }

            static inline bool CmpAllGt(const VecValue<2>& lhs, const VecValue<2>& rhs)
            {
                return (lhs.value[VEC_X] > rhs.value[VEC_X]) || (lhs.value[VEC_Y] > rhs.value[VEC_Y]);
            }

            static inline bool CmpAllGtEq(const VecValue<2>& lhs, const VecValue<2>& rhs)
            {
                return (lhs.value[VEC_X] >= rhs.value[VEC_X]) || (lhs.value[VEC_Y] >= rhs.value[VEC_Y]);
            }
        };

        /*********************************Vector3*************************************/
        // vector3 has 3 member.
        template <>
        struct Calculator<VecValue<3>> {
            static inline VecValue<3> Add(const VecValue<3>& lhs, const VecValue<3>& rhs)
            {
                VecValue<3> result = {};
                result.value[VEC_X] = lhs.value[VEC_X] + rhs.value[VEC_X];
                result.value[VEC_Y] = lhs.value[VEC_Y] + rhs.value[VEC_Y];
                result.value[VEC_Z] = lhs.value[VEC_Z] + rhs.value[VEC_Z];
                return result;
            }

            static inline VecValue<3> Sub(const VecValue<3>& lhs, const VecValue<3>& rhs)
            {
                VecValue<3> result = {};
                result.value[VEC_X] = lhs.value[VEC_X] - rhs.value[VEC_X];
                result.value[VEC_Y] = lhs.value[VEC_Y] - rhs.value[VEC_Y];
                result.value[VEC_Z] = lhs.value[VEC_Z] - rhs.value[VEC_Z];
                return result;
            }

            static inline VecValue<3> Mul(const VecValue<3>& lhs, float value)
            {
                VecValue<3> result = {};
                result.value[VEC_X] = lhs.value[VEC_X] / value;
                result.value[VEC_Y] = lhs.value[VEC_Y] / value;
                result.value[VEC_Z] = lhs.value[VEC_Z] / value;
                return result;
            }

            static inline VecValue<3> Mul(const VecValue<3>& lhs, const VecValue<3>& rhs)
            {
                VecValue<3> result = {};
                result.value[VEC_X] = lhs.value[VEC_X] * rhs.value[VEC_X];
                result.value[VEC_Y] = lhs.value[VEC_Y] * rhs.value[VEC_Y];
                result.value[VEC_Z] = lhs.value[VEC_Z] * rhs.value[VEC_Z];
                return result;
            }

            static inline VecValue<3> Div(const VecValue<3>& lhs, const VecValue<3>& rhs)
            {
                VecValue<3> result = {};
                result.value[VEC_X] = lhs.value[VEC_X] / rhs.value[VEC_X];
                result.value[VEC_Y] = lhs.value[VEC_Y] / rhs.value[VEC_Y];
                result.value[VEC_Z] = lhs.value[VEC_Z] / rhs.value[VEC_Z];
                return result;
            }

            static inline float Dot(const VecValue<3>& lhs, const VecValue<3>& rhs)
            {
                return lhs.value[VEC_X] * rhs.value[VEC_X] +
                    lhs.value[VEC_Y] * rhs.value[VEC_Y] +
                    lhs.value[VEC_Z] * rhs.value[VEC_Z];
            }

            static inline float Length(const VecValue<3>& v)
            {
                return static_cast<float>(sqrt(Dot(v, v)));
            }

            static inline float Distance(const VecValue<3>& lhs, const VecValue<3>& rhs)
            {
                auto dir = Sub(lhs, rhs);
                return Length(dir);
            }

            static inline void SetValue(VecValue<3>& value, float a)
            {
                value.value[VEC_X] = a;
                value.value[VEC_Y] = a;
                value.value[VEC_Z] = a;
            }

            static inline void SetValue(VecValue<3>& value, float a, float b, float c, float)
            {
                value.value[VEC_X] = a;
                value.value[VEC_Y] = b;
                value.value[VEC_Z] = c;
            }

            static inline VecValue<3> Normalize(const VecValue<3>& lhs)
            {
                float rsqrt = 1.f / sqrt(Dot(lhs, lhs));
                return Mul(lhs, rsqrt);
            }

            static inline VecValue<3> Cross(const VecValue<3>& lhs, const VecValue<3>& rhs)
            {
                VecValue<3> result = {};
                result[VEC_X] = lhs[VEC_Y] * rhs[VEC_Z] - lhs[VEC_Z] * rhs[VEC_Y];
                result[VEC_Y] = lhs[VEC_Z] * rhs[VEC_X] - lhs[VEC_X] * rhs[VEC_Z];
                result[VEC_Z] = lhs[VEC_X] * rhs[VEC_Y] - lhs[VEC_Y] * rhs[VEC_X];
                return result;
            }

            static inline VecValue<3> ComponentMin(const VecValue<3>& lhs, const VecValue<3>& rhs)
            {
                VecValue<3> result = {};
                result.value[VEC_X] = fmin(lhs.value[VEC_X], rhs.value[VEC_X]);
                result.value[VEC_Y] = fmin(lhs.value[VEC_Y], rhs.value[VEC_Y]);
                result.value[VEC_Z] = fmin(lhs.value[VEC_Z], rhs.value[VEC_Z]);
                return result;
            }

            static inline VecValue<3> ComponentMax(const VecValue<3>& lhs, const VecValue<3>& rhs)
            {
                VecValue<3> result = {};
                result.value[VEC_X] = fmax(lhs.value[VEC_X], rhs.value[VEC_X]);
                result.value[VEC_Y] = fmax(lhs.value[VEC_Y], rhs.value[VEC_Y]);
                result.value[VEC_Z] = fmax(lhs.value[VEC_Z], rhs.value[VEC_Z]);
                return result;
            }

            static inline bool CmpAllEq(const VecValue<3>& lhs, const VecValue<3>& rhs)
            {
                return (Math::Equal(lhs.value[VEC_X], rhs.value[VEC_X]) &&
                        Math::Equal(lhs.value[VEC_Y], rhs.value[VEC_Y]) &&
                        Math::Equal(lhs.value[VEC_Z], rhs.value[VEC_Z]));
            }

            static inline bool CmpAllLt(const VecValue<3>& lhs, const VecValue<3>& rhs)
            {
                return (lhs.value[VEC_X] < rhs.value[VEC_X])
                || (lhs.value[VEC_Y] < rhs.value[VEC_Y])
                || (lhs.value[VEC_Z] < rhs.value[VEC_Z]);
            }

            static inline bool CmpAllLtEq(const VecValue<3>& lhs, const VecValue<3>& rhs)
            {
                return (lhs.value[VEC_X] <= rhs.value[VEC_X])
                || (lhs.value[VEC_Y] <= rhs.value[VEC_Y])
                || (lhs.value[VEC_Z] <= rhs.value[VEC_Z]);
            }

            static inline bool CmpAllGt(const VecValue<3>& lhs, const VecValue<3>& rhs)
            {
                return (lhs.value[VEC_X] > rhs.value[VEC_X])
                || (lhs.value[VEC_Y] > rhs.value[VEC_Y])
                || (lhs.value[VEC_Z] > rhs.value[VEC_Z]);
            }

            static inline bool CmpAllGtEq(const VecValue<3>& lhs, const VecValue<3>& rhs)
            {
                return (lhs.value[VEC_X] >= rhs.value[VEC_X])
                || (lhs.value[VEC_Y] >= rhs.value[VEC_Y])
                || (lhs.value[VEC_Z] >= rhs.value[VEC_Z]);
            }
        };

        /*********************************Vector4*************************************/
        // vector4 has 4 member.
        template <>
        struct Calculator<VecValue<4>> {
            static inline VecValue<4> Add(const VecValue<4>& lhs, const VecValue<4>& rhs)
            {
                VecValue<4> result = {};
                result.value[VEC_X] = lhs.value[VEC_X] + rhs.value[VEC_X];
                result.value[VEC_Y] = lhs.value[VEC_Y] + rhs.value[VEC_Y];
                result.value[VEC_Z] = lhs.value[VEC_Z] + rhs.value[VEC_Z];
                result.value[VEC_W] = lhs.value[VEC_W] + rhs.value[VEC_W];
                return result;
            }

            static inline VecValue<4> Sub(const VecValue<4>& lhs, const VecValue<4>& rhs)
            {
                VecValue<4> result = {};
                result.value[VEC_X] = lhs.value[VEC_X] - rhs.value[VEC_X];
                result.value[VEC_Y] = lhs.value[VEC_Y] - rhs.value[VEC_Y];
                result.value[VEC_Z] = lhs.value[VEC_Z] - rhs.value[VEC_Z];
                result.value[VEC_W] = lhs.value[VEC_W] - rhs.value[VEC_W];
                return result;
            }

            static inline VecValue<4> Cross(const VecValue<4>& lhs, const VecValue<4>& rhs)
            {
                VecValue<4> result = {};
                result[VEC_X] = lhs[VEC_Y] * rhs[VEC_Z] - lhs[VEC_Z] * rhs[VEC_Y];
                result[VEC_Y] = lhs[VEC_Z] * rhs[VEC_X] - lhs[VEC_X] * rhs[VEC_Z];
                result[VEC_Z] = lhs[VEC_X] * rhs[VEC_Y] - lhs[VEC_Y] * rhs[VEC_X];
                result[VEC_W] = 0.0;
                return result;
            }

            static inline VecValue<4> Mul(const VecValue<4>& lhs, float value)
            {
                VecValue<4> result = {};
                result.value[VEC_X] = lhs.value[VEC_X] * value;
                result.value[VEC_Y] = lhs.value[VEC_Y] * value;
                result.value[VEC_Z] = lhs.value[VEC_Z] * value;
                result.value[VEC_W] = lhs.value[VEC_W] * value;
                return result;
            }

            static inline VecValue<4> Mul(const VecValue<4>& lhs, const VecValue<4>& rhs)
            {
                VecValue<4> result = {};
                result.value[VEC_X] = lhs.value[VEC_X] * rhs.value[VEC_X];
                result.value[VEC_Y] = lhs.value[VEC_Y] * rhs.value[VEC_Y];
                result.value[VEC_Z] = lhs.value[VEC_Z] * rhs.value[VEC_Z];
                result.value[VEC_W] = lhs.value[VEC_W] * rhs.value[VEC_W];
                return result;
            }

            static inline VecValue<4> Div(const VecValue<4>& lhs, const VecValue<4>& rhs)
            {
                VecValue<4> result = {};
                result.value[VEC_X] = lhs.value[VEC_X] / rhs.value[VEC_X];
                result.value[VEC_Y] = lhs.value[VEC_Y] / rhs.value[VEC_Y];
                result.value[VEC_Z] = lhs.value[VEC_Z] / rhs.value[VEC_Z];
                result.value[VEC_W] = lhs.value[VEC_W] / rhs.value[VEC_W];
                return result;
            }

            static inline float Dot(const VecValue<4>& lhs, const VecValue<4>& rhs)
            {
                return lhs.value[VEC_X] * rhs.value[VEC_X] +
                    lhs.value[VEC_Y] * rhs.value[VEC_Y] +
                    lhs.value[VEC_Z] * rhs.value[VEC_Z] +
                    lhs.value[VEC_W] * rhs.value[VEC_W];
            }

            static inline float Length(const VecValue<4>& v)
            {
                return static_cast<float>(sqrt(Dot(v, v)));
            }

            static inline float Distance(const VecValue<4>& lhs, const VecValue<4>& rhs)
            {
                auto dir = Sub(lhs, rhs);
                return Length(dir);
            }

            static inline void SetValue(VecValue<4>& value, float a)
            {
                value.value[VEC_X] = a;
                value.value[VEC_Y] = a;
                value.value[VEC_Z] = a;
                value.value[VEC_W] = a;
            }

            static inline void SetValue(VecValue<4>& value, float a, float b, float c, float d)
            {
                value.value[VEC_X] = a;
                value.value[VEC_Y] = b;
                value.value[VEC_Z] = c;
                value.value[VEC_W] = d;
            }

            static inline VecValue<4> Normalize(const VecValue<4>& lhs)
            {
                float rsqrt = 1.f / sqrt(Dot(lhs, lhs));
                return Mul(lhs, rsqrt);
            }

            static inline VecValue<4> ComponentMin(const VecValue<4>& lhs, const VecValue<4>& rhs)
            {
                VecValue<4> result = {};
                result.value[VEC_X] = fmin(lhs.value[VEC_X], rhs.value[VEC_X]);
                result.value[VEC_Y] = fmin(lhs.value[VEC_Y], rhs.value[VEC_Y]);
                result.value[VEC_Z] = fmin(lhs.value[VEC_Z], rhs.value[VEC_Z]);
                result.value[VEC_W] = fmin(lhs.value[VEC_W], rhs.value[VEC_W]);
                return result;
            }

            static inline VecValue<4> ComponentMax(const VecValue<4>& lhs, const VecValue<4>& rhs)
            {
                VecValue<4> result = {};
                result.value[VEC_X] = fmax(lhs.value[VEC_X], rhs.value[VEC_X]);
                result.value[VEC_Y] = fmax(lhs.value[VEC_Y], rhs.value[VEC_Y]);
                result.value[VEC_Z] = fmax(lhs.value[VEC_Z], rhs.value[VEC_Z]);
                result.value[VEC_W] = fmax(lhs.value[VEC_W], rhs.value[VEC_W]);
                return result;
            }

            static inline bool CmpAllEq(const VecValue<4>& lhs, const VecValue<4>& rhs)
            {
                return (Math::Equal(lhs.value[VEC_X], rhs.value[VEC_X]) &&
                        Math::Equal(lhs.value[VEC_Y], rhs.value[VEC_Y]) &&
                        Math::Equal(lhs.value[VEC_Z], rhs.value[VEC_Z]) &&
                        Math::Equal(lhs.value[VEC_W], rhs.value[VEC_W]));
            }

            static inline bool CmpAllLt(const VecValue<4>& lhs, const VecValue<4>& rhs)
            {
                return (lhs.value[VEC_X] < rhs.value[VEC_X]) || (lhs.value[VEC_Y] < rhs.value[VEC_Y])
                || (lhs.value[VEC_Z] < rhs.value[VEC_Z]) || (lhs.value[VEC_W] < rhs.value[VEC_W]);
            }

            static inline bool CmpAllLtEq(const VecValue<4>& lhs, const VecValue<4>& rhs)
            {
                return (lhs.value[VEC_X] <= rhs.value[VEC_X]) || (lhs.value[VEC_Y] <= rhs.value[VEC_Y])
                || (lhs.value[VEC_Z] <= rhs.value[VEC_Z]) || (lhs.value[VEC_W] <= rhs.value[VEC_W]);
            }

            static inline bool CmpAllGt(const VecValue<4>& lhs, const VecValue<4>& rhs)
            {
                return (lhs.value[VEC_X] > rhs.value[VEC_X]) || (lhs.value[VEC_Y] > rhs.value[VEC_Y])
                || (lhs.value[VEC_Z] > rhs.value[VEC_Z]) || (lhs.value[VEC_W] > rhs.value[VEC_W]);
            }

            static inline bool CmpAllGtEq(const VecValue<4>& lhs, const VecValue<4>& rhs)
            {
                return (lhs.value[VEC_X] >= rhs.value[VEC_X]) || (lhs.value[VEC_Y] >= rhs.value[VEC_Y])
                || (lhs.value[VEC_Z] >= rhs.value[VEC_Z]) || (lhs.value[VEC_W] >= rhs.value[VEC_W]);
            }

            static inline VecValue<4> GetConjugate(const VecValue<4>& lhs)
            {
                VecValue<4> result = {};
                result.value[VEC_X] = -lhs.value[VEC_X];
                result.value[VEC_Y] = -lhs.value[VEC_Y];
                result.value[VEC_Z] = -lhs.value[VEC_Z];
                result.value[VEC_W] = lhs.value[VEC_W];
                return result;
            }

            static inline VecValue<4> Madd(const VecValue<4>& mul1, const VecValue<4>& mul2, const VecValue<4>& add)
            {
                return Add(Mul(mul1, mul2), add);
            }

            static inline VecValue<4> Lerp(const VecValue<4>& src, const VecValue<4>& dest, const VecValue<4>& alpha)
            {
                return Madd(Sub(dest, src), alpha, src);
            }
        };
#endif
    }

    inline Vector3::Vector3()
    {
        detail::Calculator<MemType>::SetValue(value, 0, 0, 0, 0);
    }

    inline Vector3::Vector3(float v)
    {
        detail::Calculator<MemType>::SetValue(value, v);
    }

    inline Vector3::Vector3(float tx, float ty, float tz)
    {
        detail::Calculator<MemType>::SetValue(value, tx, ty, tz, 0);
    }

    inline Vector3::Vector3(const Vector3& v)
    {
        detail::Calculator<MemType>::SetValue(value, v.x, v.y, v.z, 0);
    }

    inline Vector3::Vector3(const Vector4& v)
    {
        detail::Calculator<MemType>::SetValue(value, v.x, v.y, v.z, 0);
    }

    inline Vector3& Vector3::operator=(const Vector3& v)
    {
        detail::Calculator<MemType>::SetValue(value, v.x, v.y, v.z, 0);
        return *this;
    }

    inline Vector3& Vector3::operator+=(const Vector3& v)
    {
        value = detail::Calculator<MemType>::Add(value, v.value);
        return *this;
    }

    inline Vector3& Vector3::operator-=(const Vector3& v)
    {
        value = detail::Calculator<MemType>::Sub(value, v.value);
        return *this;
    }

    inline Vector3& Vector3::operator+=(float v)
    {
        value = detail::Calculator<MemType>::Add(value, Vector3{ v, v, v }.value);
        return *this;
    }

    inline Vector3& Vector3::operator-=(float v)
    {
        value = detail::Calculator<MemType>::Sub(value, Vector3{ v, v, v }.value);
        return *this;
    }

    inline Vector3& Vector3::operator*=(const Vector3& v)
    {
        value = detail::Calculator<MemType>::Mul(value, v.value);
        return *this;
    }

    inline Vector3& Vector3::operator/=(const Vector3& v)
    {
        value = detail::Calculator<MemType>::Div(value, v.value);
        return *this;
    }

    inline Vector3& Vector3::operator*=(float v)
    {
        value = detail::Calculator<MemType>::Mul(value, Vector3{v, v, v}.value);
        return *this;
    }

    inline Vector3& Vector3::operator/=(float v)
    {
        value = detail::Calculator<MemType>::Div(value, Vector3{v}.value);
        return *this;
    }

    inline bool Vector3::operator==(const Vector3& v) const
    {
        return detail::Calculator<MemType>::CmpAllEq(value, v.value);
    }

    inline bool Vector3::operator!=(const Vector3& v) const
    {
        return !(*this == v);
    }

    inline float& Vector3::operator[](size_t index)
    {
        return (&x)[index];
    }

    inline const float& Vector3::operator[](size_t index) const
    {
        return (&x)[index];
    }

    inline float Vector3::Dot(const Vector3& v) const
    {
        return detail::Calculator<MemType>::Dot(value, v.value);
    }

    inline float Vector3::Length() const
    {
        return detail::Calculator<MemType>::Length(value);
    }

    inline float Vector3::Distance(const Vector3& v) const
    {
        return detail::Calculator<MemType>::Distance(value, v.value);
    }

    inline Vector3 Vector3::Cross(const Vector3& v) const
    {
        Vector3 result;
        result.value = detail::Calculator<MemType>::Cross(value, v.value);
        return result;
    }

    inline Vector3 Vector3::Lerp(const Vector3& dest, float alpha) const
    {
        Vector3 result;
        Vector3 a = Vector3(alpha);
        result.value = detail::Calculator<MemType>::Lerp(value, dest.value, a.value);
        return result;
    }

    inline Vector3 Vector3::ComponentMin(const Vector3& v) const
    {
        Vector3 result;
        result.value = detail::Calculator<MemType>::ComponentMin(value, v.value);
        return result;
    }

    inline Vector3 Vector3::ComponentMax(const Vector3& v) const
    {
        Vector3 result;
        result.value = detail::Calculator<MemType>::ComponentMax(value, v.value);
        return result;
    }

    inline bool Vector3::IsLessThan(const Vector3& rhs) const
    {
        return detail::Calculator<MemType>::CmpAllLt(value, rhs.value);
    }

    inline bool Vector3::IsLessEqualThan(const Vector3& rhs) const
    {
        return detail::Calculator<MemType>::CmpAllLtEq(value, rhs.value);
    }

    inline bool Vector3::IsGreaterThan(const Vector3& rhs) const
    {
        return detail::Calculator<MemType>::CmpAllGt(value, rhs.value);
    }

    inline bool Vector3::IsGreaterEqualThan(const Vector3& rhs) const
    {
        return detail::Calculator<MemType>::CmpAllGtEq(value, rhs.value);
    }

    inline bool Vector3::IsEqual(const Vector3& rhs) const
    {
        return detail::Calculator<MemType>::CmpAllEq(value, rhs.value);
    }

    inline float Vector3::GetMaxElement() const
    {
        return std::max<float>(x, std::max<float>(y, z));
    }

    inline float Vector3::GetMinElement() const
    {
        return std::min<float>(x, std::min<float>(y, z));
    }

    inline bool Vector3::IsValid() const
    {
        if (std::isnan(x) || std::isinf(x) ||
            std::isnan(y) || std::isinf(y) ||
            std::isnan(z) || std::isinf(z)) {
            return false;
        }
        return true;
    }

    inline Vector3& Vector3::Normalize()
    {
        value = detail::Calculator<MemType>::Normalize(value);
        return *this;
    }

    inline Vector3 Vector3::Reciprocal() const
    {
        return Vector3(1.0f / x, 1.0f / y, 1.0f / z);
    }

    inline Vector4::Vector4()
    {
        detail::Calculator<MemType>::SetValue(value, 0.0f, 0.0f, 0.0f, 0.0f);
    }

    inline Vector4::Vector4(float v)
    {
        detail::Calculator<MemType>::SetValue(value, v, v, v, v);
    }

    inline Vector4::Vector4(float tx, float ty, float tz, float tw)
    {
        detail::Calculator<MemType>::SetValue(value, tx, ty, tz, tw);
    }

    inline Vector4::Vector4(const Vector3& v, float w)
    {
        detail::Calculator<MemType>::SetValue(value, v.x, v.y, v.z, w);
    }

    inline void Vector4::operator=(const Vector3& v)
    {
        detail::Calculator<MemType>::SetValue(value, v.x, v.y, v.z, 1.0f);
    }

    inline Vector4& Vector4::operator+=(const Vector4& v)
    {
        value = detail::Calculator<MemType>::Add(value, v.value);
        return *this;
    }

    inline Vector4& Vector4::operator-=(const Vector4& v)
    {
        value = detail::Calculator<MemType>::Sub(value, v.value);
        return *this;
    }

    inline Vector4& Vector4::operator*=(const Vector4& v)
    {
        value = detail::Calculator<MemType>::Mul(value, v.value);
        return *this;
    }

    inline Vector4& Vector4::operator/=(const Vector4& v)
    {
        value = detail::Calculator<MemType>::Div(value, v.value);
        return *this;
    }

    inline Vector4& Vector4::operator*=(float v)
    {
        value = detail::Calculator<MemType>::Mul(value, Vector4{v, v, v, v}.value);
        return *this;
    }

    inline Vector4& Vector4::operator/=(float v)
    {
        value = detail::Calculator<MemType>::Div(value, Vector4{v, v, v, v}.value);
        return *this;
    }

    inline bool Vector4::operator==(const Vector4& v) const
    {
        return detail::Calculator<MemType>::CmpAllEq(value, v.value);
    }

    inline bool Vector4::operator!=(const Vector4& v) const
    {
        return !(*this == v);
    }

    inline float& Vector4::operator[](size_t index)
    {
        return (&x)[index];
    }

    inline const float& Vector4::operator[](size_t index) const
    {
        return (&x)[index];
    }

    inline float Vector4::Dot(const Vector4& v) const
    {
        return detail::Calculator<MemType>::Dot(value, v.value);
    }

    inline float Vector4::Length() const
    {
        return detail::Calculator<MemType>::Length(value);
    }

    inline float Vector4::Distance(const Vector4& v) const
    {
        return detail::Calculator<MemType>::Distance(value, v.value);
    }

    inline Vector4& Vector4::Normalize()
    {
        value = detail::Calculator<MemType>::Normalize(value);
        return *this;
    }

    inline Vector4 Vector4::ComponentMin(const Vector4& v) const
    {
        Vector4 result;
        result.value = detail::Calculator<MemType>::ComponentMin(value, v.value);
        return result;
    }

    inline Vector4 Vector4::ComponentMax(const Vector4& v) const
    {
        Vector4 result;
        result.value = detail::Calculator<MemType>::ComponentMax(value, v.value);
        return result;
    }

    inline bool Vector4::IsLessThan(const Vector4& rhs) const
    {
        return detail::Calculator<MemType>::CmpAllLt(value, rhs.value);
    }

    inline bool Vector4::IsLessEqualThan(const Vector4& rhs) const
    {
        return detail::Calculator<MemType>::CmpAllLtEq(value, rhs.value);
    }


    inline bool Vector4::IsGreaterThan(const Vector4& rhs) const
    {
        return detail::Calculator<MemType>::CmpAllGt(value, rhs.value);
    }


    inline bool Vector4::IsGreaterEqualThan(const Vector4& rhs) const
    {
        return detail::Calculator<MemType>::CmpAllGtEq(value, rhs.value);
    }

    inline bool Vector4::IsEqual(const Vector4& rhs) const
    {
        return detail::Calculator<MemType>::CmpAllEq(value, rhs.value);
    }
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
