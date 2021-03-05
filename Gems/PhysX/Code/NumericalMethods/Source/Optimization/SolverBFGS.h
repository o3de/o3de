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
