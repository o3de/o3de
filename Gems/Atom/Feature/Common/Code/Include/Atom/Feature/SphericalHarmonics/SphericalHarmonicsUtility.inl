/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/SphericalHarmonics/SphericalHarmonicsUtility.h>

#include <cmath>
#include <utility>
#include <vector>

namespace AZ
{
    namespace Render
    {
        namespace SHConstants
        {
            // used by polynomial solver
            constexpr float K01 = 0.282094791773878f;    // sqrt(  1/PI)/2
            constexpr float K02 = 0.488602511902920f;    // sqrt(  5/PI)/4
            constexpr float K03 = 1.092548430592079f;    // sqrt( 15/PI)/4
            constexpr float K04 = 0.315391565252520f;    // sqrt(  5/PI)/4
            constexpr float K05 = 0.546274215296040f;    // sqrt( 15/PI)/4
            constexpr float K06 = 0.590043589926644f;    // sqrt( 70/PI)/8
            constexpr float K07 = 2.890611442640554f;    // sqrt(105/PI)/2
            constexpr float K08 = 0.457045799464466f;    // sqrt( 42/PI)/8
            constexpr float K09 = 0.373176332590115f;    // sqrt(  7/PI)/4
            constexpr float K10 = 1.445305721320277f;    // sqrt(105/PI)/4
                                  
            // used by ZHF3 rotation
            constexpr float Sqrt3      = 1.732050807568877f;    // sqrt(3)
            constexpr float Sqrt3Div2  = 0.866025403784439f;    // sqrt(3) / 2
            constexpr float C2Div3     = 0.666666666666667f;    // 2 / 3
            constexpr float C1Div3     = 0.333333333333333f;    // 1 / 3

            // upper & lower bounds used by factorial approximation
            // used by factorial approximation
            constexpr double A1 = 0.0833333333333333333333333333333;   //                              1 / 12
            constexpr double A2 = 0.0333333333333333333333333333333;   //                              1 / 30
            constexpr double A3 = 0.2523809523809523809523809523810;   //                             53 / 210
            constexpr double A4 = 0.5256064690026954177897574123989;   //                            195 / 371
            constexpr double A5 = 1.0115230681268417117473721247306;   //                          22999 / 22737
            constexpr double A6 = 1.5174736491532873984284915194955;   //                       29944523 / 19733142
            constexpr double A7 = 2.2694889742049599609091506722099;   //                   109535241009 / 48264275462
            constexpr double A8 = 3.0099173832593981700731407342077;   //           29404527905795295658 / 9769214287853155785
            constexpr double A9 = 4.0268871923439012261688759531814;   // 455377030420113432210116914702 / 113084128923675014537885725485

            // used by naive solver
            constexpr double Sqrt2      = 1.4142135623730950488016887242097;    // sqrt(2)
            constexpr double Inv4Pi     = 0.07957747154594766788444188168626;   // 1 / (4 * Pi)
            constexpr double Ln2Pi      = 1.8378770664093454835606594728112;    // ln(2 * pi)
            constexpr double Pi         = 3.1415926535897932384626433832795;    // Pi

            // used in associated legendre polynomial(ALP)
            constexpr double FactorialLUT[33] =
            {
                1.0,
                1.0,
                2.0,
                6.0,
                24.0,
                120.0,
                720.0,
                5040.0,
                40320.0,
                362880.0,
                3628800.0,
                39916800.0,
                479001600.0,
                6227020800.0,
                87178291200.0,
                1307674368000.0,
                20922789888000.0,
                355687428096000.0,
                6402373705728000.0,
                1.21645100408832e+17,
                2.43290200817664e+18,
                5.109094217170944e+19,
                1.12400072777760768e+21,
                2.58520167388849766e+22,
                6.20448401733239439e+23,
                1.55112100433309860e+25,
                4.03291461126605636e+26,
                1.08888694504183522e+28,
                3.04888344611713861e+29,
                8.84176199373970195e+30,
                2.65252859812191059e+32,
                8.22283865417792282e+33,
                2.63130836933693530e+35
            };

            // Double factorial LUT, each entry represents: (2*index - 1)!! (1 if index is 0)
            // used in ALP
            constexpr double DoubleFactorialLUT[17] =
            {
                1.0,
                1.0,
                3.0,
                15.0,
                105.0,
                945.0,
                10395.0,
                135135.0,
                2027025.0,
                34459425.0,
                654729075.0,
                13749310575.0,
                316234143225.0,
                7905853580625.0,
                213458046676875.0,
                6190283353629375.0,
                1.9189878396251062e+17
            };
        } // namespace SHConstants

