/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <stdint.h>

namespace AZ
{
    namespace Render
    {
        // provided set of function for evaluating spherical harmonic basis functions
        namespace SHBasis
        {
            // input dir[3] is the direction (normalized) to evaluate SH basis in cartesian coordinates
            // Y-up, -Z-forward axis system is assumed (but will be converted to z-up internally)

            // LXMXs are separate functions for SH basis in band L, order M for potential unrolled calculations
            // the corresponding polynomial funtion is available at:
            //     https://en.wikipedia.org/wiki/Table_of_spherical_harmonics, note the first part is complex SH, last part is actaully the real SH used
            float L0M0 (const float dir[3]);

            float L1MN1(const float dir[3]);
            float L1M0 (const float dir[3]);
            float L1MP1(const float dir[3]);

            float L2MN2(const float dir[3]);
            float L2MN1(const float dir[3]);
            float L2M0 (const float dir[3]);
            float L2MP1(const float dir[3]);
            float L2MP2(const float dir[3]);

            float L3MN3(const float dir[3]);
            float L3MN2(const float dir[3]);
            float L3MN1(const float dir[3]);
            float L3M0 (const float dir[3]);
            float L3MP1(const float dir[3]);
            float L3MP2(const float dir[3]);
            float L3MP3(const float dir[3]);

            // Polynomial solver evaluates first 4 bands (0-3) via analytical polynomial form
            float  Poly3(const int l, const int m, const float dir[3]);

            // Naive solver evaluates SH by definition
            // up to 17 bands (0-16) due to size of LUT
            // (could go beyond 16 but behavious are unpredicted)
            // equation slightly reorganized to mitigate precision problem
            double Naive16(const int l, const int m, const float dir[3]);

            // Extended naive solver that evaluate factorial actually, support a
            // faster but lower precision approximation mode compute factorial via gamma function, and a
            // slower but higher precision brute force mode compute factorial via while loop
            double NaiveEx(const int l, const int m, const float dir[3], const bool mode = 0);

            // wrappers for callers who don't mind which solver to use
            float  EvalSHBasisFast(const int l, const int m, const float dir[3]);
            double EvalSHBasis(const int l, const int m, const float dir[3]);
        }

        namespace SHRotation
        {
            // Implementation based on the combinantion of Zonal Harmonics Factorization and
            // rotation invariant property, based on:
            //   http://filmicworlds.com/blog/simple-and-fast-spherical-harmonic-rotation/
            // only requires 57 multiplicatios for 3 band rotation, don't need advance math operations,
            // but the dense matrix multiplication in the final step can severely affect performance for higher band,
            // thus lack of flexibility to be extended beyond first few bands.
            void ZHF3(const float R[9], const uint32_t maxBand, const float* inSH, float* outSH);

            // Naive Wigner D diagonal block matrix implementation, base on resursive process proposed in :
            //  "Rotation Matrices for Real Spherical Harmonics. Direct Determination by Recursion" Ivanic J. Ruedenberg K. 1996
            // this function a direct implementation of equations mentioned in the paper, also partially refers to
            // Appendix 1 in http://silviojemma.com/public/papers/lighting/spherical-harmonic-lighting.pdf by Green R. 2003
            // theoretically support arbitrary number of bands, in practice might bounded by available memory and data precision
            void WignerD(const float R[9], const uint32_t maxBand, const double* inSH, double* outSH);

            // wrappers for callers who don't mind which solver to use
            void EvalSHRotationFast(const float R[9], const uint32_t maxBand, const float* inSH, float* outSH);
            void EvalSHRotation(const float R[9], const uint32_t maxBand, const double* inSH, double* outSH);
        }
    } // namespace Render
} // namespace AZ

#include "SphericalHarmonicsUtility.inl" // include implementation
