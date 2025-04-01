/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactUtils.h>

#include <TestRunner/Python/TestImpactPythonInstrumentedTestRunnerBase.h>

namespace TestImpact
{
    //! Null test runner for instrumented Python tests.
    class PythonInstrumentedNullTestRunner
        : public PythonInstrumentedTestRunnerBase
    {
    public:
        using PythonInstrumentedTestRunnerBase::PythonInstrumentedTestRunnerBase;

        AZStd::pair<ProcessSchedulerResult, AZStd::vector<TestJobRunner::Job>> RunTests(
            const TestJobRunner::JobInfos& jobInfos,
            StdOutputRouting stdOutRouting,
            StdErrorRouting stdErrRouting,
            AZStd::optional<AZStd::chrono::milliseconds> runTimeout,
            AZStd::optional<AZStd::chrono::milliseconds> runnerTimeout) override;
    };
} // namespace TestImpact
