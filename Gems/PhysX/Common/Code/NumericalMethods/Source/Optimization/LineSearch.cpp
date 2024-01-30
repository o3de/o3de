/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Optimization/LineSearch.h>
#include <Optimization/Utilities.h>
#include <Optimization/Constants.h>
#include <float.h>
#include <math.h>

namespace NumericalMethods::Optimization
{
    bool IsFailure(const LineSearchResult& result)
    {
        return result.m_outcome >= LineSearchOutcome::FailureExceededIterations;
    }

    ScalarVariable CubicMinimum(const double a, const double f_a, const double df_a, const double b, const double f_b,
        const double c, const double f_c)
    {
        double coefficients[4] = {};
        coefficients[1] = df_a;
        const double db = b - a;
        const double dc = c - a;
        const double denominator = (db * db * dc * dc) * (db - dc);
        double e[2];
        e[0] = f_b - f_a - coefficients[1] * db;
        e[1] = f_c - f_a - coefficients[1] * dc;
        coefficients[3] = (dc * dc * e[0] - db * db * e[1]) / denominator;
        coefficients[2] = (-dc * dc * dc * e[0] + db * db * db * e[1]) / denominator;
        const double radical = coefficients[2] * coefficients[2] - 3.0 * coefficients[3] * coefficients[1];

        return a + (-coefficients[2] + sqrt(radical)) / (3.0 * coefficients[3]);
    }

    ScalarVariable QuadraticMinimum(const double a, const double f_a, const double df_a, const double b, const double f_b)
    {
        double coefficients[3] = {};
        const double db = b - a;
        coefficients[1] = df_a;
        coefficients[2] = (f_b - f_a - coefficients[1] * db) / (db * db);
        return a - coefficients[1] / (2.0 * coefficients[2]);
    }

    bool ValidateStepSize(const ScalarVariable alphaNew, const double alpha0, const double alpha1, const double edgeThreshold)
    {
        const double alphaMin = AZStd::GetMin(alpha0, alpha1);
        const double alphaMax = AZStd::GetMax(alpha0, alpha1);
        const double range = alphaMax - alphaMin;
        return (azisfinite(alphaNew) && (alphaNew > alphaMin + edgeThreshold * range) &&
            (alphaNew < alphaMax - edgeThreshold * range));
    }

    LineSearchResult SelectStepSizeFromInterval(double alpha0, double alpha1, double f_alpha0, double f_alpha1,
        double df_alpha0, const Function& f, const VectorVariable& x0, const VectorVariable& searchDirection,
        const double f_x0, const double df_x0, const double c1, const double c2)
    {
        const double cubicEdgeThreshold = 0.2;
        const double quadraticEdgeThreshold = 0.1;
        double alphaLast = 0.0;
        double f_alphaLast = f_x0;
        LineSearchResult result;

        for (AZ::u32 iteration = 0; iteration < lineSearchIterations; iteration++)
        {
            ScalarVariable alphaNew = 0.0;
            if (iteration > 0)
            {
                // first try selecting a new alpha value based on cubic interpolation through the most recent points
                alphaNew = CubicMinimum(alpha0, f_alpha0, df_alpha0, alpha1, f_alpha1, alphaLast, f_alphaLast);
            }

            // if this is the first iteration, or the cubic method failed or is invalid, try a quadratic
            if (iteration == 0 || !ValidateStepSize(alphaNew, alpha0, alpha1, cubicEdgeThreshold))
            {
                alphaNew = QuadraticMinimum(alpha0, f_alpha0, df_alpha0, alpha1, f_alpha1);

                // if the quadratic is invalid, use bisection
                if (!ValidateStepSize(alphaNew, alpha0, alpha1, quadraticEdgeThreshold))
                {
                    alphaNew = ScalarVariable(0.5 * (alpha0 + alpha1));
                }
            }

            // Check if alphaNew satisfies the Wolfe conditions
            // First the sufficient decrease condition
            const double f_alphaNew = FunctionValue(f, x0 + alphaNew * searchDirection);
            if ((f_alphaNew > f_x0 + c1 * alphaNew * df_x0) || (f_alphaNew >= f_alpha0))
            {
                // The decrease is not sufficient, so set up the parameters for the next iteration
                f_alphaLast = f_alpha1;
                alphaLast = alpha1;
                alpha1 = alphaNew;
                f_alpha1 = f_alphaNew;
            }
            else
            {
                // There is sufficient decrease, so test the second Wolfe condition i.e. whether the derivative
                // corresponding to alphaNew is shallower than the derivative at x0
                double df_alphaNew = DirectionalDerivative(f, x0 + alphaNew * searchDirection, searchDirection);
                if (fabs(df_alphaNew) <= -c2 * df_x0)
                {
                    // alphaNew satisfies the Wolfe conditions, so return it
                    result.m_outcome = LineSearchOutcome::Success;
                    result.m_stepSize = alphaNew;
                    result.m_functionValue = f_alphaNew;
                    result.m_derivativeValue = df_alphaNew;
                    return result;
                }

                if (df_alphaNew * (alpha1 - alpha0) >= 0.0)
                {
                    f_alphaLast = f_alpha1;
                    alphaLast = alpha1;
                    alpha1 = alpha0;
                    f_alpha1 = f_alpha0;
                }

                else
                {
                    f_alphaLast = f_alpha0;
                    alphaLast = alpha0;
                }
                alpha0 = alphaNew;
                f_alpha0 = f_alphaNew;
                df_alpha0 = df_alphaNew;
            }
        }

        // Failed to find a conforming step size
        result.m_stepSize = 0.0;
        result.m_functionValue = 0.0;
        result.m_derivativeValue = 0.0;
        result.m_outcome = LineSearchOutcome::FailureExceededIterations;
        return result;
    }

