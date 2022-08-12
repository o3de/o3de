/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactRuntime.h>
#include <TestImpactFramework/Native/TestImpactNativeConfiguration.h>

#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace TestImpact
{
    class NativeTestEngine;
    class NativeTestTarget;
    class NativeProductionTarget;
    class SourceCoveringTestsList;

    //! The native API exposed to the client responsible for all test runs and persistent data management.
    class NativeRuntime
    {
    public:
        //! Constructs a runtime with the specified configuration and policies.
        //! @param config The configuration used for this runtime instance.
        //! @param dataFile The optional data file to be used instead of that specified in the config file.
        //! @param previousRunDataFile The optional previous run data file to be used instead of that specified in the config file.
        //! @param testsToExclude The tests to exclude from the run (will override any excluded tests in the config file).
        //! @param suiteFilter The test suite for which the coverage data and test selection will draw from.
        //! @param executionFailurePolicy Determines how to handle test targets that fail to execute.
        //! @param executionFailureDraftingPolicy Determines how test targets that previously failed to execute are drafted into subsequent test sequences.
        //! @param testFailurePolicy Determines how to handle test targets that report test failures.
        //! @param integrationFailurePolicy Determines how to handle instances where the build system model and/or test impact analysis data is compromised.
        //! @param testShardingPolicy  Determines how to handle test targets that have opted in to test sharding.
        NativeRuntime(
            NativeRuntimeConfig&& config,
            const AZStd::optional<RepoPath>& dataFile,
            [[maybe_unused]]const AZStd::optional<RepoPath>& previousRunDataFile,
            const AZStd::vector<ExcludedTarget>& testsToExclude,
            SuiteType suiteFilter,
            Policy::ExecutionFailure executionFailurePolicy,
            Policy::FailedTestCoverage failedTestCoveragePolicy,
            Policy::TestFailure testFailurePolicy,
            Policy::IntegrityFailure integrationFailurePolicy,
            Policy::TestSharding testShardingPolicy,
            Policy::TargetOutputCapture targetOutputCapture,
            AZStd::optional<size_t> maxConcurrency = AZStd::nullopt);

        ~NativeRuntime();
        
        //! Runs a test sequence where all tests with a matching suite in the suite filter and also not on the excluded list are selected.
        //! @param testTargetTimeout The maximum duration individual test targets may be in flight for (infinite if empty).
        //! @param globalTimeout The maximum duration the entire test sequence may run for (infinite if empty).
        //! @param testSequenceStartCallback The client function to be called after the test targets have been selected but prior to running the tests.
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
        //! @param dynamicDependencyMapPolicy The policy to determine how the coverage data of produced by test sequences is used to update the dynamic dependency map.
        //! @param testTargetTimeout The maximum duration individual test targets may be in flight for (infinite if empty).
        //! @param globalTimeout The maximum duration the entire test sequence may run for (infinite if empty).
        //! @param testSequenceStartCallback The client function to be called after the test targets have been selected but prior to running the tests.
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

        //! Runs a test sequence as per the ImpactAnalysisTestSequence where the tests not selected are also run (albeit without instrumentation).
        //! @param changeList The change list used to determine the tests to select.
        //! @param testPrioritizationPolicy Determines how selected tests will be prioritized.
        //! @param testTargetTimeout The maximum duration individual test targets may be in flight for (infinite if empty).
        //! @param globalTimeout The maximum duration the entire test sequence may run for (infinite if empty).
        //! @param testSequenceStartCallback The client function to be called after the test targets have been selected but prior to running the tests.
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

        //! Runs all tests not on the excluded list and uses their coverage data to seed the test impact analysis data (ant existing data will be overwritten).
        //! @param testTargetTimeout The maximum duration individual test targets may be in flight for (infinite if empty).
        //! @param globalTimeout The maximum duration the entire test sequence may run for (infinite if empty).
        //! @param testSequenceStartCallback The client function to be called after the test targets have been selected but prior to running the tests.
        //! @param testSequenceCompleteCallback The client function to be called after the test sequence has completed.
        //! @param testRunCompleteCallback The client function to be called after an individual test run has completed.
        //! @returns The test run and sequence report for the selected test sequence.
        Client::SeedSequenceReport SeededTestSequence(
            AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
            AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
            AZStd::optional<TestSequenceStartCallback> testSequenceStartCallback,
            AZStd::optional<TestSequenceCompleteCallback<Client::SeedSequenceReport>> testSequenceCompleteCallback,
            AZStd::optional<TestRunCompleteCallback> testRunCompleteCallback);

        //! Returns true if the runtime has test impact analysis data (either preexisting or generated).
        bool HasImpactAnalysisData() const;

    private:
        using RuntimeConfig = NativeRuntimeConfig;
        using TestEngine = NativeTestEngine;
        using ProductionTarget = NativeProductionTarget;
        using TestTarget = NativeTestTarget;

        //! Updates the test enumeration cache for test targets that had sources modified by a given change list.
        //! @param changeDependencyList The resolved change dependency list generated for the change list.
        //void EnumerateMutatedTestTargets(const ChangeDependencyList& changeDependencyList);

        //! Selects the test targets covering a given change list and updates the enumeration cache of the test targets with sources
        //! modified in that change list.
        //! @param changeList The change list for which the covering tests and enumeration cache updates will be generated for.
        //! @param testPrioritizationPolicy The test prioritization strategy to use for the selected test targets.
        //! @returns The pair of selected test targets and discarded test targets.
        AZStd::pair<AZStd::vector<const TestTarget*>, AZStd::vector<const TestTarget*>> SelectCoveringTestTargets(
            const ChangeList& changeList,
            Policy::TestPrioritization testPrioritizationPolicy);

        //! Prepares the dynamic dependency map for a seed update by clearing all existing data and deleting the file that will be serialized.
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

        RuntimeConfig m_config;
        RepoPath m_sparTiaFile;
        SuiteType m_suiteFilter;
        Policy::ExecutionFailure m_executionFailurePolicy;
        Policy::FailedTestCoverage m_failedTestCoveragePolicy;
        Policy::TestFailure m_testFailurePolicy;
        Policy::IntegrityFailure m_integrationFailurePolicy;
        Policy::TestSharding m_testShardingPolicy;
        Policy::TargetOutputCapture m_targetOutputCapture;
        size_t m_maxConcurrency = 0;
        AZStd::unique_ptr<BuildTargetList<ProductionTarget, TestTarget>> m_buildTargets;
        AZStd::unique_ptr<DynamicDependencyMap<ProductionTarget, TestTarget>> m_dynamicDependencyMap;
        AZStd::unique_ptr<TestSelectorAndPrioritizer<ProductionTarget, TestTarget>> m_testSelectorAndPrioritizer;
        AZStd::unique_ptr<TestEngine> m_testEngine;
        AZStd::unique_ptr<TestTargetExclusionList<TestTarget>> m_regularTestTargetExcludeList;
        AZStd::unique_ptr<TestTargetExclusionList<TestTarget>> m_instrumentedTestTargetExcludeList;
        AZStd::unordered_set<const TestTarget*> m_testTargetShardList;
        AZStd::unordered_set<const TestTarget*> m_previouslyFailingTestTargets;
        bool m_hasImpactAnalysisData = false;
    };
} // namespace TestImpact
