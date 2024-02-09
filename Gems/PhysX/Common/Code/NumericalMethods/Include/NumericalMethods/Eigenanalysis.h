/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <complex>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/array.h>

namespace NumericalMethods
{
    //! Namespace for methods for finding eigenvalues and unit-length eigenvectors of matrices.
    //! The 3x3 symmetric eigensolver algorithm was adapted from <a href="
    //! https://www.geometrictools.com/Documentation/RobustEigenSymmetric3x3.pdf">A Robust Eigensolver for 3x3 Symmetric
    //! Matrices</a> by Eberly.
    namespace Eigenanalysis
    {
        //! Real scalar type used by requests within this interface.
        using Real = double;

        //! Complex scalar type used by requests within this interface.
        using Complex = std::complex<double>;

        //! Struct for holding a single eigenvalue/eigenvector pair.
        //! The template parameter defines the dimensions of the eigenvector.
        template <typename Scalar, unsigned int D>
        struct Eigenpair
        {
            Scalar m_value; //<! An eigenvalue.
            AZStd::array<Scalar, D> m_vector; //<! An array of components representing the corresponding eigenvector.
        };

        //! Class used to represent square matrices to pass to the eigensolver requests.
        //! The template parameter defines the dimensions of the matrix.
        template <typename Scalar, unsigned int D>
        class SquareMatrix
        {
        public:
            using Row = AZStd::array<Scalar, D>; //!< Type declaration for individual rows.

            SquareMatrix() = default;

            //! Row accessor.
            //! Delegates directly to the underlying row array. No bounds checking.
            //! @return Returns a reference to a row, which itself can be dereferenced.
            Row& operator[](int row)
            {
                return m_rows[row];
            }

            //! Row accessor.
            //! Delegates directly to the underlying array. No bounds checking.
            //! @return Returns a constant reference to a row, which itself can be dereferenced.
            const Row& operator[](int row) const
            {
                return m_rows[row];
            }

            AZStd::array<Row, D> m_rows; //!< The matrix expressed in row-major form.
        };

        //! Used when returning the solver result to indicate if the solver was successful or to indicate failure
        //! reasons.
        enum class SolverOutcome
        {
            Invalid, //!< Default value to which fields of this type should be initialized.
            Success, //!< The solver successfully found the eigenvalues and vectors.
            Failure, //!< The solver failed for unspecified reasons.
            FailureInvalidInput, //!< The solver failed because the input matrix was not valid.
        };

        //! Struct for holding both the result of the eigenanalysis and the qualitative outcome i.e. success or failure.
        //! The template parameter defines the dimensions of the eigenvectors.
        template <typename Scalar, unsigned int D>
        struct SolverResult
        {
            //! Indicates whether the solver completed successfully or failed.
            SolverOutcome m_outcome = SolverOutcome::Invalid;

            //! A vector of eigenvalue/eigenvector pairs.
            //! This vector will contain one entry for each unit-length eigenvector of the input matrix. Their
            //! corresponding eigenvalues need not be unique.
            AZStd::vector<Eigenpair<Scalar, D>> m_eigenpairs;
        };

        //! Compute the eigenvalues and a corresponding eigenbasis for a real symmetric 3x3 matrix.
        //! The eigenvalues in this case are guaranteed to be real and the eigenbasis returned is guaranteed to be
        //! right-handed and orthonormal (within numerical precision).
        //! @param matrix The input matrix which must be real and symmetric.
        //! @return An instance of \ref SolverResult.
        //!   The \ref SolverResult::m_outcome will be set to \ref SolverOutcome::FailureInvalidInput if the given
        //!   matrix is not real and symmetric. Otherwise, \ref SolverResult::m_eigenpairs will contain an
        //!   orthonormal basis.
        SolverResult<Real, 3> Solver3x3RealSymmetric(const SquareMatrix<Real, 3>& matrix);
    } // namespace Eigenanalysis
} // namespace NumericalMethods
