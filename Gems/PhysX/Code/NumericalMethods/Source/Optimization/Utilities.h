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
#include <LinearAlgebra.h>

namespace NumericalMethods::Optimization
{
    class Function;

    //! Helper function for evaluating a function at a point.
    double FunctionValue(const Function& function, const VectorVariable& point);

    //! The 1-dimensional rate of change of a function with respect to changing the independent variables along the specified direction.
    //! Note that some textbooks / authors define the directional derivative with respect to a normalized direction,
    //! but that convention is not used here.
    double DirectionalDerivative(const Function& function, const VectorVariable& point, const VectorVariable& direction);

    //! Vector of derivatives with respect to each of the independent variables of a function, evaluated at the specified point.
    VectorVariable Gradient(const Function& function, const VectorVariable& point);
} // namespace NumericalMethods::Optimization