        // provided set of functions for evaluating spherical harmonic basis function
        namespace SHBasis
        {
            // A fast implementation for evaluation of first 4 bands (band 0, 1, 2, 3), return 0.0 if band or order is invalid
            // input:
            //  l -> SH band, non negative integer <= 3
            //  m -> SH order, integer within range [-l, l], invalid order will result in unpredicted behaviour
            //dir -> cartesian coordinates (Y-up, -Z-forward) of direction on unit sphere, required to be normalize
            float  EvalSHBasisFast(const int l, const int m, const float dir[3])
            {
                if (l <= 3 && l >= 0 && m <= 3 && m >= -3)
                {
                    return Poly3(l, m, dir);
                }
                else
                {
                    return 0.0f;
                }
            }

            // A fast implementation for evaluation of first 3 bands (band 0, 1, 2), return 0.0 if band or order is invalid
            // input:
            //  l -> SH band, non negative integer
            //  m -> SH order, integer within range [-l, l], invalid order will result in unpredicted behaviour
            //dir -> cartesian coordinates (Y-up, -Z-forward) of direction on unit sphere, required to be normalize
            double EvalSHBasis(const int l, const int m, const float dir[3])
            {
                // gradually shrink to slower but more general slover
                if (l < 0)
                {
                    return 0.0;
                }
                if (l <= 2)
                {
                    return Poly3(l, m, dir);
                }
                else if (l <= 16)
                {
                    return Naive16(l, m, dir);
                }
                else
                {
                    // use brute factorial by default
                    return NaiveEx(l, m, dir, true);
                }
            }

            // ----------L = 0------------

            // SH basis function for band l = 0, order m = 0
            float L0M0 ([[maybe_unused]] const float dir[3]) { return  SHConstants::K01; }

            // ----------L = 1------------

            // SH basis function for band l = 1, order m = -1
            float L1MN1(const float dir[3]) { return -SHConstants::K02 * dir[2]; }
            // SH basis function for band l = 1, order m =  0
            float L1M0 (const float dir[3]) { return  SHConstants::K02 * dir[1]; }
            // SH basis function for band l = 1, order m =  1
            float L1MP1(const float dir[3]) { return -SHConstants::K02 * dir[0]; }

            // ----------L = 2------------

            // SH basis function for band l = 2, order m = -2
            float L2MN2(const float dir[3]) { return  SHConstants::K03 *  dir[0] * dir[2]; }
            // SH basis function for band l = 2, order m = -1
            float L2MN1(const float dir[3]) { return -SHConstants::K03 *  dir[2] * dir[1]; }
            // SH basis function for band l = 2, order m =  0
            float L2M0 (const float dir[3]) { return  SHConstants::K04 * (3.0f * dir[1] * dir[1] - 1.0f); }
            // SH basis function for band l = 2, order m =  1
            float L2MP1(const float dir[3]) { return -SHConstants::K03 *  dir[0] * dir[1]; }
            // SH basis function for band l = 2, order m =  2
            float L2MP2(const float dir[3]) { return  SHConstants::K05 * (dir[0] * dir[0] - dir[2] * dir[2]); }

            // ----------L = 3------------

            // SH basis function for band l = 3, order m = -3
            float L3MN3(const float dir[3]) { return -SHConstants::K06 * dir[2] * (3.0f * dir[0] * dir[0] - dir[2] * dir[2]); }
            // SH basis function for band l = 3, order m = -2
            float L3MN2(const float dir[3]) { return  SHConstants::K07 * dir[1] * dir[2] * dir[0]; }
            // SH basis function for band l = 3, order m = -1
            float L3MN1(const float dir[3]) { return -SHConstants::K08 * dir[2] * (5.0f * dir[1] * dir[1] - 1.0f); }
            // SH basis function for band l = 3, order m =  0
            float L3M0 (const float dir[3]) { return  SHConstants::K09 * dir[1] * (5.0f * dir[1] * dir[1] - 3.0f); }
            // SH basis function for band l = 3, order m =  1
            float L3MP1(const float dir[3]) { return -SHConstants::K08 * dir[0] * (5.0f * dir[1] * dir[1] - 1.0f); }
            // SH basis function for band l = 3, order m =  2
            float L3MP2(const float dir[3]) { return  SHConstants::K10 * dir[1] * (dir[0] * dir[0] - dir[2] * dir[2]); }
            // SH basis function for band l = 3, order m =  3
            float L3MP3(const float dir[3]) { return -SHConstants::K06 * dir[0] * (dir[0] * dir[0] - 3.0f * dir[2] * dir[2]); }

