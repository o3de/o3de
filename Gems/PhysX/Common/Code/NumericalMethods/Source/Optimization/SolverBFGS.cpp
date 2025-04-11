/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Optimization/SolverBFGS.h>
#include <Optimization/LineSearch.h>
#include <Optimization/Utilities.h>
#include <Optimization/Constants.h>

namespace NumericalMethods::Optimization
{
    SolverResult MinimizeBFGS(const Function& f, const AZStd::vector<double>& xInitial)
    {
        // using the notation from Nocedal and Wright
        // H - an approximation to the inverse of the Hessian (matrix of second derivatives)
        // s - the difference between the function value this iteration and the previous iteration
        // y - the difference between the function gradient this iteration and the previous iteration

        SolverResult result;
        const AZ::u32 dimension = static_cast<AZ::u32>(xInitial.size());
        VectorVariable searchDirection(dimension);

        MatrixVariable H(dimension, dimension);
        MatrixVariable I(dimension, dimension);
        for (AZ::u32 i = 0; i < dimension; i++)
        {
            H.Element(i, i) = 1.0;
            I.Element(i, i) = 1.0;
        }

        VectorVariable x = VectorVariable::CreateFromVector(xInitial);
        double f_x = FunctionValue(f, x);
        for (; result.m_iterations < solverIterations; ++result.m_iterations)
        {
            // stop if the gradient is small enough
            VectorVariable gradient = Gradient(f, x);
            if (gradient.Norm() < gradientTolerance)
            {
                result.m_outcome = SolverOutcome::Success;
                result.m_xValues = x.GetValues();
                return result;
            }

            // find a search direction based on the Hessian and gradient and then search for an appropriate step size in
            // that direction
            searchDirection = -(H * gradient);
            LineSearchResult lineSearchResult = LineSearchWolfe(f, x, f_x, searchDirection);
            if (IsFailure(lineSearchResult))
            {
                result.m_outcome = SolverOutcome::Incomplete;
                result.m_xValues = x.GetValues();
                return result;
            }
            VectorVariable s = lineSearchResult.m_stepSize * searchDirection;
            x += s;
            f_x = lineSearchResult.m_functionValue;
            VectorVariable y = Gradient(f, x) - gradient;

            // on the first iteration, use a heuristic to scale the Hessian
            if (result.m_iterations == 0)
            {
                double scale = y.Dot(s) / y.Dot(y);
                for (AZ::u32 i = 0; i < dimension; i++)
                {
                    H.Element(i, i) = scale;
                }
            }

            // update the approximate inverse Hessian using the BFGS formula (see Nocedal and Wright)
            double rho = 1.0 / y.Dot(s);
            H = (I - rho * OuterProduct(s, y)) * H * (I - rho * OuterProduct(y, s)) + rho * OuterProduct(s, s);
        }

        result.m_outcome = SolverOutcome::MaxIterations;
        result.m_xValues = x.GetValues();
        return result;
    }
} // namespace NumericalMethods::Optimization
