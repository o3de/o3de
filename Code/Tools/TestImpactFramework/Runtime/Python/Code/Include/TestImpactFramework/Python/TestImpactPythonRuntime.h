/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/Python/TestImpactPythonConfiguration.h>

namespace TestImpact
{
    //! The native API exposed to the client responsible for all test runs and persistent data management.
    class PythonRuntime
    {
    public:
        PythonRuntime(
            [[maybe_unused]] PythonRuntimeConfig&& config,
            [[maybe_unused]] const AZStd::optional<RepoPath>& dataFile,
            [[maybe_unused]] const AZStd::optional<RepoPath>& previousRunDataFile,
            [[maybe_unused]] const AZStd::vector<ExcludedTarget>& testsToExclude,
            [[maybe_unused]] SuiteType suiteFilter,
            [[maybe_unused]] Policy::ExecutionFailure executionFailurePolicy,
            [[maybe_unused]] Policy::FailedTestCoverage failedTestCoveragePolicy,
            [[maybe_unused]] Policy::TestFailure testFailurePolicy,
            [[maybe_unused]] Policy::IntegrityFailure integrationFailurePolicy,
            [[maybe_unused]] Policy::TargetOutputCapture targetOutputCapture)
        {
        }

        ~PythonRuntime()
        {

        }

        //! Returns true if the runtime has test impact analysis data (either preexisting or generated).
        bool HasImpactAnalysisData() const
        {
            return false;
        }
    };
} // namespace TestImpact