            // Polynomial solver evaluates first 4 bands (0-3) via analytical polynomial form
            // input:
            //  l -> SH band, non negative integer <= 3
            //  m -> SH order, integer within range [-l, l]
            //dir -> cartesian coordinates (Y-up, -Z-forward) of direction on unit sphere, required to be normalized
            // This solver doesn't involve complicate calculations thus single precision float would be enough
            float Poly3(const int l, const int m, const float dir[3])
            {
                int index = l * (l + 1) + m;

                //transform y up, -z forward to z up, -y forward
                float zUpDir[3];
                zUpDir[0] = dir[0];
                zUpDir[1] = dir[2];
                zUpDir[2] = dir[1];

                switch (index)
                {
                    //----------L = 0------------
                    case 0: return   SHConstants::K01;

                    //----------L = 1------------
                    case 1: return  -SHConstants::K02 * zUpDir[1];
                    case 2: return   SHConstants::K02 * zUpDir[2];
                    case 3: return  -SHConstants::K02 * zUpDir[0];

                    //----------L = 2------------
                    case 4: return   SHConstants::K03 * zUpDir[0] * zUpDir[1];
                    case 5: return  -SHConstants::K03 * zUpDir[1] * zUpDir[2];
                    case 6: return   SHConstants::K04 * (3.0f * zUpDir[2] * zUpDir[2] - 1.0f);
                    case 7: return  -SHConstants::K03 * zUpDir[0] * zUpDir[2];
                    case 8: return   SHConstants::K05 * (zUpDir[0] * zUpDir[0] - zUpDir[1] * zUpDir[1]);

                    //----------L = 3------------
                    case 9: return  -SHConstants::K06 * (3.0f * zUpDir[0] * zUpDir[0] - zUpDir[1] * zUpDir[1]) * zUpDir[1];
                    case 10:return   SHConstants::K07 * zUpDir[1] * zUpDir[0] * zUpDir[2];
                    case 11:return  -SHConstants::K08 * zUpDir[1] * (5.0f * zUpDir[2] * zUpDir[2] - 1.0f);
                    case 12:return   SHConstants::K09 * zUpDir[2] * (5.0f * zUpDir[2] * zUpDir[2] - 3.0f);
                    case 13:return  -SHConstants::K08 * zUpDir[0] * (5.0f * zUpDir[2] * zUpDir[2] - 1.0f);
                    case 14:return   SHConstants::K10 * (zUpDir[0] * zUpDir[0] - zUpDir[1] * zUpDir[1]) * zUpDir[2];
                    case 15:return  -SHConstants::K06 * zUpDir[0] * (zUpDir[0] * zUpDir[0] - 3.0f * zUpDir[1] * zUpDir[1]);
                }

                return 0.0;
            }

            // Continued fraction approximation by T.J.Stieltjes,
            // gives 5/2+(13/2)*ln(x) valid siginificant decimal digits, very accurate for floating point result
            // the actual factorial should be exp(output) where exp() is temporarily omitted here
            // Unlike Stirling's formula, Stieltjes's approximation is convergant, i.e. more coefficients(AXs in the function)
            // make result closer to exact number
            // Please see http://oeis.org/wiki/User:Peter_Luschny/FactorialFunction for more information
            double FactorialStieltjesNoExp(uint32_t x)
            {
                // use LUT if possible
                if (x < 33)
                {
                    return log(SHConstants::FactorialLUT[x]);
                }

                double  Z = x + 1.0;
                return 0.5 * SHConstants::Ln2Pi + (Z - 0.5) * log(Z) - Z +
                    SHConstants::A1 / (Z + SHConstants::A2 / (Z + SHConstants::A3 / (Z +
                    SHConstants::A4 / (Z + SHConstants::A5 / (Z + SHConstants::A6 / (Z +
                    SHConstants::A7 / (Z + SHConstants::A8 / (Z + SHConstants::A9 / Z))))))));
            }

            // brute force factorial, only limited by data structure precision
            // factorial: x! = x * (x - 1) * (x - 2) * ... * 1
            // by definition 0! = 1
            const double Factorial(uint32_t x)
            {
                if (x == 0)
                {
                    return 1.0;
                }

                double result = x;
                while (--x > 0)
                {
                    result *= x;
                }
                return result;
            }

