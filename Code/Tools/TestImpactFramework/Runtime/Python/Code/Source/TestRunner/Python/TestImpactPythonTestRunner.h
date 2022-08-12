/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactUtils.h>

#include <TestRunner/Python/TestImpactPythonTestRunnerBase.h>

namespace TestImpact
{
    class PythonTestRunner
        : public PythonTestRunnerBase
    {
    public:
        using PythonTestRunnerBase::PythonTestRunnerBase;
    };
} // namespace TestImpact
