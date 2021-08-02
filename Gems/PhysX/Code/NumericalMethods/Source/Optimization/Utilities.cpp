/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <NumericalMethods/Optimization.h>
#include <Optimization/Constants.h>
#include <Optimization/Utilities.h>

namespace NumericalMethods::Optimization
{
    double FunctionValue(const Function& function, const VectorVariable& point)
    {
        return function.Execute(point.GetValues()).GetValue();
    }

    double DirectionalDerivative(const Function& function, const VectorVariable& point, const VectorVariable& direction)
    {
        return Gradient(function, point).Dot(direction);
    }

    VectorVariable Gradient(const Function& function, const VectorVariable& point)
    {
        const AZ::u32 dimension = point.GetDimension();
        VectorVariable gradient(dimension);
        VectorVariable direction(dimension);
        for (AZ::u32 i = 0; i < dimension; i++)
        {
            direction[i] = 1.0;
            double f_plus = FunctionValue(function, point + epsilon * direction);
            double f_minus = FunctionValue(function, point - epsilon * direction);
            gradient[i] = (f_plus - f_minus) / (2.0 * epsilon);
            direction[i] = 0.0;
        }
        return gradient;
    }
} // namespace NumericalMethods::Optimization