            // brute force double factorial, only limited by data structure precision
            // double factorial is factorial with step = 2: x!! = x * (x - 2) * (x - 4) * ... * 1
            // by definition both 0!! and -1!! = 1
            const double DoubleFactorial(uint32_t x)
            {
                if (x == 0 || x == -1)
                {
                    return 1.0;
                }

                double result = x;
                while ((x -= 2) > 0)
                {
                    result *= x;
                }
                return result;
            }

            // K(l, m) * P(l, m)
            // where:
            //      K is normalization factor that K = sqrt( ((2*l + 1) / (4pi)) ((l - |m|)! / (l + m)!) )
            //      P is real associated legendre polynomial at x with band l, order m
            // Please see http://silviojemma.com/public/papers/lighting/spherical-harmonic-lighting.pdf page 11, equation 6 for more information
            double KP(int l, int m, double x, bool mode)
            {
                // kp <=> K(l, m) * (2m - 1)!!
                double kp = 0.0;

                if (mode)
                {
                    // Brute force mode, only limited by floating point precision (support ~620 bands at most for double)

                    kp = sqrt((2.0 * l + 1.0) * SHConstants::Inv4Pi * Factorial(l - m) / Factorial(l + m)) *
                        DoubleFactorial(2 * m - 1);
                }
                else
                {
                    double f = 0.0;
                    if (m > 0)
                    {

                        // Stieltjes's approximation mode
                        // guarantee at least 13 valid digits for final returned value during test
                        // (tested up to band 200 due to precision limitations (limited by exp function valid input) )

                        // put factorials as close as possible to prevent explosion during computation
                        // t1 = ln((l - m)!), ln apprears because it's actually a form of gamma function    
                        // same for t2, t3, t4
                        double t1 = FactorialStieltjesNoExp(l - m);
                        double t2 = FactorialStieltjesNoExp(2 * m - 1);
                        double t3 = FactorialStieltjesNoExp(l + m);
                        double t4 = FactorialStieltjesNoExp(m - 1);

                        // f <=> ((l - m)! (2m - 1)! (2m - 1)!) / ((l + m)! (m - 1)! (m - 1)! 2^(m-1) 2^(m-1))
                        //   <=> ((l - m)! (2m - 1)!! (2m - 1)!!) / ((l + m)!!)
                        f = exp2(2 - 2 * m) * exp(t1 + 2.0 * t2 - t3 - 2.0 * t4);
                    }
                    else
                    {
                        // skip calculation for zonal harmonics (m = 0), because all factorial terms cancelled out
                        f = 1.0;
                    }

                    kp = sqrt((2.0 * l + 1.0) * SHConstants::Inv4Pi * f);
                }

                // (-1)^(m): 1 if m is even, -1 if m is odd
                double fact = 1.0 - 2.0 * (m % 2);

                // P of band m, order m, base case
                double Pmm = fact * kp * pow(1 - x * x, m / 2.0);

                if (l == m)
                {
                    return Pmm;
                }

                // P of band m+1, order m, lift band by 1
                double Pmmp1 = x * (2.0 * m + 1.0) * Pmm;
                if (l == m + 1)
                {
                    return Pmmp1;
                }

                // P of band m-1
                double Pml = 0.0;

                // first two order has been calculated by previous expressions
                for (int ll = m + 2; ll <= l; ++ll)
                {
                    Pml = ((2.0 * ll - 1.0) * x * Pmmp1 - (ll + m - 1.0) * Pmm) / (ll - m);
                    Pmm = Pmmp1;
                    Pmmp1 = Pml;
                }
                return Pml;
            }

