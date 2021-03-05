/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <NumericalMethods_precompiled.h>
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
