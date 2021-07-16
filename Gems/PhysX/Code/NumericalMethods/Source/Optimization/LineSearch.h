/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <LinearAlgebra.h>
#include <NumericalMethods/Optimization.h>

namespace NumericalMethods::Optimization
{
    //! Used to indicated if a line search was successful or give details of failure reasons.
    enum class LineSearchOutcome
    {
        Success, //!< A result which completely satisfies the line search requirements.
        BestEffort, //!< A result which is not optimal but should still be usable.
        FailureExceededIterations //!< Failed because the iteration limit was reached and the value is not usable.
    };

    //! Struct to bundle together the numerical results of a line search and a qualitative indicator of search success.
    struct LineSearchResult
    {
        double m_stepSize;
        double m_functionValue;
        double m_derivativeValue;
        LineSearchOutcome m_outcome;
    };

    //! Helper function to check whether a LineSearchResult should be considered a failure.
    bool IsFailure(const LineSearchResult& result);

    //! Finds the value of x which minimizes the cubic polynomial which interpolates the provided points.
    //! Finds the cubic polynomial P(x) which satisfies
    //! P(a) = f_a
    //! P'(a) = df_a
    //! P(b) = f_b
    //! P(c) = f_c
    //! and returns the value of x which minimizes P.
    ScalarVariable CubicMinimum(const double a, const double f_a, const double df_a, const double b, const double f_b,
        const double c, const double f_c);

    //! Finds the value of x which minimizes the quadratic which interpolates the provided points.
    //! Finds the quadratic Q(x) which satisfies
    //! Q(a) = f_a
    //! Q'(a) = df_a
    //! Q(b) = f_b
    //! and returns the value of value which minimizes Q.
    ScalarVariable QuadraticMinimum(const double a, const double f_a, const double df_a, const double b, const double f_b);

    //! Checks that the result of an interpolation is satisfactory.
    //! Checks that the result of an interpolation is valid, inside the expected interval, and sufficiently far from
    //! interval boundaries.
    //! @param alphaNew The new step size to be checked.
    //! @param alpha0 One boundary of the current step size interval.
    //! @param alpha1 The other boundary of the current interval.
    //! @param edgeThreshold Defines how close to the edges of current interval the new step size is allowed to be.
    bool ValidateStepSize(const ScalarVariable alphaNew, const double alpha0, const double alpha1,
        const double edgeThreshold);

    //! Used in LineSearchWolfe to narrow down a step size once a bracketing interval has been found.
    //! This corresponds to the zoom function in Nocedal and Wright.
    LineSearchResult SelectStepSizeFromInterval(double alpha0, double alpha1, double f_alpha0, double f_alpha1,
        double df_alpha0, const Function& f, const VectorVariable& x0, const VectorVariable& searchDirection,
        const double f_x0, const double df_x0, const double c1, const double c2);

    //! Searches for a step size satisfying the Wolfe conditions for solution improvement.
    //! Given a search direction, attempts to find a step size in that direction which satisfies the Wolfe conditions (
    //! conditions for solution improvement which have nice properties for algorithms which rely on the line search).
    //! The first wolfe condition requires that the function value at the new point is sufficiently improved relative to
    //! the previous iteration.  The second condition requires that the directional derivative at the new point is
    //! sufficient to indicate that significantly more progress could not have been made by choosing a larger step.
    //! The search proceeds in two phases - first an interval containing a suitable point is found, then a point within
    //! that interval is narrowed down using the SelectStepSizeFromInterval function.
    LineSearchResult LineSearchWolfe(const Function& f, const VectorVariable& x0, double f_x0,
        const VectorVariable& searchDirection);
} // namespace NumericalMethods::Optimization