            // K(l, m) * P(l, m)
            // where:
            //      K is normalization factor that K = sqrt( ((2*l + 1) / (4pi)) ((l - |m|)! / (l + m)!) )
            //      P is real associated legendre polynomial at x with band l, order m
            // This version uses look up table to handle factorial, thus supported number of band is limited
            // Please see http://silviojemma.com/public/papers/lighting/spherical-harmonic-lighting.pdf page 11, equation 6 for more information
            double KPLUT(const int l, const int m, double x)
            {
                // computing K(l, m) * (2m - 1)!!, which requires higher precision due to factorial
                double Kterm2 = SHConstants::FactorialLUT[l - m] * (SHConstants::DoubleFactorialLUT[m] *
                    (SHConstants::DoubleFactorialLUT[m] / SHConstants::FactorialLUT[l + m]));
                double Kterm1 = (2.0 * l + 1.0) * SHConstants::Inv4Pi;
                double kp = sqrt(Kterm1 * Kterm2);

                // (-1)^(m): 1 if m is even, -1 if m is odd
                double fact = 1.0 - 2.0 * (m % 2);
                // P of band m, order m, base case
                double Pmm = fact * kp * pow(1.0 - x * x, m / 2.0);

                if (l == m)
                {
                    return Pmm;
                }

                // P of band m+1, order m, lift band by 1
                double Pmmp1 = x * (2.0 * m + 1.0) * Pmm;
                if (l == m + 1)
                {
                    return Pmmp1;
                }

                // P of band m-1
                double Pml = 0.0;

                // first two order has been calculated by previous expressions
                for (int ll = m + 2; ll <= l; ++ll)
                {
                    Pml = ((2.0 * ll - 1.0) * x * Pmmp1 - (ll + m - 1.0) * Pmm) / (ll - m);
                    Pmm = Pmmp1;
                    Pmmp1 = Pml;
                }
                return Pml;
            }

            // Naive solver evaluates SH by definition
            // up to 17 bands (0-16) due to size of LUT
            // (could go beyond 16 but behavious are unpredicted)
            // equation slightly reorganized to mitigate precision problem
            // input:
            //  l -> SH band, non negative integer <= 16
            //  m -> SH order, integer within range [-l, l], invalid order will result in unpredicted behaviour
            //dir -> cartesian coordinates (Y-up, -Z-forward) of direction on unit sphere, required to be normalized
            double Naive16(const int l, const int m, const float dir[3])
            {
                double cosTheta = dir[1];
                double phi = atan2(dir[2], dir[0]);

                if (m == 0)
                {
                    return KPLUT(l, 0, cosTheta);
                }
                else if (m > 0)
                {
                    return SHConstants::Sqrt2 * cos(m * phi)  * KPLUT(l, m, cosTheta);
                }
                else
                {
                    return SHConstants::Sqrt2 * sin(-m * phi) * KPLUT(l, -m, cosTheta);
                }
            }

            // Extended naive solver
            // input:
            //   l -> SH band, non negative integer
            //   m -> SH order, integer within range [-l, l], invalid order will result in unpredicted behaviour
            // dir -> cartesian coordinates (Y-up, -Z-forward) of direction on unit sphere, required to be normalized
            //mode -> 0 approximation mode (use approximated factorial, O(1) performance),
            //        1 brute force   mode (evaluate factorial brute forcely), useful for reference
            double NaiveEx(const int l, const int m, const float dir[3], const bool mode)
            {
                double cosTheta = dir[1];
                double phi = atan2(dir[2], dir[0]);

                if (m == 0)
                {
                    return KP(l, 0, cosTheta, mode);
                }
                else if (m > 0)
                {
                    return SHConstants::Sqrt2 * cos(m * phi)  * KP(l, m, cosTheta, mode);
                }
                else
                {
                    return SHConstants::Sqrt2 * sin(-m * phi) * KP(l, -m, cosTheta, mode);
                }
            }
        } // namespace SHBasis

        // provided set of functions for rotating SH coefficients
        namespace SHRotation
        {
            // Fast evaluation for first 3 bands
            // input:
            //      R  -> flatten row major 3x3 rotation matrix, for example:
            //            { ux, vx, wx, uy, vy, wy, uz, vz, wz }
            // maxBand -> maximum band index to be rotated, e.g. 2 for 3 band rotation (0, 1, 2)
            //    inSH -> input  SH coefficient array, should contain at least (maxBand + 1)^2 elements
            //   outSH -> output SH coefficient array
            void EvalSHRotationFast(const float R[9], const uint32_t maxBand, const float* inSH, float* outSH)
            {
                if (maxBand >= 0 && maxBand <= 2)
                {
                    ZHF3(R, maxBand, inSH, outSH);
                }
            }

            // Naive implamentation of Wigner D matrix for SH rotation that supports higher bands
            // input:
            //      R  -> flatten row major 3x3 rotation matrix, for example:
            //            { ux, vx, wx, uy, vy, wy, uz, vz, wz }
            // maxBand -> maximum band index to be rotated, e.g. 2 for 3 band rotation (0, 1, 2)
            //    inSH -> input  SH coefficient array, should contain at least (maxBand + 1)^2 elements
            //   outSH -> output SH coefficient array
            void EvalSHRotation(const float R[9], const uint32_t maxBand, const double* inSH, double* outSH)
            {
                if (maxBand >= 0)
                {
                    WignerD(R, maxBand, inSH, outSH);
                }
            }

