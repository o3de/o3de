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

#include <TestImpactFramework/TestImpactConfiguration.h>
#include <TestImpactFramework/TestImpactChangeList.h>
#include <TestImpactFramework/TestImpactClientTestSelection.h>
#include <TestImpactFramework/TestImpactClientTestRun.h>
#include <TestImpactFramework/TestImpactClientFailureReport.h>
#include <TestImpactFramework/TestImpactTestSequence.h>

#include <AzCore/std/string/string.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace TestImpact
{
    class DynamicDependencyMap;
    class TestEngine;
    class TestTarget;

    //! Callback for a test sequence that isn't using test impact analysis to determine selected tests.
    //! @param tests The tests that will be run for this sequence.
    using TestSequenceStartCallback = AZStd::function<void(Client::TestRunSelection&& tests)>;

    //! Callback for a test sequence using test impact analysis.
    //! @param selectedTests The tests that have been selected for this run by test impact analysis.
    //! @param discardedTests The tests that have been rejected for this run by test impact analysis. 
    //! @param draftedTests The tests that have been drafted in for this run due to requirements outside of test impact analysis
    //! (e.g. test targets that have been added to the repository since the last test impact analysis sequence or test that failed
    //! to execute previously).
    //! These tests will be run with coverage instrumentation.
    //! @note discardedTests and draftedTests may contain overlapping tests.
    using ImpactAnalysisTestSequenceStartCallback = AZStd::function<void(
        Client::TestRunSelection&& selectedTests,
        AZStd::vector<AZStd::string>&& discardedTests,
        AZStd::vector<AZStd::string>&& draftedTests)>;

    //! Callback for a test sequence using test impact analysis.
    //! @param selectedTests The tests that have been selected for this run by test impact analysis.
    //! @param discardedTests The tests that have been rejected for this run by test impact analysis.
    //! These tests will not be run without coverage instrumentation unless there is an entry in the draftedTests list.
    //! @param draftedTests The tests that have been drafted in for this run due to requirements outside of test impact analysis
    //! (e.g. test targets that have been added to the repository since the last test impact analysis sequence or test that failed
    //! to execute previously).
    //! @note discardedTests and draftedTests may contain overlapping tests.
    using SafeImpactAnalysisTestSequenceStartCallback = AZStd::function<void(
        Client::TestRunSelection&& selectedTests,
        Client::TestRunSelection&& discardedTests,
        AZStd::vector<AZStd::string>&& draftedTests)>;

    //! Callback for end of a test sequence.
    //! @param failureReport The test runs that failed for any reason during this sequence.
    //! @param duration The total duration of this test sequence.
    using TestSequenceCompleteCallback = AZStd::function<void(Client::RegularSequenceFailure&& failureReport, AZStd::chrono::milliseconds duration)>;

    //! Callback for end of a test impact analysis test sequence.
    //! @param failureReport The test runs that failed for any reason during this sequence.
    //! @param duration The total duration of this test sequence.
    using ImpactAnalysisTestSequenceCompleteCallback = AZStd::function<void(Client::ImpactAnalysisSequenceFailure&& failureReport, AZStd::chrono::milliseconds duration)>;

    //! Callback for test runs that have completed for any reason
    //! test The test that has completed.
    using TestCompleteCallback = AZStd::function<void(Client::TestRun&& test)>;

    //! The API exposed to the client responsible for all test runs and persistent data management.
    class Runtime
    {
    public:
        //! Constructs a runtime with the specified configuration and policies.
        //! @param config The configuration used for this runtime instance.
        //! @param executionFailurePolicy Determines how to handle test targets that fail to execute.
        //! @param executionFailureDraftingPolicy Determines how test targets that previously failed to execute are drafted into subsequent test sequences.
        //! @param testFailurePolicy Determines how to handle test targets that report test failures.
        //! @param integrationFailurePolicy Determines how to handle instances where the build system model and/or test impact analysis data is compromised.
        //! @param testShardingPolicy  Determines how to handle test targets that have opted in to test sharding.
        Runtime(
            RuntimeConfig&& config,
            Policy::ExecutionFailure executionFailurePolicy,
            Policy::ExecutionFailureDrafting executionFailureDraftingPolicy,
            Policy::TestFailure testFailurePolicy,
            Policy::IntegrityFailure integrationFailurePolicy,
            Policy::TestSharding testShardingPolicy,
            TargetOutputCapture targetOutputCapture,
            AZStd::optional<size_t> maxConcurrency = AZStd::nullopt);

        ~Runtime();

        //! Runs a test sequence where all tests with a matching suite in the suite filter and also not on the excluded list are selected.
        //! @param suitesFilter The test suites that will be included in the test selection.
        //! @param testTargetTimeout The maximum duration individual test targets may be in flight for (infinite if empty).
        //! @param testTargetTimeout The maximum duration the entire test sequence may run for (infinite if empty).
        //! @param testSequenceStartCallback The client function to be called after the test targets have been selected but prior to running the tests.
        //! @param testSequenceCompleteCallback The client function to be called after the test sequence has completed.
        //! @param testRunCompleteCallback The client function to be called after an individual test run has compelted.
        TestSequenceResult RegularTestSequence(
            const AZStd::unordered_set<AZStd::string> suitesFilter,
            AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
            AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
            AZStd::optional<TestSequenceStartCallback> testSequenceStartCallback,
            AZStd::optional<TestSequenceCompleteCallback> testSequenceCompleteCallback,
            AZStd::optional<TestCompleteCallback> testRunCompleteCallback);

        //! Runs a test sequence where tests are selected according to test impact analysis so long as they are not on the excluded list.
        //! @param changeList The change list used to determine the tests to select.
        //! @param testPrioritizationPolicy Determines how selected tests will be prioritized.
        //! @param testTargetTimeout The maximum duration individual test targets may be in flight for (infinite if empty).
        //! @param testTargetTimeout The maximum duration the entire test sequence may run for (infinite if empty).
        //! @param testSequenceStartCallback The client function to be called after the test targets have been selected but prior to running the tests.
        //! @param testSequenceCompleteCallback The client function to be called after the test sequence has completed.
        //! @param testRunCompleteCallback The client function to be called after an individual test run has compelted.
        TestSequenceResult ImpactAnalysisTestSequence(
            const ChangeList& changeList,
            Policy::TestPrioritization testPrioritizationPolicy,
            AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
            AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
            AZStd::optional<ImpactAnalysisTestSequenceStartCallback> testSequenceStartCallback,
            AZStd::optional<ImpactAnalysisTestSequenceCompleteCallback> testSequenceCompleteCallback,
            AZStd::optional<TestCompleteCallback> testRunCompleteCallback);

        //! Runs a test sequence as per the ImpactAnalysisTestSequence where the tests not selected are also run (albeit without instrumentation).
        //! @param changeList The change list used to determine the tests to select.
        //! @param suitesFilter The test suites that will be included in the test selection.
        //! @param testPrioritizationPolicy Determines how selected tests will be prioritized.
        //! @param testTargetTimeout The maximum duration individual test targets may be in flight for (infinite if empty).
        //! @param testTargetTimeout The maximum duration the entire test sequence may run for (infinite if empty).
        //! @param testSequenceStartCallback The client function to be called after the test targets have been selected but prior to running the tests.
        //! @param testSequenceCompleteCallback The client function to be called after the test sequence has completed.
        //! @param testRunCompleteCallback The client function to be called after an individual test run has compelted.
        TestSequenceResult SafeImpactAnalysisTestSequence(
            const ChangeList& changeList,
            const AZStd::unordered_set<AZStd::string> suitesFilter,
            Policy::TestPrioritization testPrioritizationPolicy,
            AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
            AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
            AZStd::optional<SafeImpactAnalysisTestSequenceStartCallback> testSequenceStartCallback,
            AZStd::optional<ImpactAnalysisTestSequenceCompleteCallback> testSequenceCompleteCallback,
            AZStd::optional<TestCompleteCallback> testRunCompleteCallback);

        //! Runs all tests not on the excluded list and uses their coverage data to seed the test impact analysis data (ant existing data will be overwritten).
        //! @param testTargetTimeout The maximum duration the entire test sequence may run for (infinite if empty).
        //! @param testSequenceStartCallback The client function to be called after the test targets have been selected but prior to running the tests.
        //! @param testSequenceCompleteCallback The client function to be called after the test sequence has completed.
        //! @param testRunCompleteCallback The client function to be called after an individual test run has compelted.
        TestSequenceResult SeededTestSequence(
            AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
            AZStd::optional<TestSequenceStartCallback> testSequenceStartCallback,
            AZStd::optional<TestSequenceCompleteCallback> testSequenceCompleteCallback,
            AZStd::optional<TestCompleteCallback> testRunCompleteCallback);

        //! Returns true if the runtime has test impact analysis data (either preexisting or generated).
        bool HasImpactAnalysisData() const;

    private:
        RuntimeConfig m_config;
        Policy::ExecutionFailure m_executionFailurePolicy;
        Policy::ExecutionFailureDrafting m_executionFailureDraftingPolicy;
        Policy::TestFailure m_testFailurePolicy;
        Policy::IntegrityFailure m_integrationFailurePolicy;
        Policy::TestSharding m_testShardingPolicy;
        TargetOutputCapture m_targetOutputCapture;
        size_t m_maxConcurrency;
        AZStd::unique_ptr<DynamicDependencyMap> m_dynamicDependencyMap;
        AZStd::unique_ptr<TestEngine> m_testEngine;
        AZStd::unordered_set<const TestTarget*> m_testTargetExcludeList;
        AZStd::unordered_set<const TestTarget*> m_testTargetShardList;
        bool m_hasImpactAnalysisData = false;
    };
} // namespace TestImpact
