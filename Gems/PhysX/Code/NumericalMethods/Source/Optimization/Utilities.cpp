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