            // Fast evaluation for first 3 bands
            // input:
            //      R  -> flatten row major 3x3 rotation matrix, for example:
            //            { ux, vx, wx, uy, vy, wy, uz, vz, wz }
            // maxBand -> maximum band index to be rotated, e.g. 2 for 3 band rotation (0, 1, 2)
            //    inSH -> input  SH coefficient array, should contain at least (maxBand + 1)^2 elements
            //   outSH -> output SH coefficient array
            void ZHF3(const float R[9], const uint32_t maxBand, const float* inSH, float* outSH)
            {
                switch (maxBand)
                {
                    // band 2
                case 2: {

                    // invAX = invA dot x,
                    // where:
                    //    x    is the column vector contains SH coefficients of band 2 (5 in total)
                    //    invA is the inverse matrix of A, each column of A contains 5 band-2 SH coefficients ("C") for each
                    //         axis of carefully chosen basis(to ensure A is invertible):
                    //         A = {C2-2(N0), C2-2(N1), C2-2(N2), C2-2(N3), C2-2(N4),
                    //              C2-1(N0), C2-1(N1), C2-1(N2), C2-1(N3), C2-1(N4),
                    //              C2_0(N0), C2_0(N1), C2_0(N2), C2_0(N3), C2_0(N4),
                    //              C2_1(N0), C2_1(N1), C2_1(N2), C2_1(N3), C2_1(N4),
                    //              C2_2(N0), C2_2(N1), C2_2(N2), C2_2(N3), C2_2(N4),
                    //         where:
                    //            N0 = (1, 0, 0), N1 = (0, 0, 1), N2 = (1/sqrt(2), 1/sqrt(2), 0),
                    //            N3 = (1/sqrt(2), 0, 1/sqrt(2)), N4 = (0, 1/sqrt(2), 1/sqrt(2))
                    // below is an expended verison of above dot product that 0-valued elements are omitted
                    float invAX0 =  inSH[7] + inSH[8] + inSH[8] - inSH[5];
                    float invAX1 =  inSH[4] + SHConstants::Sqrt3 * inSH[6] + inSH[7] + inSH[8];
                    float invAX2 =  inSH[4];
                    float invAX3 = -inSH[7];
                    float invAX4 = -inSH[5];

                    // basis (N0-4) rotated by R
                    // e.g. (r0x, r0y, r0z) = R dot N0, same for rest vectors
                    float r0x = R[0];        float r0y = R[3];        float r0z = R[6];
                    float r1x = R[2];        float r1y = R[5];        float r1z = R[8];
                    float r2x = R[0] + R[1]; float r2y = R[3] + R[4]; float r2z = R[6] + R[7];
                    float r3x = R[0] + R[2]; float r3y = R[3] + R[5]; float r3z = R[6] + R[8];
                    float r4x = R[1] + R[2]; float r4y = R[4] + R[5]; float r4z = R[7] + R[8];

                    // shortcuts to avoid duplicate multiplications
                    float invAX0X = invAX0 * r0x;
                    float invAX0Y = invAX0 * r0y;

                    float invAX1X = invAX1 * r1x;
                    float invAX1Y = invAX1 * r1y;

                    float invAX2X = invAX2 * r2x;
                    float invAX2Y = invAX2 * r2y;

                    float invAX3X = invAX3 * r3x;
                    float invAX3Y = invAX3 * r3y;

                    float invAX4X = invAX4 * r4x;
                    float invAX4Y = invAX4 * r4y;

                    // c = 2 / 3, k = 1 / 3, N2-4 uses 2 / 3 to
                    // cancel out constant factor 2 that originally in front of invAX2-4
                    float c = SHConstants::C2Div3;
                    float k = SHConstants::C1Div3;
                    // refers to original blog:
                    // R dot x = C_RN  * invA * x = C_RN * (invAX)
                    // where
                    //    C_RN is the dense matrix that each column contains 5 coefficients for rotated basis N0-4
                    //         assume rotated N0-4 is RN0-4 (R dot N0, R dot N1...), then C_RN can be expressed as:
                    //         C_RN = {C2-2(RN0) ... C2-2(RN4),
                    //                   .               .
                    //                   .               .
                    //                   .               .
                    //                 C2_2(RN0) ... C2_2(RN4)}, which is similar to A
                    // below is an expanded version of above matrix-vector dot product
                    outSH[4] =   invAX0X* r0y              +  invAX1X* r1y              +  invAX2X* r2y              +  invAX3X* r3y              +  invAX4X* r4y;
                    outSH[5] = -(invAX0Y* r0z              +  invAX1Y* r1z              +  invAX2Y* r2z              +  invAX3Y* r3z              +  invAX4Y* r4z);
                    outSH[6] =   invAX0 *(r0z*r0z-k)       +  invAX1 *(r1z*r1z-k)       +  invAX2 *(r2z*r2z-c)       +  invAX3 *(r3z*r3z-c)       +  invAX4 *(r4z*r4z-c);
                    outSH[7] = -(invAX0X* r0z              +  invAX1X* r1z              +  invAX2X* r2z              +  invAX3X* r3z              +  invAX4X* r4z);
                    outSH[8] =  (invAX0X* r0x-invAX0Y*r0y) + (invAX1X* r1x-invAX1Y*r1y) + (invAX2X* r2x-invAX2Y*r2y) + (invAX3X*r3x-invAX3Y*r3y)  + (invAX4X*r4x-invAX4Y*r4y);

                    outSH[6] *= SHConstants::Sqrt3Div2;
                    outSH[8] *= 0.5;
                }

                // band 1
                // derived from the same process as band 2, using basis
                // N0 = (1, 0, 0), N1 = (0, 1, 0), N2 = (0, 0, 1)
                case 1: {
                    outSH[1] =  R[3] * inSH[3] + R[4] * inSH[1] - R[5] * inSH[2];
                    outSH[2] = -R[6] * inSH[3] - R[7] * inSH[1] + R[8] * inSH[2];
                    outSH[3] =  R[0] * inSH[3] + R[1] * inSH[1] - R[2] * inSH[2];
                }

                // band 0
                // first band unaffected by rotation, constant on all directions
                case 0: outSH[0] = inSH[0]; break;
                }
            }

