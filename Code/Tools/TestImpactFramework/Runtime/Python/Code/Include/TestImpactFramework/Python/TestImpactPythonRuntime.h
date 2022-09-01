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
    class PythonTestSelectorAndPrioritizer;

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

        //! Runs a test sequence where all tests with a matching suite in the suite filter and also not on the excluded list are selected.
        //! @param testTargetTimeout The maximum duration individual test targets may be in flight for (infinite if empty).
        //! @param globalTimeout The maximum duration the entire test sequence may run for (infinite if empty).
        //! @param testSequenceStartCallback The client function to be called after the test targets have been selected but prior to running
        //! the tests.
        //! @param testSequenceCompleteCallback The client function to be called after the test sequence has completed.
        //! @param testRunCompleteCallback The client function to be called after an individual test run has completed.
        //! @returns The test run and sequence report for the selected test sequence.
        Client::RegularSequenceReport RegularTestSequence(
            AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
            AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
            AZStd::optional<TestSequenceStartCallback> testSequenceStartCallback,
            AZStd::optional<TestSequenceCompleteCallback<Client::RegularSequenceReport>> testSequenceCompleteCallback,
            AZStd::optional<TestRunCompleteCallback> testRunCompleteCallback);

        //! Runs a test sequence where tests are selected according to test impact analysis so long as they are not on the excluded list.
        //! @param changeList The change list used to determine the tests to select.
        //! @param testPrioritizationPolicy Determines how selected tests will be prioritized.
        //! @param dynamicDependencyMapPolicy The policy to determine how the coverage data of produced by test sequences is used to update
        //! the dynamic dependency map.
        //! @param testTargetTimeout The maximum duration individual test targets may be in flight for (infinite if empty).
        //! @param globalTimeout The maximum duration the entire test sequence may run for (infinite if empty).
        //! @param testSequenceStartCallback The client function to be called after the test targets have been selected but prior to running
        //! the tests.
        //! @param testSequenceCompleteCallback The client function to be called after the test sequence has completed.
        //! @param testRunCompleteCallback The client function to be called after an individual test run has completed.
        //! @returns The test run and sequence report for the selected and drafted test sequences.
        Client::ImpactAnalysisSequenceReport ImpactAnalysisTestSequence(
            const ChangeList& changeList,
            Policy::TestPrioritization testPrioritizationPolicy,
            Policy::DynamicDependencyMap dynamicDependencyMapPolicy,
            AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
            AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
            AZStd::optional<ImpactAnalysisTestSequenceStartCallback> testSequenceStartCallback,
            AZStd::optional<TestSequenceCompleteCallback<Client::ImpactAnalysisSequenceReport>> testSequenceCompleteCallback,
            AZStd::optional<TestRunCompleteCallback> testRunCompleteCallback);

        //! Runs a test sequence as per the ImpactAnalysisTestSequence where the tests not selected are also run (albeit without
        //! instrumentation).
        //! @param changeList The change list used to determine the tests to select.
        //! @param testPrioritizationPolicy Determines how selected tests will be prioritized.
        //! @param testTargetTimeout The maximum duration individual test targets may be in flight for (infinite if empty).
        //! @param globalTimeout The maximum duration the entire test sequence may run for (infinite if empty).
        //! @param testSequenceStartCallback The client function to be called after the test targets have been selected but prior to running
        //! the tests.
        //! @param testSequenceCompleteCallback The client function to be called after the test sequence has completed.
        //! @param testRunCompleteCallback The client function to be called after an individual test run has completed.
        //! @returns The test run and sequence report for the selected, discarded and drafted test sequences.
        Client::SafeImpactAnalysisSequenceReport SafeImpactAnalysisTestSequence(
            const ChangeList& changeList,
            Policy::TestPrioritization testPrioritizationPolicy,
            AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
            AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
            AZStd::optional<SafeImpactAnalysisTestSequenceStartCallback> testSequenceStartCallback,
            AZStd::optional<TestSequenceCompleteCallback<Client::SafeImpactAnalysisSequenceReport>> testSequenceCompleteCallback,
            AZStd::optional<TestRunCompleteCallback> testRunCompleteCallback);

        //! Runs all tests not on the excluded list and uses their coverage data to seed the test impact analysis data (ant existing data
        //! will be overwritten).
        //! @param testTargetTimeout The maximum duration individual test targets may be in flight for (infinite if empty).
        //! @param globalTimeout The maximum duration the entire test sequence may run for (infinite if empty).
        //! @param testSequenceStartCallback The client function to be called after the test targets have been selected but prior to running
        //! the tests.
        //! @param testSequenceCompleteCallback The client function to be called after the test sequence has completed.
        //! @param testRunCompleteCallback The client function to be called after an individual test run has completed.
        //! @returns The test run and sequence report for the selected test sequence.
        Client::SeedSequenceReport SeededTestSequence(
            AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
            AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
            AZStd::optional<TestSequenceStartCallback> testSequenceStartCallback,
            AZStd::optional<TestSequenceCompleteCallback<Client::SeedSequenceReport>> testSequenceCompleteCallback,
            AZStd::optional<TestRunCompleteCallback> testRunCompleteCallback);

    private:
        using RuntimeConfig = PythonRuntimeConfig;
        using TestEngine = PythonTestEngine;
        using ProductionTarget = PythonProductionTarget;
        using TestTarget = PythonTestTarget;

        //! Selects the test targets covering a given change list and updates the enumeration cache of the test targets with sources
        //! modified in that change list.
        //! @param changeList The change list for which the covering tests and enumeration cache updates will be generated for.
        //! @param testPrioritizationPolicy The test prioritization strategy to use for the selected test targets.
        //! @returns The pair of selected test targets and discarded test targets.
        AZStd::pair<AZStd::vector<const TestTarget*>, AZStd::vector<const TestTarget*>> SelectCoveringTestTargets(
            const ChangeList& changeList, Policy::TestPrioritization testPrioritizationPolicy);

        //! Prepares the dynamic dependency map for a seed update by clearing all existing data and deleting the file that will be
        //! serialized.
        void ClearDynamicDependencyMapAndRemoveExistingFile();

        //! Generates a base policy state for the current runtime policy runtime configuration.
        PolicyStateBase GeneratePolicyStateBase() const;

        //! Generates a regular/seed sequence policy state for the current runtime policy runtime configuration.
        SequencePolicyState GenerateSequencePolicyState() const;

        //! Generates a safe impact analysis sequence policy state for the current runtime policy runtime configuration.
        SafeImpactAnalysisSequencePolicyState GenerateSafeImpactAnalysisSequencePolicyState(
            Policy::TestPrioritization testPrioritizationPolicy) const;

        //! Generates an impact analysis sequence policy state for the current runtime policy runtime configuration.
        ImpactAnalysisSequencePolicyState GenerateImpactAnalysisSequencePolicyState(
            Policy::TestPrioritization testPrioritizationPolicy, Policy::DynamicDependencyMap dynamicDependencyMapPolicy) const;

        PythonRuntimeConfig m_config;
        RepoPath m_sparTiaFile;
        SuiteType m_suiteFilter;
        Policy::ExecutionFailure m_executionFailurePolicy;
        Policy::FailedTestCoverage m_failedTestCoveragePolicy;
        Policy::TestFailure m_testFailurePolicy;
        Policy::IntegrityFailure m_integrationFailurePolicy;
        Policy::TargetOutputCapture m_targetOutputCapture;
        AZStd::unique_ptr<BuildTargetList<ProductionTarget, TestTarget>> m_buildTargets;
        AZStd::unique_ptr<DynamicDependencyMap<ProductionTarget, TestTarget>> m_dynamicDependencyMap;
        AZStd::unique_ptr<PythonTestSelectorAndPrioritizer> m_testSelectorAndPrioritizer;
        AZStd::unique_ptr<TestEngine> m_testEngine;
        AZStd::unique_ptr<TestTargetExclusionList<TestTarget>> m_testTargetExcludeList;
        AZStd::unordered_set<const TestTarget*> m_previouslyFailingTestTargets;
        bool m_hasImpactAnalysisData = false;
    };
} // namespace TestImpact
