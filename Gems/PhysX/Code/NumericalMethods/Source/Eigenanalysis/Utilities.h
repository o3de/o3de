/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace NumericalMethods
{
    class VectorVariable;

    namespace Eigenanalysis
    {
        //! Compute the cross product between two 3D vectors.
        //! @param lhs The left-hand side vector.
        //! @param rhs The right-hand side vector.
        //! @return The 3D vector equal to the cross product if both input vectors are 3-dimensional.
        VectorVariable CrossProduct(const VectorVariable& lhs, const VectorVariable& rhs);

        //! Robustly computes a right-handed orthonormal basis containing a given unit-length 3D input vector.
        //! @param vecW[in] A 3D input vector which must be of unit length.
        //! @param vecU[out] The first of the computed unit-length orthogonal vectors.
        //! @param vecV[out] The second of the computed unit-length orthogonal vectors.
        //! @return {vecU, vecV, vecW} will be a right-handed orthogonal set.
        void ComputeOrthogonalComplement(const VectorVariable& vecW, VectorVariable& vecU, VectorVariable& vecV);

        //! Given elements of a symmetric 3x3 matrix and one of its eigenvalues, computes the corresponding eigenvector.
        //! For numerical stability, this function should only be used to find the eigenvector corresponding to
        //! eigenvalues that are unique and numerically not close to other eigenvalues.
        //! @param a<ij> The element of the matrix in row i, column j.
        //! @param val One of the eigenvalues of the matrix.
        //! @return The corresponding eigenvector.
        VectorVariable ComputeEigenvector0(
            double a00, double a01, double a02, double a11, double a12, double a22, double val
        );

        //! Given elements of a symmetric 3x3 matrix, one of its eigenvalues and an unrelated eigenvector, computes the
        //! eigenvector corresponding to the eigenvalue.
        //! This algorithm is numerically stable even if the eigenvalue is repeated.
        //! @param a<ij> The element of the matrix in row i, column j.
        //! @param val The eigenvalue whose corresponding eigenvector is to be found.
        //! @param vec The unrelated eigenvector that is already known.
        //! @return The eigenvector corresponding to the given eigenvalue.
        VectorVariable ComputeEigenvector1(
            double a00,
            double a01,
            double a02,
            double a11,
            double a12,
            double a22,
            double val,
            const VectorVariable& vec
        );

        // Given two eigenvectors of a symmetric 3x3 matrix, computes the third.
        // The third eigenvector is found by taking the cross product of the known eigenvectors (the eigenvectors of a
        // real symmetric 3x3 matrix are always orthogonal).
        //! @param vec0 The first of the already known eigenvectors.
        //! @param vec1 The second of the already known eigenvectors.
        //! @return The computed eigenvector.
        VectorVariable ComputeEigenvector2(const VectorVariable& vec0, const VectorVariable& vec1);
    } // namespace Eigenanalysis
} // namespace NumericalMethods