            // implementing last row of TABLE 2 in:
            //    https://pubs.acs.org/doi/pdf/10.1021/jp9833350
            // "Rotation Matrices for Real Spherical Harmonics. Direct Determination by Recursion: Addition and Corrections" by Ivanic J. and Ruedenberg K.
            // It's a free correction of original publication in 1996, which protected by copyright thus not freely available
            double PR(int i, int l, int a, int b, std::vector<std::vector<std::vector<double>>>& R)
            {
                // using elements in band 1's rotation matrix
                double rim1 = R[1][i + 1][0];
                double ri0  = R[1][i + 1][1];
                double ri1  = R[1][i + 1][2];

                int lm1 = l - 1;
                if (b == -l)
                {
                    return ri1 * R[lm1][a + lm1][0] + rim1 * R[lm1][a + lm1][2 * lm1];
                }
                else if (b == l)
                {
                    return ri1 * R[lm1][a + lm1][2 * lm1] - rim1 * R[lm1][a + lm1][0];
                }
                else
                {
                    return ri0 * R[lm1][a + lm1][b + lm1];
                }
            }

            // implementing first row of TABLE 2 in the same resource above
            double U(int l, int m, int n, std::vector<std::vector<std::vector<double>>>& R)
            {
                return PR(0, l, m, n, R);
            }

            // implementing second row of TABLE 2 in the same resource above
            double V(int l, int m, int n, std::vector<std::vector<std::vector<double>>>& R)
            {
                double p0 = 0.0;
                double p1 = 0.0;
                bool d = false;
                if (m == 0)
                {
                    p0 = PR(1, l, 1, n, R);
                    p1 = PR(-1, l, -1, n, R);
                    return p0 + p1;
                }
                else if (m > 0.0)
                {
                    d = (m == 1);
                    p0 = PR(1, l, m - 1, n, R);
                    p1 = PR(-1, l, -m + 1, n, R);
                    return p0 * sqrt(1 + d) - p1 * (1 - d);
                }
                else
                {
                    d = (m == -1);
                    p0 = PR(1, l, m + 1, n, R);
                    p1 = PR(-1, l, -m - 1, n, R);

                    // it's a mistake in the original paper, even in the corrected version
                    // this term should be sqrt(1 + d) instead of sqrt(1 - d)
                    return p0 * (1 - d) + p1 * sqrt(1 + d);
                }
            }