    LineSearchResult LineSearchWolfe(const Function& f, const VectorVariable& x0, double f_x0,
        const VectorVariable& searchDirection)
    {
        // uses the notation from Nocedal and Wright, where alpha represents the step size
        // alpha0 and alpha1 are the lower and upper bounds of an interval which brackets the final value of alpha
        // initial step size of 1 is recommended for quasi-Newton methods (Nocedal and Wright)
        double alpha0 = 0.0;
        double alpha1 = 1.0;

        double f_alpha1 = FunctionValue(f, x0 + alpha1 * searchDirection);
        double f_alpha0 = f_x0;
        const double df_x0 = DirectionalDerivative(f, x0, searchDirection);
        double df_alpha0 = df_x0;

        for (AZ::u32 iteration = 0; iteration < lineSearchIterations; iteration++)
        {
            // if the value of f corresponding to alpha1 isn't sufficiently small compared to f at x0,
            // then the interval [alpha0 ... alpha1] must bracket a suitable point.
            if ((f_alpha1 > f_x0 + WolfeConditionsC1 * alpha1 * df_x0) || (iteration > 0 && f_alpha1 > f_alpha0))
            {
                return SelectStepSizeFromInterval(alpha0, alpha1, f_alpha0, f_alpha1, df_alpha0,
                    f, x0, searchDirection, f_x0, df_x0, WolfeConditionsC1, WolfeConditionsC2);
            }

            // otherwise, if the derivative corresponding to alpha1 is large enough, alpha1 already
            // satisfies the Wolfe conditions and so return alpha1.
            double df_alpha1 = DirectionalDerivative(f, x0 + alpha1 * searchDirection, searchDirection);
            if (fabs(df_alpha1) <= -WolfeConditionsC2 * df_x0)
            {
                LineSearchResult result;
                result.m_outcome = LineSearchOutcome::Success;
                result.m_stepSize = alpha1;
                result.m_functionValue = f_alpha1;
                result.m_derivativeValue = df_alpha1;
                return result;
            }

            if (df_alpha1 >= 0.0)
            {
                return SelectStepSizeFromInterval(alpha1, alpha0, f_alpha1, f_alpha0, df_alpha1,
                    f, x0, searchDirection, f_x0, df_x0, WolfeConditionsC1, WolfeConditionsC2);
            }

            // haven't found an interval which is guaranteed to bracket a suitable point,
            // so expand the search region for the next iteration
            alpha0 = alpha1;
            f_alpha0 = f_alpha1;
            alpha1 = 2.0 * alpha1;
            f_alpha1 = FunctionValue(f, x0 + alpha1 * searchDirection);
            df_alpha0 = df_alpha1;
        }

        LineSearchResult result;
        result.m_outcome = LineSearchOutcome::BestEffort;
        result.m_stepSize = alpha1;
        result.m_functionValue = f_alpha1;
        result.m_derivativeValue = DirectionalDerivative(f, x0 + alpha1 * searchDirection, searchDirection);
        return result;
    }
} // namespace NumericalMethods::Optimization
