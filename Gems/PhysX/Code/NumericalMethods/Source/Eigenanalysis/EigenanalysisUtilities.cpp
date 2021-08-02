/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <cmath>

#include <AzCore/std/algorithm.h>

#include <LinearAlgebra.h>
#include <Eigenanalysis/Utilities.h>

namespace NumericalMethods::Eigenanalysis
{
    VectorVariable CrossProduct(const VectorVariable& lhs, const VectorVariable& rhs)
    {
        AZ_Assert(
            lhs.GetDimension() == 3 && rhs.GetDimension() == 3, "VectorVariable dimensions invalid for cross product."
        );

        return VectorVariable::CreateFromVector({
            lhs[1] * rhs[2] - lhs[2] * rhs[1],
            lhs[2] * rhs[0] - lhs[0] * rhs[2],
            lhs[0] * rhs[1] - lhs[1] * rhs[0]
        });
    }

    void ComputeOrthogonalComplement(
        const VectorVariable& vecW, VectorVariable& vecU, VectorVariable& vecV
    )
    {
        // Robustly computes a right-handed orthogonal basis {vecU, vecV, vecW}.
        double invLength = 1.0;

        if (fabs(vecW[0]) > fabs(vecW[1]))
        {
            // The component of maximum absolute value is either vecW[0] or vecW[2].
            invLength /= sqrt(vecW[0] * vecW[0] + vecW[2] * vecW[2]);
            vecU = VectorVariable::CreateFromVector({ -vecW[2] * invLength, 0.0, vecW[0] * invLength });
        }
        else
        {
            // The component of maximum absolute value is either vecW[1] or vecW[2].
            invLength /= sqrt(vecW[1] * vecW[1] + vecW[2] * vecW[2]);
            vecU = VectorVariable::CreateFromVector({ 0.0, vecW[2] * invLength, -vecW[1] * invLength });
        }

        vecV = CrossProduct(vecW, vecU);
    }

    VectorVariable ComputeEigenvector0(
        double a00, double a01, double a02, double a11, double a12, double a22, double val
    )
    {
        // By definition, (A-e*I)v = 0, where e is the eigenvalue and v is the corresponding eigenvector to be found.
        // This condition implies that the rows (A-e*I) must be perpendicular to v. This matrix must have rank 2, so two
        // rows will be linearly dependent. For those two rows, the cross product will be (nearly) zero. So to find v,
        // we can simply take the cross product of the two rows that maximize its magnitude.
        VectorVariable row0 = VectorVariable::CreateFromVector({ a00 - val, a01, a02 });
        VectorVariable row1 = VectorVariable::CreateFromVector({ a01, a11 - val, a12 });
        VectorVariable row2 = VectorVariable::CreateFromVector({ a02, a12, a22 - val });

        VectorVariable r0xr1 = CrossProduct(row0, row1);
        VectorVariable r0xr2 = CrossProduct(row0, row2);
        VectorVariable r1xr2 = CrossProduct(row1, row2);

        double d0 = r0xr1.Dot(r0xr1);
        double d1 = r0xr2.Dot(r0xr2);
        double d2 = r1xr2.Dot(r1xr2);

        return d0 >= d1 && d0 >= d2 ? r0xr1 * (1.0 / sqrt(d0)) :
               d1 >= d0 && d1 >= d2 ? r0xr2 * (1.0 / sqrt(d1)) :
                                      r1xr2 * (1.0 / sqrt(d2)) ;
    }

    VectorVariable ComputeEigenvector1(
        double a00,
        double a01,
        double a02,
        double a11,
        double a12,
        double a22,
        double val,
        const VectorVariable& vec
    )
    {
        // Real symmetric matrices must have orthogonal eigenvectors. Thus, if we generate two vectors vecU and vecV
        // orthogonal to the eigenvector vec already found, the remaining eigenvectors must be a circular combination
        // of vecU and vecW. This reduces the problem to a 2D system. For details see Eberly.
        VectorVariable vecU(3);
        VectorVariable vecV(3);
        ComputeOrthogonalComplement(vec, vecU, vecV);

        MatrixVariable matA(3, 3);

        matA.Element(0, 0) = a00;
        matA.Element(0, 1) = a01;
        matA.Element(0, 2) = a02;

        matA.Element(1, 0) = a01;
        matA.Element(1, 1) = a11;
        matA.Element(1, 2) = a12;

        matA.Element(2, 0) = a02;
        matA.Element(2, 1) = a12;
        matA.Element(2, 2) = a22;

        double m00 = vecU.Dot(matA * vecU) - val;
        double absM00 = fabs(m00);

        double m01 = vecU.Dot(matA * vecV);
        double absM01 = fabs(m01);

        double m11 = vecV.Dot(matA * vecV) - val;
        double absM11 = fabs(m11);

        auto discardComponentAndNormalize = [](double& factor, double& other) {
            other /= factor;
            factor = 1.0 / sqrt(1.0 + other * other);
            other *= factor;
        };

        if (absM00 > absM11)
        {
            if (AZStd::max(absM00, absM01) > 0.0)
            {
                if (absM00 >= absM01)
                {
                    discardComponentAndNormalize(m00, m01);
                }
                else
                {
                    discardComponentAndNormalize(m01, m00);
                }
                return vecU * m01 - vecV * m00;
            }
            else
            {
                return vecU;
            }
        }
        else
        {
            if (AZStd::max(absM11, absM01) > 0.0)
            {
                if (absM11 >= absM01)
                {
                    discardComponentAndNormalize(m11, m01);
                }
                else
                {
                    discardComponentAndNormalize(m01, m11);
                }
                return vecU * m11 - vecV * m01;
            }
            else
            {
                return vecU;
            }
        }
    }

    VectorVariable ComputeEigenvector2(const VectorVariable& vec0, const VectorVariable& vec1)
    {
        return CrossProduct(vec0, vec1);
    }
} // namespace NumericalMethods::Eigenanalysis