            // implementing third row of TABLE 2 in the same resource above
            double W(int l, int m, int n, std::vector<std::vector<std::vector<double>>>& R)
            {
                double p0 = 0.0;
                double p1 = 0.0;
                if (m == 0.0)
                {
                    // this case will be filtered out by zero coefficient, so donesn't matter which value is returned
                    return 0.0;
                }
                else if (m > 0.0)
                {
                    p0 = PR(1, l, m + 1, n, R);
                    p1 = PR(-1, l, -m - 1, n, R);
                    return p0 + p1;
                }
                else
                {
                    p0 = PR(1, l, m - 1, n, R);
                    p1 = PR(-1, l, -m + 1, n, R);
                    return p0 - p1;
                }
            }

            // Naive implamentation of Wigner D matrix for SH rotation that supports higher bands
            // input:
            //      R  -> flatten row major 3x3 rotation matrix, for example:
            //            { ux, vx, wx, uy, vy, wy, uz, vz, wz }
            // maxBand -> maximum band index to be rotated, e.g. 2 for 3 band rotation (0, 1, 2)
            //    inSH -> input  SH coefficient array, should contain at least (maxBand + 1)^2 elements
            //   outSH -> output SH coefficient array
            void WignerD(const float R[9], const uint32_t maxBand, const double* inSH, double* outSH)
            {
                using namespace std;

                vector<vector<vector<double>>> Rot;
                for (uint32_t l = 0; l < maxBand + 1; ++l)
                {
                    // create (2l + 1)x(2l + 1) rotation matrix for each band
                    Rot.push_back(vector<vector<double>>(2 * l + 1, vector<double>(2 *l + 1, 0.0)));
                }

                // band 0
                Rot[0][0][0] = 1.0;

                // band 1, also base case R in last row of TABLE 2 in the original resource
                // added Condon-Shortely phase function: (-1)^m to match the standard used in graphics
                Rot[1][0][0] =  R[4];
                Rot[1][0][1] = -R[5];
                Rot[1][0][2] =  R[3];
                Rot[1][1][0] = -R[7];
                Rot[1][1][1] =  R[8];
                Rot[1][1][2] = -R[6];
                Rot[1][2][0] =  R[1];
                Rot[1][2][1] = -R[2];
                Rot[1][2][2] =  R[0];

                // calculation rotation matrix for each band
                for (int l = 2; l <= int(maxBand); ++l)
                {
                    // solve all m x n elements in band l's matrix
                    for (int m = -l; m <= l; ++m)
                    {
                        for (int n = -l; n <= l; ++n)
                        {
                            // delta function d_m0: 1 (if m == 0); 0 (otherwise)
                            bool d = (m == 0);
                            double denominator = (abs(n) == l) ? (2 * l) * (2 * l - 1) : (l + n) * (l - n);

                            // implementing TABLE 1 in the original resource
                            double u = sqrt(((l + m) * (l - m)) / denominator);
                            double v = sqrt(((1.0 + d) * (l + abs(m) - 1.0) * (l + abs(m))) / denominator) * (1.0 - 2.0 * d) * 0.5;
                            double w = sqrt(((l - abs(m) - 1.0) * (l - abs(m))) / denominator) * (1.0 - d) * (-0.5);

                            // the only case u,v,w will be 0 is multiplied with a 0
                            // thus it might be reasonable to use not equal here for double
                            if (u != 0.0)
                            {
                                u = u * U(l, m, n, Rot);
                            }

                            if (v != 0.0)
                            {
                                v = v * V(l, m, n, Rot);
                            }

                            if (w != 0.0)
                            {
                                w = w * W(l, m, n, Rot);
                            }

                            //record result
                            Rot[l][m + l][n + l] = u + v + w;
                        }
                    }
                }

                // rotate coefficients via dot product
                outSH[0] = inSH[0];
                for (int l = 1; l <= int(maxBand); ++l)
                {
                    // multiply entire row for each order
                    int bandStart = l * l;

                    // matrix muliplication
                    for (int m = 0; m < 2 * l + 1; ++m)
                    {
                        // i is recentered order m
                        for (int i = 0; i < (2 * l + 1); ++i)
                        {
                            outSH[bandStart + m] += (Rot[l][m][i] * inSH[bandStart + i]);
                        }
                    }
                }
            }
        } // namespace SHRotation
    } // namespace Render
} // namespace AZ
