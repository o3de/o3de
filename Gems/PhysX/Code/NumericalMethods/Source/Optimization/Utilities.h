/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
