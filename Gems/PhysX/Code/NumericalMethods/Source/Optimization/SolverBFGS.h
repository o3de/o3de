/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <NumericalMethods/Optimization.h>

namespace NumericalMethods::Optimization
{
    //! Minimizes the supplied function using the Broyden-Fletcher-Goldfarb-Shanno algorithm (see Nocedal and Wright).
    //! @param f Function to be minimized.
    //! @param xInitial Initial guess for the independent variable.
    //! @return Value of the independent variable which minimizes f.
    SolverResult MinimizeBFGS(const Function& f, const AZStd::vector<double>& xInitial);
} // namespace NumericalMethods::Optimization
