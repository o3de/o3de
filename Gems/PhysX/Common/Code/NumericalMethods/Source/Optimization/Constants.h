/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
    const double WolfeConditionsC1 = 1e-4;
    const double WolfeConditionsC2 = 0.9;
} // namespace NumericalMethods::Optimization
