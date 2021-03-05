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

#include <AzCore/base.h>

namespace NumericalMethods::Optimization
{
    const AZ::u32 lineSearchIterations = 100;
    const AZ::u32 solverIterations = 500;

    // value of the gradient norm used for terminating the search
    const double gradientTolerance = 1e-6;

    // used in finite difference evaluation of derivatives
    // a value close to the square root of the machine precision is recommended in Nocedal and Wright
    const double epsilon = 1e-7;

    // values recommended in Nocedal and Wright for constants in the Wolfe conditions for satisfactory solution improvement
    const double c1 = 1e-4;
    const double c2 = 0.9;
} // namespace NumericalMethods::Optimization
