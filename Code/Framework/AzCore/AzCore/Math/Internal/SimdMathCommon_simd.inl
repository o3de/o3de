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
        alignas(16) constexpr float g_sinCoef1[4]    = { -0.0001950727f, -0.0001950727f, -0.0001950727f, -0.0001950727f };
        alignas(16) constexpr float g_sinCoef2[4]    = {  0.0083320758f,  0.0083320758f,  0.0083320758f,  0.0083320758f };
        alignas(16) constexpr float g_sinCoef3[4]    = { -0.1666665247f, -0.1666665247f, -0.1666665247f, -0.1666665247f };
        alignas(16) constexpr float g_cosCoef1[4]    = { -0.0013602249f, -0.0013602249f, -0.0013602249f, -0.0013602249f };
        alignas(16) constexpr float g_cosCoef2[4]    = {  0.0416566950f,  0.0416566950f,  0.0416566950f,  0.0416566950f };
        alignas(16) constexpr float g_cosCoef3[4]    = { -0.4999990225f, -0.4999990225f, -0.4999990225f, -0.4999990225f };
        alignas(16) constexpr float g_acosHiCoef1[4] = { -0.0012624911f, -0.0012624911f, -0.0012624911f, -0.0012624911f };
        alignas(16) constexpr float g_acosHiCoef2[4] = {  0.0066700901f,  0.0066700901f,  0.0066700901f,  0.0066700901f };
        alignas(16) constexpr float g_acosHiCoef3[4] = { -0.0170881256f, -0.0170881256f, -0.0170881256f, -0.0170881256f };
        alignas(16) constexpr float g_acosHiCoef4[4] = {  0.0308918810f,  0.0308918810f,  0.0308918810f,  0.0308918810f };
        alignas(16) constexpr float g_acosLoCoef1[4] = { -0.0501743046f, -0.0501743046f, -0.0501743046f, -0.0501743046f };
        alignas(16) constexpr float g_acosLoCoef2[4] = {  0.0889789874f,  0.0889789874f,  0.0889789874f,  0.0889789874f };
        alignas(16) constexpr float g_acosLoCoef3[4] = { -0.2145988016f, -0.2145988016f, -0.2145988016f, -0.2145988016f };
        alignas(16) constexpr float g_acosLoCoef4[4] = {  1.5707963050f,  1.5707963050f,  1.5707963050f,  1.5707963050f };
        alignas(16) constexpr float g_acosCoef1[4]   = { -0.0200752200f, -0.0200752200f, -0.0200752200f, -0.0200752200f };
        alignas(16) constexpr float g_acosCoef2[4]   = {  0.0759031500f,  0.0759031500f,  0.0759031500f,  0.0759031500f };
        alignas(16) constexpr float g_acosCoef3[4]   = { -0.2126757000f, -0.2126757000f, -0.2126757000f, -0.2126757000f };
        alignas(16) constexpr float g_atanHiRange[4] = {  2.4142135624f,  2.4142135624f,  2.4142135624f,  2.4142135624f };
        alignas(16) constexpr float g_atanLoRange[4] = {  0.4142135624f,  0.4142135624f,  0.4142135624f,  0.4142135624f };
        alignas(16) constexpr float g_atanCoef1[4]   = {  8.05374449538e-2f,  8.05374449538e-2f,  8.05374449538e-2f,  8.05374449538e-2f };
        alignas(16) constexpr float g_atanCoef2[4]   = { -1.38776856032e-1f, -1.38776856032e-1f, -1.38776856032e-1f, -1.38776856032e-1f };
        alignas(16) constexpr float g_atanCoef3[4]   = {  1.99777106478e-1f,  1.99777106478e-1f,  1.99777106478e-1f,  1.99777106478e-1f };
        alignas(16) constexpr float g_atanCoef4[4]   = { -3.33329491539e-1f, -3.33329491539e-1f, -3.33329491539e-1f, -3.33329491539e-1f };
        alignas(16) constexpr float g_expCoef1[4]    = {  1.2102203e7f, 1.2102203e7f, 1.2102203e7f, 1.2102203e7f };
        alignas(16) constexpr int32_t g_expCoef2[4]  = { -8388608, -8388608, -8388608, -8388608 };
        alignas(16) constexpr float g_expCoef3[4]    = {  1.1920929e-7f, 1.1920929e-7f, 1.1920929e-7f, 1.1920929e-7f };
        alignas(16) constexpr float g_expCoef4[4]    = {  3.371894346e-1, 3.371894346e-1, 3.371894346e-1f, 3.371894346e-1f };
        alignas(16) constexpr float g_expCoef5[4]    = {  6.57636276e-1f, 6.57636276e-1f, 6.57636276e-1f, 6.57636276e-1f };
        alignas(16) constexpr float g_expCoef6[4]    = {  1.00172476f, 1.00172476f, 1.00172476f, 1.00172476f };

        namespace Common
        {
            template <typename VecType>
            AZ_MATH_INLINE typename VecType::FloatType FastLoadConstant(const float* values)
            {
#if AZ_TRAIT_USE_PLATFORM_SIMD_SSE
                return *(typename VecType::FloatType*)(values);
#else
                return VecType::LoadAligned(values);
#endif
            }

            template <typename VecType>
            AZ_MATH_INLINE typename VecType::Int32Type FastLoadConstant(const int32_t* values)
            {
#if AZ_TRAIT_USE_PLATFORM_SIMD_SSE
                return *(typename VecType::Int32Type*)(values);
#else
                return VecType::LoadAligned(values);
#endif
            }

            template <typename VecType>
            AZ_MATH_INLINE typename VecType::FloatType Wrap(typename VecType::FloatArgType value, typename VecType::FloatArgType minValue, typename VecType::FloatArgType maxValue)
            {
                const typename VecType::FloatType valueAdjust = VecType::Sub(value, minValue);
                const typename VecType::FloatType maxAdjust   = VecType::Sub(maxValue, minValue);
                const typename VecType::FloatType valueOffset = VecType::Select(maxAdjust, VecType::ZeroFloat(), VecType::CmpLt(valueAdjust, VecType::ZeroFloat()));
                return VecType::Add(minValue, VecType::Add(valueOffset, VecType::Mod(valueAdjust, maxAdjust)));
            }

            template <typename VecType>
            AZ_MATH_INLINE typename VecType::FloatType AngleMod(typename VecType::FloatArgType value)
            {
                const typename VecType::FloatType vecPi = VecType::Splat(Constants::Pi);
                const typename VecType::FloatType vecTwoPi = VecType::Splat(Constants::TwoPi);
                const typename VecType::FloatType positiveAngles = VecType::Sub(VecType::Mod(VecType::Add(value, vecPi), vecTwoPi), vecPi);
                const typename VecType::FloatType negativeAngles = VecType::Add(VecType::Mod(VecType::Sub(value, vecPi), vecTwoPi), vecPi);
                const typename VecType::FloatType mask = VecType::CmpGtEq(value, VecType::ZeroFloat());
                return VecType::Select(positiveAngles, negativeAngles, mask);
            }

            template <typename VecType>
            AZ_MATH_INLINE void ComputeSinxCosx(typename VecType::FloatArgType x, typename VecType::FloatArgType& sinx, typename VecType::FloatArgType& cosx)
            {
                const typename VecType::FloatType x2 = VecType::Mul(x, x);
                const typename VecType::FloatType x3 = VecType::Mul(x2, x);

                // sin(x) = x3 * (x2 * (x2 * s1 + s2) + s3) + x
                sinx = VecType::Madd(x3,
                    VecType::Madd(x2,
                        VecType::Madd(x2, FastLoadConstant<VecType>(Simd::g_sinCoef1), FastLoadConstant<VecType>(Simd::g_sinCoef2)),
                        FastLoadConstant<VecType>(Simd::g_sinCoef3)),
                    x);

                // cos(x) = x2 * (x2 * (x2 * c1 + c2) + c3) + 1
                cosx = VecType::Madd(x2,
                    VecType::Madd(x2,
                        VecType::Madd(x2, FastLoadConstant<VecType>(Simd::g_cosCoef1), FastLoadConstant<VecType>(Simd::g_cosCoef2)),
                        FastLoadConstant<VecType>(Simd::g_cosCoef3)),
                    VecType::Splat(1.0f));
            }

            template <typename VecType>
            AZ_MATH_INLINE typename VecType::FloatType Sin(typename VecType::FloatArgType value)
            {
                // Range Reduction
                typename VecType::FloatType x = VecType::Mul(value, FastLoadConstant<VecType>(Simd::g_TwoOverPi));

                // Find offset mod 4
                const typename VecType::Int32Type intx = VecType::ConvertToIntNearest(x);
                const typename VecType::Int32Type offset = VecType::And(intx, VecType::Splat(3));

                const typename VecType::FloatType intxFloat = VecType::ConvertToFloat(intx);
                x = VecType::Sub(value, VecType::Mul(intxFloat, FastLoadConstant<VecType>(Simd::g_HalfPi)));

                typename VecType::FloatType sinx, cosx;
                ComputeSinxCosx<VecType>(x, sinx, cosx);

                // Choose sin for even offset, cos for odd
                typename VecType::Int32Type mask = VecType::CmpEq(VecType::And(offset, VecType::Splat(1)), VecType::ZeroInt());
                typename VecType::FloatType result = VecType::Select(sinx, cosx, VecType::CastToFloat(mask));

                // Change sign for result if offset is 1 or 2
                mask = VecType::CmpEq(VecType::And(offset, VecType::Splat(2)), VecType::ZeroInt());
                result = VecType::Select(result, VecType::Xor(result, VecType::Splat(-0.0f)), VecType::CastToFloat(mask));

                return result;
            }

            template <typename VecType>
            AZ_MATH_INLINE typename VecType::FloatType Cos(typename VecType::FloatArgType value)
            {
                // Range Reduction
                typename VecType::FloatType x = VecType::Mul(value, FastLoadConstant<VecType>(Simd::g_TwoOverPi));

                // Find offset mod 4 (additional 1 offset from cos vs sin)
                typename VecType::Int32Type intx = VecType::ConvertToIntNearest(x);
                typename VecType::Int32Type offset = VecType::And(VecType::Add(intx, VecType::Splat(1)), VecType::Splat(3));

                typename VecType::FloatType intxFloat = VecType::ConvertToFloat(intx);
                x = VecType::Sub(value, VecType::Mul(intxFloat, FastLoadConstant<VecType>(Simd::g_HalfPi)));

                typename VecType::FloatType sinx, cosx;
                ComputeSinxCosx<VecType>(x, sinx, cosx);

                // Choose sin for even offset, cos for odd
                typename VecType::Int32Type mask = VecType::CmpEq(VecType::And(offset, VecType::Splat(1)), VecType::ZeroInt());
                typename VecType::FloatType result = VecType::Select(sinx, cosx, VecType::CastToFloat(mask));

                // Change sign for result if offset is 1 or 2
                mask = VecType::CmpEq(VecType::And(offset, VecType::Splat(2)), VecType::ZeroInt());
                result = VecType::Select(result, VecType::Xor(result, VecType::Splat(-0.0f)), VecType::CastToFloat(mask));

                return result;
            }

            template <typename VecType>
            AZ_MATH_INLINE void SinCos(typename VecType::FloatArgType value, typename VecType::FloatArgType& sin, typename VecType::FloatArgType& cos)
            {
                // Range Reduction
                typename VecType::FloatType x = VecType::Mul(value, FastLoadConstant<VecType>(Simd::g_TwoOverPi));

                // Find offset mod 4
                typename VecType::Int32Type intx = VecType::ConvertToIntNearest(x);
                typename VecType::Int32Type offsetSin = VecType::And(intx, VecType::Splat(3));
                typename VecType::Int32Type offsetCos = VecType::And(VecType::Add(intx, VecType::Splat(1)), VecType::Splat(3));

                typename VecType::FloatType intxFloat = VecType::ConvertToFloat(intx);
                x = VecType::Sub(value, VecType::Mul(intxFloat, FastLoadConstant<VecType>(Simd::g_HalfPi)));

                typename VecType::FloatType sinx, cosx;
                ComputeSinxCosx<VecType>(x, sinx, cosx);

                // Choose sin for even offset, cos for odd
                typename VecType::FloatType sinMask = VecType::CastToFloat(VecType::CmpEq(VecType::And(offsetSin, VecType::Splat(1)), VecType::ZeroInt()));
                typename VecType::FloatType cosMask = VecType::CastToFloat(VecType::CmpEq(VecType::And(offsetCos, VecType::Splat(1)), VecType::ZeroInt()));

                sin = VecType::Select(sinx, cosx, sinMask);
                cos = VecType::Select(sinx, cosx, cosMask);

                // Change sign for result if offset puts it in quadrant 1 or 2
                sinMask = VecType::CastToFloat(VecType::CmpEq(VecType::And(offsetSin, VecType::Splat(2)), VecType::ZeroInt()));
                cosMask = VecType::CastToFloat(VecType::CmpEq(VecType::And(offsetCos, VecType::Splat(2)), VecType::ZeroInt()));

                sin = VecType::Select(sin, VecType::Xor(sin, FastLoadConstant<VecType>(reinterpret_cast<const float*>(Simd::g_negateMask))), sinMask);
                cos = VecType::Select(cos, VecType::Xor(cos, FastLoadConstant<VecType>(reinterpret_cast<const float*>(Simd::g_negateMask))), cosMask);
            }

            template <typename VecType>
            AZ_MATH_INLINE typename VecType::FloatType SinCos(typename VecType::FloatArgType angles)
            {
                const typename VecType::FloatType angleOffset = VecType::LoadImmediate(0.0f, Constants::HalfPi, 0.0f, Constants::HalfPi);
                const typename VecType::FloatType sinAngles = VecType::Add(angles, angleOffset);
                return VecType::Sin(sinAngles);
            }

            template <typename VecType>
            AZ_MATH_INLINE typename VecType::FloatType Acos(typename VecType::FloatArgType value)
            {
                const typename VecType::FloatType xabs = VecType::Abs(value);
                const typename VecType::FloatType xabs2 = VecType::Mul(xabs, xabs);
                const typename VecType::FloatType xabs4 = VecType::Mul(xabs2, xabs2);
                const typename VecType::FloatType t1 = VecType::Sqrt(VecType::Sub(VecType::Splat(1.0f), xabs));

                const typename VecType::FloatType select = VecType::CmpLt(value, VecType::ZeroFloat());

                const typename VecType::FloatType hi = VecType::Madd(
                                                            xabs,
                                                            VecType::Madd(
                                                                xabs,
                                                                VecType::Madd(
                                                                    xabs, FastLoadConstant<VecType>(g_acosHiCoef1), FastLoadConstant<VecType>(g_acosHiCoef2)
                                                                ),
                                                                FastLoadConstant<VecType>(g_acosHiCoef3)
                                                            ),
                                                            FastLoadConstant<VecType>(g_acosHiCoef4)
                                                         );

                const typename VecType::FloatType lo = VecType::Madd(
                                                            xabs,
                                                            VecType::Madd(
                                                                xabs,
                                                                VecType::Madd(
                                                                    xabs, FastLoadConstant<VecType>(g_acosLoCoef1), FastLoadConstant<VecType>(g_acosLoCoef2)
                                                                ),
                                                                FastLoadConstant<VecType>(g_acosLoCoef3)
                                                            ),
                                                            FastLoadConstant<VecType>(g_acosLoCoef4)
                                                        );

                const typename VecType::FloatType result = VecType::Madd(hi, xabs4, lo);

                const typename VecType::FloatType positive = VecType::Mul(t1, result);
                const typename VecType::FloatType negative = VecType::Sub(VecType::Splat(Constants::Pi), positive);

                return VecType::Select(negative, positive, select);
            }

            template <typename VecType>
            AZ_MATH_INLINE typename VecType::FloatType AcosEstimate(typename VecType::FloatArgType value)
            {
                const typename VecType::FloatType xabs = VecType::Abs(value);
                const typename VecType::FloatType t1 = VecType::SqrtEstimate(VecType::Sub(VecType::Splat(1.0f), xabs));

                const typename VecType::FloatType select = VecType::CmpLt(value, VecType::ZeroFloat());

                const typename VecType::FloatType result = VecType::Madd(
                                                            xabs,
                                                            VecType::Madd(
                                                                xabs,
                                                                VecType::Madd(
                                                                    xabs, FastLoadConstant<VecType>(g_acosCoef1), FastLoadConstant<VecType>(g_acosCoef2)
                                                                ),
                                                                FastLoadConstant<VecType>(g_acosCoef3)
                                                            ),
                                                            FastLoadConstant<VecType>(g_HalfPi)
                                                         );

                const typename VecType::FloatType positive = VecType::Mul(t1, result);
                const typename VecType::FloatType negative = VecType::Sub(VecType::Splat(Constants::Pi), positive);

                return VecType::Select(negative, positive, select);
            }

            template <typename VecType>
            AZ_MATH_INLINE typename VecType::FloatType Atan(typename VecType::FloatArgType value)
            {
                typename VecType::FloatType x = value;
                const typename VecType::FloatType signbit = VecType::And(x, VecType::CastToFloat(FastLoadConstant<VecType>(Simd::g_negateMask)));

                const typename VecType::FloatType xabs = VecType::Abs(x);

                // Test for x > Sqrt(2) + 1
                const typename VecType::FloatType cmp0 = VecType::CmpGt(xabs, FastLoadConstant<VecType>(Simd::g_atanHiRange));
                // Test for x > Sqrt(2) - 1
                typename VecType::FloatType cmp1 = VecType::CmpGt(xabs, FastLoadConstant<VecType>(Simd::g_atanLoRange));
                // Test for Sqrt(2) + 1 >= x > Sqrt(2) - 1
                const typename VecType::FloatType cmp2 = VecType::AndNot(cmp0, cmp1);

                // -1/x
                // this step is calculated for all values of x, but only used if x > Sqrt(2) + 1
                // in order to avoid a division by zero, detect if xabs is zero here and replace it with an arbitrary value
                // if xabs does equal zero, the value here doesn't matter because the result will be thrown away
                typename VecType::FloatType xabsSafe =
                    VecType::Add(xabs, VecType::And(VecType::CmpEq(xabs, VecType::ZeroFloat()), FastLoadConstant<VecType>(Simd::g_vec1111)));
                const typename VecType::FloatType y0 = VecType::And(cmp0, FastLoadConstant<VecType>(Simd::g_HalfPi));
                typename VecType::FloatType x0 = VecType::Div(FastLoadConstant<VecType>(Simd::g_vec1111), xabsSafe);
                x0 = VecType::Xor(x0, VecType::CastToFloat(FastLoadConstant<VecType>(Simd::g_negateMask)));

                const typename VecType::FloatType y1 = VecType::And(cmp2, FastLoadConstant<VecType>(Simd::g_QuarterPi));
                // (x-1)/(x+1)
                const typename VecType::FloatType x1_numer = VecType::Sub(xabs, FastLoadConstant<VecType>(Simd::g_vec1111));
                const typename VecType::FloatType x1_denom = VecType::Add(xabs, FastLoadConstant<VecType>(Simd::g_vec1111));
                const typename VecType::FloatType x1 = VecType::Div(x1_numer, x1_denom);

                typename VecType::FloatType x2 = VecType::And(cmp2, x1);
                x0 = VecType::And(cmp0, x0);
                x2 = VecType::Or(x2, x0);
                cmp1 = VecType::Or(cmp0, cmp2);
                x2 = VecType::And(cmp1, x2);
                x = VecType::AndNot(cmp1, xabs);
                x = VecType::Or(x2, x);

                typename VecType::FloatType y = VecType::Or(y0, y1);

                typename VecType::FloatType x_sqr = VecType::Mul(x, x);
                typename VecType::FloatType x_cub = VecType::Mul(x_sqr, x);

                typename VecType::FloatType result = VecType::Madd(x_cub,
                                                        VecType::Madd(x_sqr,
                                                            VecType::Madd(x_sqr,
                                                                VecType::Madd(x_sqr,
                                                                    FastLoadConstant<VecType>(Simd::g_atanCoef1),
                                                                    FastLoadConstant<VecType>(Simd::g_atanCoef2)),
                                                                FastLoadConstant<VecType>(Simd::g_atanCoef3)),
                                                            FastLoadConstant<VecType>(Simd::g_atanCoef4)),
                                                        x);

                y = VecType::Add(y, result);

                y = VecType::Xor(y, signbit);

                return y;
            }

            template <typename VecType>
            AZ_MATH_INLINE typename VecType::FloatType Atan2(typename VecType::FloatArgType y, typename VecType::FloatArgType x)
            {
                const typename VecType::FloatType x_eq_0 = VecType::CmpEq(x, VecType::ZeroFloat());
                const typename VecType::FloatType x_ge_0 = VecType::CmpGtEq(x, VecType::ZeroFloat());
                const typename VecType::FloatType x_lt_0 = VecType::CmpLt(x, VecType::ZeroFloat());

                const typename VecType::FloatType y_eq_0 = VecType::CmpEq(y, VecType::ZeroFloat());
                const typename VecType::FloatType y_lt_0 = VecType::CmpLt(y, VecType::ZeroFloat());

                // returns 0 if x == y == 0 (degenerate case) or x >= 0, y == 0
                const typename VecType::FloatType zero_mask = VecType::And(x_ge_0, y_eq_0);

                // returns +/- pi/2 if x == 0, y != 0
                const typename VecType::FloatType pio2_mask = VecType::AndNot(y_eq_0, x_eq_0);
                const typename VecType::FloatType pio2_mask_sign = VecType::And(y_lt_0, VecType::CastToFloat(FastLoadConstant<VecType>(Simd::g_negateMask)));
                typename VecType::FloatType pio2_result = FastLoadConstant<VecType>(g_HalfPi);
                pio2_result = VecType::Xor(pio2_result, pio2_mask_sign);
                pio2_result = VecType::And(pio2_mask, pio2_result);

                // pi when y == 0 and x < 0
                const typename VecType::FloatType pi_mask = VecType::And(y_eq_0, x_lt_0);
                typename VecType::FloatType pi_result = FastLoadConstant<VecType>(g_Pi);
                pi_result = VecType::And(pi_mask, pi_result);

                typename VecType::FloatType swap_sign_mask_offset = VecType::And(x_lt_0, y_lt_0);
                swap_sign_mask_offset = VecType::And(swap_sign_mask_offset, VecType::CastToFloat(FastLoadConstant<VecType>(Simd::g_negateMask)));

                typename VecType::FloatType offset1 = FastLoadConstant<VecType>(g_Pi);
                offset1 = VecType::Xor(offset1, swap_sign_mask_offset);

                typename VecType::FloatType offset = VecType::And(x_lt_0, offset1);

                // the result of this part of the computation is thrown away if x equals 0,
                // but if x does equal 0, it will cause a division by zero
                // so replace zero by an arbitrary value here in that case
                typename VecType::FloatType xSafe = VecType::Add(x, VecType::And(x_eq_0, FastLoadConstant<VecType>(Simd::g_vec1111)));
                const typename VecType::FloatType atan_mask = VecType::Not(VecType::Or(x_eq_0, y_eq_0));
                const typename VecType::FloatType atan_arg = VecType::Div(y, xSafe);
                typename VecType::FloatType atan_result = VecType::Atan(atan_arg);
                atan_result = VecType::Add(atan_result, offset);
                atan_result = VecType::AndNot(pio2_mask, atan_result);
                atan_result = VecType::And(atan_mask, atan_result);

                // Select between zero, pio2, pi, and atan
                typename VecType::FloatType result = VecType::AndNot(zero_mask, pio2_result);
                result = VecType::Or(result, pio2_result);
                result = VecType::Or(result, pi_result);
                result = VecType::Or(result, atan_result);

                return result;
            }

            template <typename VecType>
            AZ_MATH_INLINE typename VecType::FloatType ExpEstimate(typename VecType::FloatArgType x)
            {
                // N. N. Schraudolph, 'A Fast, Compact Approximation of the Exponential Function'
                // This method exploits the logrithmic nature of IEEE-754 floating point to quickly estimate exp(x)
                // While the concept is based on that paper, this specific implementation is based on a selection from several variants 
                // of that algorithm to choose the fastest of the variants that had the highest accuracy.
                typename VecType::Int32Type a = VecType::ConvertToIntNearest(VecType::Mul(FastLoadConstant<VecType>(Simd::g_expCoef1), x));
                typename VecType::Int32Type b = VecType::And(a, FastLoadConstant<VecType>(Simd::g_expCoef2));
                typename VecType::Int32Type c = VecType::Sub(a, b);
                typename VecType::FloatType f = VecType::Mul(FastLoadConstant<VecType>(Simd::g_expCoef3), VecType::ConvertToFloat(c)); // Approximately (x/log(2)) - floor(x/log(2))
                typename VecType::FloatType i = VecType::Madd(f, FastLoadConstant<VecType>(Simd::g_expCoef4), FastLoadConstant<VecType>(Simd::g_expCoef5));
                typename VecType::FloatType j = VecType::Madd(i, f, FastLoadConstant<VecType>(Simd::g_expCoef6)); // Approximately 2^f
                return VecType::CastToFloat(VecType::Add(b, VecType::CastToInt(j)));
            }

            template <typename VecType>
            AZ_MATH_INLINE typename VecType::FloatType Normalize(typename VecType::FloatArgType value)
            {
                const typename VecType::FloatType lengthSquared = VecType::SplatIndex0(VecType::FromVec1(VecType::Dot(value, value)));
                const typename VecType::FloatType length = VecType::Sqrt(lengthSquared);
                return VecType::Div(value, length);
            }

            template <typename VecType>
            AZ_MATH_INLINE typename VecType::FloatType NormalizeEstimate(typename VecType::FloatArgType value)
            {
                const typename VecType::FloatType lengthSquared = VecType::SplatIndex0(VecType::FromVec1(VecType::Dot(value, value)));
                const typename VecType::FloatType invLength = VecType::SqrtInvEstimate(lengthSquared);
                return VecType::Mul(invLength, value);
            }

            template <typename VecType>
            AZ_MATH_INLINE typename VecType::FloatType NormalizeSafe(typename VecType::FloatArgType value, float tolerance)
            {
                const typename VecType::FloatType floatEpsilon = VecType::Splat(tolerance * tolerance);
                const typename VecType::FloatType lengthSquared = VecType::SplatIndex0(VecType::FromVec1(VecType::Dot(value, value)));
                if (VecType::CmpAllLt(lengthSquared, floatEpsilon))
                {
                    return VecType::ZeroFloat();
                }
                return VecType::Div(value, VecType::Sqrt(lengthSquared));
            }

            template <typename VecType>
            AZ_MATH_INLINE typename VecType::FloatType NormalizeSafeEstimate(typename VecType::FloatArgType value, float tolerance)
            {
                const typename VecType::FloatType floatEpsilon = VecType::Splat(tolerance * tolerance);
                const typename VecType::FloatType lengthSquared = VecType::SplatIndex0(VecType::FromVec1(VecType::Dot(value, value)));
                if (VecType::CmpAllLt(lengthSquared, floatEpsilon))
                {
                    return VecType::ZeroFloat();
                }
                return VecType::Mul(value, VecType::SqrtInvEstimate(lengthSquared));
            }

            template <typename Vec4Type, typename Vec3Type>
            AZ_MATH_INLINE typename Vec4Type::FloatType QuaternionTransform(typename Vec4Type::FloatArgType quat, typename Vec3Type::FloatArgType vec3)
            {
                const typename Vec4Type::FloatType Two = Vec4Type::Splat(2.0f);
                const typename Vec4Type::FloatType scalar = Vec4Type::SplatIndex3(quat); // Scalar portion of quat (W, W, W)

                const typename Vec4Type::FloatType partial1 = Vec4Type::SplatIndex0(Vec4Type::FromVec1(Vec3Type::Dot(quat, vec3)));
                const typename Vec4Type::FloatType partial2 = Vec4Type::Mul(quat, partial1);
                const typename Vec4Type::FloatType sum1 = Vec4Type::Mul(partial2, Two); // quat.Dot(vec3) * vec3 * 2.0f

                const typename Vec4Type::FloatType partial3 = Vec4Type::SplatIndex0(Vec4Type::FromVec1(Vec3Type::Dot(quat, quat)));
                const typename Vec4Type::FloatType partial4 = Vec4Type::Mul(scalar, scalar);
                const typename Vec4Type::FloatType partial5 = Vec4Type::Sub(partial4, partial3);
                const typename Vec4Type::FloatType sum2 = Vec4Type::Mul(partial5, vec3); // vec3 * (scalar * scalar - quat.Dot(quat))

                const typename Vec4Type::FloatType partial6 = Vec4Type::Mul(scalar, Two);
                const typename Vec4Type::FloatType partial7 = Vec3Type::Cross(quat, vec3);
                const typename Vec4Type::FloatType sum3 = Vec4Type::Mul(partial6, partial7); // scalar * 2.0f * quat.Cross(vec3)

                return Vec4Type::Add(Vec4Type::Add(sum1, sum2), sum3);
            }

            template <typename Vec4Type, typename Vec3Type>
            AZ_MATH_INLINE typename Vec4Type::FloatType ConstructPlane(typename Vec3Type::FloatArgType normal, typename Vec3Type::FloatArgType point)
            {
                const Vec1::FloatType distance = Vec1::Sub(Vec1::ZeroFloat(), Vec3Type::Dot(normal, point));
                return Vec4Type::ReplaceIndex3(normal, Vec4Type::SplatIndex0(Vec4Type::FromVec1(distance))); // replace 'w' coordinate with distance
            }

            template <typename Vec4Type, typename Vec3Type>
            AZ_MATH_INLINE Vec1::FloatType PlaneDistance(typename Vec4Type::FloatArgType plane, typename Vec3Type::FloatArgType point)
            {
                const typename Vec4Type::FloatType referencePoint = Vec4Type::ReplaceIndex3(point, 1.0f); // replace 'w' coordinate with 1.0
                return Vec4Type::Dot(referencePoint, plane);
            }

            template <typename VecType>
            AZ_MATH_INLINE void Mat3x3Multiply(const typename VecType::FloatType* __restrict rowsA, const typename VecType::FloatType* __restrict rowsB, typename VecType::FloatType* __restrict out)
            {
                out[0] = VecType::Madd(VecType::SplatIndex2(rowsA[0]), rowsB[2], VecType::Madd(VecType::SplatIndex1(rowsA[0]), rowsB[1], VecType::Mul(VecType::SplatIndex0(rowsA[0]), rowsB[0])));
                out[1] = VecType::Madd(VecType::SplatIndex2(rowsA[1]), rowsB[2], VecType::Madd(VecType::SplatIndex1(rowsA[1]), rowsB[1], VecType::Mul(VecType::SplatIndex0(rowsA[1]), rowsB[0])));
                out[2] = VecType::Madd(VecType::SplatIndex2(rowsA[2]), rowsB[2], VecType::Madd(VecType::SplatIndex1(rowsA[2]), rowsB[1], VecType::Mul(VecType::SplatIndex0(rowsA[2]), rowsB[0])));
            }

            template <typename VecType>
            AZ_MATH_INLINE void Mat3x3TransposeMultiply(const typename VecType::FloatType* __restrict rowsA, const typename VecType::FloatType* __restrict rowsB, typename VecType::FloatType* __restrict out)
            {
                out[0] = VecType::Madd(VecType::SplatIndex0(rowsA[0]), rowsB[0], VecType::Madd(VecType::SplatIndex0(rowsA[1]), rowsB[1], VecType::Mul(VecType::SplatIndex0(rowsA[2]), rowsB[2])));
                out[1] = VecType::Madd(VecType::SplatIndex1(rowsA[0]), rowsB[0], VecType::Madd(VecType::SplatIndex1(rowsA[1]), rowsB[1], VecType::Mul(VecType::SplatIndex1(rowsA[2]), rowsB[2])));
                out[2] = VecType::Madd(VecType::SplatIndex2(rowsA[0]), rowsB[0], VecType::Madd(VecType::SplatIndex2(rowsA[1]), rowsB[1], VecType::Mul(VecType::SplatIndex2(rowsA[2]), rowsB[2])));
            }

            template <typename VecType>
            AZ_MATH_INLINE typename VecType::FloatType Mat3x3TransformVector(const typename VecType::FloatType* __restrict rows, typename VecType::FloatArgType vector)
            {
                typename VecType::FloatType transposed[3];
                VecType::Mat3x3Transpose(rows, transposed);
                return VecType::Mat3x3TransposeTransformVector(transposed, vector);
            }

            template <typename VecType>
            AZ_MATH_INLINE typename VecType::FloatType Mat3x3TransposeTransformVector(const typename VecType::FloatType* __restrict rows, typename VecType::FloatArgType vector)
            {
                return VecType::Madd(VecType::SplatIndex2(vector), rows[2], VecType::Madd(VecType::SplatIndex1(vector), rows[1], VecType::Mul(VecType::SplatIndex0(vector), rows[0])));
            }

            template <typename VecType>
            AZ_MATH_INLINE void Mat3x4Multiply(const typename VecType::FloatType* __restrict rowsA, const typename VecType::FloatType* __restrict rowsB, typename VecType::FloatType* __restrict out)
            {
                const typename VecType::FloatType fourth = FastLoadConstant<VecType>(g_vec0001);
                out[0] = VecType::Madd(VecType::SplatIndex3(rowsA[0]), fourth, VecType::Madd(VecType::SplatIndex2(rowsA[0]), rowsB[2], VecType::Madd(VecType::SplatIndex1(rowsA[0]), rowsB[1], VecType::Mul(VecType::SplatIndex0(rowsA[0]), rowsB[0]))));
                out[1] = VecType::Madd(VecType::SplatIndex3(rowsA[1]), fourth, VecType::Madd(VecType::SplatIndex2(rowsA[1]), rowsB[2], VecType::Madd(VecType::SplatIndex1(rowsA[1]), rowsB[1], VecType::Mul(VecType::SplatIndex0(rowsA[1]), rowsB[0]))));
                out[2] = VecType::Madd(VecType::SplatIndex3(rowsA[2]), fourth, VecType::Madd(VecType::SplatIndex2(rowsA[2]), rowsB[2], VecType::Madd(VecType::SplatIndex1(rowsA[2]), rowsB[1], VecType::Mul(VecType::SplatIndex0(rowsA[2]), rowsB[0]))));
            }

            template <typename VecType>
            AZ_MATH_INLINE void Mat4x4InverseFast(const typename VecType::FloatType* __restrict rows, typename VecType::FloatType* __restrict out)
            {
                const typename VecType::FloatType pos = VecType::Madd(VecType::SplatIndex3(rows[2]), rows[2]
                                                      , VecType::Madd(VecType::SplatIndex3(rows[1]), rows[1]
                                                      , VecType::Mul (VecType::SplatIndex3(rows[0]), rows[0])));
                typename VecType::FloatType transposed[4] = { rows[0], rows[1], rows[2], VecType::Xor(pos, FastLoadConstant<VecType>(reinterpret_cast<const float*>(g_negateMask))) };
                VecType::Mat4x4Transpose(transposed, out);
                out[3] = FastLoadConstant<VecType>(g_vec0001);
            }

            template <typename VecType>
            AZ_MATH_INLINE void Mat4x4Multiply(const typename VecType::FloatType* __restrict rowsA, const typename VecType::FloatType* __restrict rowsB, typename VecType::FloatType* __restrict out)
            {
                out[0] = VecType::Madd(VecType::SplatIndex3(rowsA[0]), rowsB[3], VecType::Madd(VecType::SplatIndex2(rowsA[0]), rowsB[2], VecType::Madd(VecType::SplatIndex1(rowsA[0]), rowsB[1], VecType::Mul(VecType::SplatIndex0(rowsA[0]), rowsB[0]))));
                out[1] = VecType::Madd(VecType::SplatIndex3(rowsA[1]), rowsB[3], VecType::Madd(VecType::SplatIndex2(rowsA[1]), rowsB[2], VecType::Madd(VecType::SplatIndex1(rowsA[1]), rowsB[1], VecType::Mul(VecType::SplatIndex0(rowsA[1]), rowsB[0]))));
                out[2] = VecType::Madd(VecType::SplatIndex3(rowsA[2]), rowsB[3], VecType::Madd(VecType::SplatIndex2(rowsA[2]), rowsB[2], VecType::Madd(VecType::SplatIndex1(rowsA[2]), rowsB[1], VecType::Mul(VecType::SplatIndex0(rowsA[2]), rowsB[0]))));
                out[3] = VecType::Madd(VecType::SplatIndex3(rowsA[3]), rowsB[3], VecType::Madd(VecType::SplatIndex2(rowsA[3]), rowsB[2], VecType::Madd(VecType::SplatIndex1(rowsA[3]), rowsB[1], VecType::Mul(VecType::SplatIndex0(rowsA[3]), rowsB[0]))));
            }

            template <typename VecType>
            AZ_MATH_INLINE void Mat4x4MultiplyAdd(const typename VecType::FloatType* __restrict rowsA, const typename VecType::FloatType* __restrict rowsB, const typename VecType::FloatType* __restrict add, typename VecType::FloatType* __restrict out)
            {
                out[0] = VecType::Madd(VecType::SplatIndex3(rowsA[0]), rowsB[3], VecType::Madd(VecType::SplatIndex2(rowsA[0]), rowsB[2], VecType::Madd(VecType::SplatIndex1(rowsA[0]), rowsB[1], VecType::Madd(VecType::SplatIndex0(rowsA[0]), rowsB[0], add[0]))));
                out[1] = VecType::Madd(VecType::SplatIndex3(rowsA[1]), rowsB[3], VecType::Madd(VecType::SplatIndex2(rowsA[1]), rowsB[2], VecType::Madd(VecType::SplatIndex1(rowsA[1]), rowsB[1], VecType::Madd(VecType::SplatIndex0(rowsA[1]), rowsB[0], add[1]))));
                out[2] = VecType::Madd(VecType::SplatIndex3(rowsA[2]), rowsB[3], VecType::Madd(VecType::SplatIndex2(rowsA[2]), rowsB[2], VecType::Madd(VecType::SplatIndex1(rowsA[2]), rowsB[1], VecType::Madd(VecType::SplatIndex0(rowsA[2]), rowsB[0], add[2]))));
                out[3] = VecType::Madd(VecType::SplatIndex3(rowsA[3]), rowsB[3], VecType::Madd(VecType::SplatIndex2(rowsA[3]), rowsB[2], VecType::Madd(VecType::SplatIndex1(rowsA[3]), rowsB[1], VecType::Madd(VecType::SplatIndex0(rowsA[3]), rowsB[0], add[3]))));
            }

            template <typename VecType>
            AZ_MATH_INLINE void Mat4x4TransposeMultiply(const typename VecType::FloatType* __restrict rowsA, const typename VecType::FloatType* __restrict rowsB, typename VecType::FloatType* __restrict out)
            {
                out[0] = VecType::Madd(VecType::SplatIndex0(rowsA[0]), rowsB[0], VecType::Madd(VecType::SplatIndex0(rowsA[1]), rowsB[1], VecType::Madd(VecType::SplatIndex0(rowsA[2]), rowsB[2], VecType::Mul(VecType::SplatIndex0(rowsA[3]), rowsB[3]))));
                out[1] = VecType::Madd(VecType::SplatIndex1(rowsA[0]), rowsB[0], VecType::Madd(VecType::SplatIndex1(rowsA[1]), rowsB[1], VecType::Madd(VecType::SplatIndex1(rowsA[2]), rowsB[2], VecType::Mul(VecType::SplatIndex1(rowsA[3]), rowsB[3]))));
                out[2] = VecType::Madd(VecType::SplatIndex2(rowsA[0]), rowsB[0], VecType::Madd(VecType::SplatIndex2(rowsA[1]), rowsB[1], VecType::Madd(VecType::SplatIndex2(rowsA[2]), rowsB[2], VecType::Mul(VecType::SplatIndex2(rowsA[3]), rowsB[3]))));
                out[3] = VecType::Madd(VecType::SplatIndex3(rowsA[0]), rowsB[0], VecType::Madd(VecType::SplatIndex3(rowsA[1]), rowsB[1], VecType::Madd(VecType::SplatIndex3(rowsA[2]), rowsB[2], VecType::Mul(VecType::SplatIndex3(rowsA[3]), rowsB[3]))));
            }

            template <typename VecType>
            AZ_MATH_INLINE typename VecType::FloatType Mat4x4TransposeTransformVector(const typename VecType::FloatType* __restrict rows, typename VecType::FloatArgType vector)
            {
                return VecType::Madd(VecType::SplatIndex3(vector), rows[3], VecType::Madd(VecType::SplatIndex2(vector), rows[2], VecType::Madd(VecType::SplatIndex1(vector), rows[1], VecType::Mul(VecType::SplatIndex0(vector), rows[0]))));
            }
        }
    }
}
