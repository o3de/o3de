/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <algorithm>
#include <cmath>

#include <AzCore/std/algorithm.h>

#include <LinearAlgebra.h>
#include <Eigenanalysis/Solver3x3.h>
#include <Eigenanalysis/Utilities.h>

namespace NumericalMethods::Eigenanalysis
{
    SolverResult<Real, 3> NonIterativeSymmetricEigensolver3x3(
        double a00, double a01, double a02, double a11, double a12, double a22
    )
    {
        // Using the notation from Eberly:
        // A        - the symmetric input matrix
        // a<ij>    - the upper elements of the matrix (0 <= i <= j <= 2).
        // B        - a matrix derived from A, such that B = (A - q*I)/p where
        //                p = sqrt( tr( (A-q*I)^2 ) / 6 )
        //                q = tr(A) / 3
        // beta<i>  - the eigenvalues of B (0 <= i <= 2)
        // alpha<i> - the eigenvalues of A (not explicit, stored in the result) (0 <= i <= 2)

        double alpha0 = 0.0;
        double alpha1 = 0.0;
        double alpha2 = 0.0;

        VectorVariable vec0 = VectorVariable::CreateFromVector({ 1.0, 0.0, 0.0 });
        VectorVariable vec1 = VectorVariable::CreateFromVector({ 0.0, 1.0, 0.0 });
        VectorVariable vec2 = VectorVariable::CreateFromVector({ 0.0, 0.0, 1.0 });

        // Precondition the matrix by factoring out the element of biggest magnitude. This is to guard against
        // floating-point overflow/underflow.
        double maxAbsElem = std::max({fabs(a00), fabs(a01), fabs(a02), fabs(a11), fabs(a12), fabs(a22)});

        if (maxAbsElem != 0.0)
        {
            // A is not the zero matrix.
            double invMaxAbsElem = 1.0 / maxAbsElem;
            a00 *= invMaxAbsElem;
            a01 *= invMaxAbsElem;
            a02 *= invMaxAbsElem;
            a11 *= invMaxAbsElem;
            a12 *= invMaxAbsElem;
            a22 *= invMaxAbsElem;

            double norm = a01 * a01 + a02 * a02 + a12 * a12;
            if (norm > 0.0)
            {
                // Compute the eigenvalues of A. For a detailed explanation of how the algorithm works, see Eberly.
                double q = (a00 + a11 + a22) / 3.0;

                double b00 = a00 - q;
                double b11 = a11 - q;
                double b22 = a22 - q;

                double p = sqrt((b00 * b00 + b11 * b11 + b22 * b22 + norm * 2.0) / 6.0);

                double c00 = b11 * b22 - a12 * a12;
                double c01 = a01 * b22 - a12 * a02;
                double c02 = a01 * a12 - b11 * a02;
                double det = (b00 * c00 - a01 * c01 + a02 * c02) / (p * p * p);

                double halfDet = AZStd::clamp(det * 0.5, -1.0, 1.0);
                double angle = acos(halfDet) / 3.0;

                static const double twoThirdsPi = 2.09439510239319549;

                // The eigenvalues of B are ordered such that beta0 <= beta1 <= beta2.
                double beta2 = cos(angle) * 2.0;
                double beta0 = cos(angle + twoThirdsPi) * 2.0;
                double beta1 = -(beta0 + beta2);

                // The eigenvalues of A are ordered such that alpha0 <= alpha1 <= alpha2.
                alpha0 = q + p * beta0;
                alpha1 = q + p * beta1;
                alpha2 = q + p * beta2;

                // Compute the eigenvectors. We either have
                //     beta0 <= beta1 < 0 < beta2 (if halfDet >= 0); or
                //     beta0 < 0 < beta1 <= beta2 (if halfDef < 0).
                // For numerical stability, we use different approaches to compute the eigenvector corresponding to the
                // eigenvalue that is definitely not repeated and the other two.
                if (halfDet >= 0.0)
                {
                    vec2 = ComputeEigenvector0(a00, a01, a02, a11, a12, a22, alpha2);
                    vec1 = ComputeEigenvector1(a00, a01, a02, a11, a12, a22, alpha1, vec2);
                    vec0 = ComputeEigenvector2(vec1, vec2);
                }
                else
                {
                    vec0 = ComputeEigenvector0(a00, a01, a02, a11, a12, a22, alpha0);
                    vec1 = ComputeEigenvector1(a00, a01, a02, a11, a12, a22, alpha1, vec0);
                    vec2 = ComputeEigenvector2(vec0, vec1);
                }
            }
            else
            {
                // A is a diagonal matrix. The eigenvalues in this case are the elements along the main diagonal, and
                // the eigenvectors are the standard Cartesian basis vectors.
                alpha0 = a00;
                alpha1 = a11;
                alpha2 = a22;
            }

            // The scaling applied to A in the precondition scales the eigenvalues by the same amount and must be
            // reverted.
            alpha0 *= maxAbsElem;
            alpha1 *= maxAbsElem;
            alpha2 *= maxAbsElem;
        }

        return SolverResult<Real, 3>{
            SolverOutcome::Success,
            {
                Eigenpair<Real, 3>{alpha0, {{vec0[0], vec0[1], vec0[2]}}},
                Eigenpair<Real, 3>{alpha1, {{vec1[0], vec1[1], vec1[2]}}},
                Eigenpair<Real, 3>{alpha2, {{vec2[0], vec2[1], vec2[2]}}}
            }
        };
    }
} // namespace NumericalMethods::Eigenanalysis
