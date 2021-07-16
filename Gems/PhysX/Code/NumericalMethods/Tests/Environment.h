/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include <LinearAlgebra.h>

namespace NumericalMethods
{
    void ExpectClose(const VectorVariable& actual, const VectorVariable& expected, double tolerance);
    void ExpectClose(const AZStd::vector<double>& actual, const AZStd::vector<double>& expected, double tolerance);
} // namespace NumericalMethods
