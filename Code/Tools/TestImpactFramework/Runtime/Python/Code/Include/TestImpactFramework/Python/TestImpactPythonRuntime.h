/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactRuntime.h>

#include <TestImpactFramework/Python/TestImpactPythonConfiguration.h>

#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace TestImpact
{
    class PythonTestEngine;
    class PythonTestTarget;
    class PythonProductionTarget;
    class SourceCoveringTestsList;

    //! The python API exposed to the client responsible for all test runs and persistent data management.
    class PythonRuntime
    {
    public:
        PythonRuntime(
            PythonRuntimeConfig&& config,
            const AZStd::optional<RepoPath>& dataFile,
            [[maybe_unused]] const AZStd::optional<RepoPath>& previousRunDataFile,
            const AZStd::vector<ExcludedTarget>& testsToExclude,
            SuiteType suiteFilter,
            Policy::ExecutionFailure executionFailurePolicy,
            Policy::FailedTestCoverage failedTestCoveragePolicy,
            Policy::TestFailure testFailurePolicy,
            Policy::IntegrityFailure integrationFailurePolicy,
            Policy::TargetOutputCapture targetOutputCapture);

        ~PythonRuntime();

        bool HasImpactAnalysisData() const;

    private:
        PythonRuntimeConfig m_config;
        RepoPath m_sparTiaFile;
        SuiteType m_suiteFilter;
        Policy::ExecutionFailure m_executionFailurePolicy;
        Policy::FailedTestCoverage m_failedTestCoveragePolicy;
        Policy::TestFailure m_testFailurePolicy;
        Policy::IntegrityFailure m_integrationFailurePolicy;
        Policy::TargetOutputCapture m_targetOutputCapture;
        AZStd::unique_ptr<BuildTargetList<PythonProductionTarget, PythonTestTarget>> m_buildTargets;
        AZStd::unique_ptr<DynamicDependencyMap<PythonProductionTarget, PythonTestTarget>> m_dynamicDependencyMap;
        AZStd::unique_ptr<TestSelectorAndPrioritizer<PythonProductionTarget, PythonTestTarget>> m_testSelectorAndPrioritizer;
        AZStd::unique_ptr<PythonTestEngine> m_testEngine;
        AZStd::unique_ptr<TestTargetExclusionList<PythonTestTarget>> m_testTargetExcludeList;
        AZStd::unordered_set<const PythonTestTarget*> m_previouslyFailingTestTargets;
        bool m_hasImpactAnalysisData = false;
    };
} // namespace TestImpact
