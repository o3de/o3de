/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <NumericalMethods/Eigenanalysis.h>

namespace NumericalMethods::Eigenanalysis
{
    //! Finds the eigenvalues and vectors of the symmetric matrix whose unique elements are given (see Eberly).
    //! @param a<ij> The element of the matrix in row i, column j.
    //! @return Orthonormal eigenbasis of the matrix and the corresponding eigenvalues.
    SolverResult<Real, 3> NonIterativeSymmetricEigensolver3x3(
        double a00, double a01, double a02,
                    double a11, double a12,
                                double a22
    );
} // namespace NumericalMethods::Eigenanalysis
