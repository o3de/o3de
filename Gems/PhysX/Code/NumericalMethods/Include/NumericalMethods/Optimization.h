/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/Outcome/Outcome.h>

namespace NumericalMethods
{
    //! Namespace for methods for finding the values of function parameters which correspond to a (possibly local)
    //! optimum function value.  For more information on optimization methods, see Numerical Optimization by Nocedal and
    //! Wright (ISBN 978-0387303031).
    namespace Optimization
    {
        //! Used when returning the solver result to indicate if the solver was successful or indicate failure reasons.
        enum class SolverOutcome
        {
            Invalid, //!< Default value to which fields of this type should be initialized.
            Success, //!< The solver successfully achieved the stopping conditions.
            Incomplete, //!< The solver stalled, but the result may still be useful.
            MaxIterations, //!< Reached the iteration limit.
            Failure, //!< The solver failed for unspecified reasons.
            FailureInvalidInput, //!< The solver failed because the initial guess provided was not valid.
        };

        //! Struct for holding both the numerical result of the solver and the qualitative outcome i.e. success or
        //! failure.
        struct SolverResult
        {
            AZStd::vector<double> m_xValues; //!< The final value of the function parameters reached by the solver.

            //! Indicates whether the solver completed successfully or failed.
            SolverOutcome m_outcome = SolverOutcome::Invalid;

            //! The number of complete solver iterations before the result is returned.
            AZ::u32 m_iterations = 0;
        };

        //! Used when evaluating functions to indicate whether the evaluation was successful or indicate the failure
        //! reason.
        enum class FunctionOutcome
        {
            InvalidInput, //!< The number of parameters provided did not match the expected dimension.
        };

        //! Class used to represent functions to be optimized.
        //! To set up a particular function for optimization, derive from this class and override the GetDimension and
        //! ExecuteImpl methods.
        class Function
        {
        public:
            Function() = default;

            //! Used internally by solver routines to perform function evaluations.
            AZ::Outcome<double, FunctionOutcome> Execute(const AZStd::vector<double>& x) const
            {
                // Ensure the number of initial parameters provided matches the number of dimensions the function
                // expects.
                if (x.size() != GetDimension())
                {
                    return AZ::Failure(FunctionOutcome::InvalidInput);
                }

                return ExecuteImpl(x);
            }

            //! The number of parameters the function takes.
            //! For example, the function f(x, y) = x * y takes 2 parameters.
            virtual AZ::u32 GetDimension() const = 0;

        protected:
            //! The actual implementation of the function evaluation.
            //! This should be overridden with the implementation for the particular function it is desired to optimize.
            virtual AZ::Outcome<double, FunctionOutcome> ExecuteImpl(const AZStd::vector<double>& x) const = 0;
        };

        //! Minimizes the given mathematical function, using the initial guess as a starting point.
        //! @param function The function to minimize.
        //! @param initialGuess The input vector whose size must be equal to the dimension of the function.
        //! @return An instance of \ref SolverResult.
        SolverResult SolverBFGS(const Function& function, const AZStd::vector<double>& initialGuess);
    } // namespace Optimization
} // namespace NumericalMethods
