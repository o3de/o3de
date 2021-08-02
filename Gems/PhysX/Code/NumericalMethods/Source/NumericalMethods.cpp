/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <NumericalMethods/Eigenanalysis.h>
#include <NumericalMethods/Optimization.h>
#include <Optimization/SolverBFGS.h>
#include <Eigenanalysis/Solver3x3.h>

namespace NumericalMethods
{
    namespace Optimization
    {
        SolverResult SolverBFGS(const Function& function, const AZStd::vector<double>& initialGuess)
        {
            return MinimizeBFGS(function, initialGuess);
        }
    }

    namespace Eigenanalysis
    {
        SolverResult<Real, 3> Solver3x3RealSymmetric(const SquareMatrix<Real, 3>& matrix)
        {
            // The matrix must be symmetric.
            if (matrix[0][1] == matrix[1][0] && matrix[0][2] == matrix[2][0] && matrix[1][2] == matrix[2][1])
            {
                return NonIterativeSymmetricEigensolver3x3(
                    matrix[0][0], matrix[0][1], matrix[0][2],
                                  matrix[1][1], matrix[1][2],
                                                matrix[2][2]
                );
            }
            else
            {
                return SolverResult<Real, 3>{SolverOutcome::FailureInvalidInput};
            }
        }
    }
} // namespace NumericalMethods
