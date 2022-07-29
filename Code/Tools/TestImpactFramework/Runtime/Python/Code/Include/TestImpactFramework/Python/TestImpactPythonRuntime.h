/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/Python/TestImpactPythonConfiguration.h>

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/containers/unordered_set.h>

namespace TestImpact
{
    class PythonTestEngine;
    class PythonTestTarget;
    class PythonProductionTarget;
    class SourceCoveringTestsList;

    template<typename TestTarget, typename ProdutionTarget>
    class ChangeDependencyList;

    template<typename TestTarget, typename ProdutionTarget>
    class BuildTargetList;

    template<typename TestTarget, typename ProdutionTarget>
    class DynamicDependencyMap;

    template<typename TestTarget, typename ProdutionTarget>
    class TestSelectorAndPrioritizer;

    template<typename TestTarget>
    class TestTargetExclusionList;

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

        //! Returns true if the runtime has test impact analysis data (either preexisting or generated).
        bool HasImpactAnalysisData() const
        {
            return false;
        }

    private:
        PythonRuntimeConfig m_config;
        RepoPath m_sparTiaFile;
        SuiteType m_suiteFilter;
        Policy::ExecutionFailure m_executionFailurePolicy;
        Policy::FailedTestCoverage m_failedTestCoveragePolicy;
        Policy::TestFailure m_testFailurePolicy;
        Policy::IntegrityFailure m_integrationFailurePolicy;
        Policy::TargetOutputCapture m_targetOutputCapture;
        size_t m_maxConcurrency = 0;
        AZStd::unique_ptr<BuildTargetList<PythonProductionTarget, PythonTestTarget>> m_buildTargets;
        AZStd::unique_ptr<DynamicDependencyMap<PythonProductionTarget, PythonTestTarget>> m_dynamicDependencyMap;
        AZStd::unique_ptr<TestSelectorAndPrioritizer<PythonProductionTarget, PythonTestTarget>> m_testSelectorAndPrioritizer;
        AZStd::unique_ptr<PythonTestEngine> m_testEngine;
        AZStd::unique_ptr<TestTargetExclusionList<PythonTestTarget>> m_testTargetExcludeList;
        AZStd::unordered_set<const PythonTestTarget*> m_previouslyFailingTestTargets;
        bool m_hasImpactAnalysisData = false;
    };
} // namespace TestImpact
