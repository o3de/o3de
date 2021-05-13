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
#include <TestImpactFramework/TestImpactTestSelection.h>
#include <TestImpactFramework/TestImpactTestRun.h>

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

    //! Policy for handling of test targets that fail to execute (e.g. due to the binary not being found).
    //! @note Test targets that fail to execute will be tagged such that their execution can be attempted at a later date. This is
    //! important as otherwise it would be erroneously assumed that they cover no sources due to having no entries in the dynamic
    //! dependency map.
    enum class ExecutionFailurePolicy
    {
        Abort, //! Abort the test sequence and report a failure.
        Continue, //! Continue the test sequence but treat the execution failures as test failures after the run.
        Ignore //! Continue the test sequence and ignore the execution failures.
    };

    //! Policy for reattempting the execution of test targets that failed to execute in previous runs.
    enum class ExecutionFailureDraftingPolicy
    {
        Never, //! Do not attempt to execute historic execution failures.
        Always //! Reattempt the exectuion of historic execution failures.
    };

    //! Policy for handling test targets that report failing tests.
    enum class TestFailurePolicy
    {
        Abort, //! Abort the test sequence and report the test failure.
        Continue //! Continue the test sequence and report the test failures after the run.
    };

    //! Policy for prioritizing selected tests.
    enum class TestPrioritizationPolicy
    {
        None, //! Do not attempt any test prioritization.
        DependencyLocality //! Prioritize test targets according to the locality of the production targets they cover in the build dependency graph.
    };

    //! Policy for sharding test targets that have been marked for test sharding.
    enum class TestShardingPolicy
    {
        Never, //! Do not shard any test targets.
        Always //! Shard all test targets that have been marked for test sharding.
    };

    //! Standard output capture of test target runs. 
    enum class TargetOutputCapture
    {
        None, //! Do not capture any output.
        StdOut, //! Send captured output to standard output
        File, //! Write captured output to file.
        StdOutAndFile //! Send captured output to standard output and write to file.
    };

    //! 
    enum class TestSequenceResult
    {
        Success,
        Failure,
        Timeout
    };

    //! Callback for a test sequence that isn't using test impact analysis.
    //! @param tests The tests that will be run for this sequence.
    using TestSequenceStartCallback = AZStd::function<void(const TestSelection& tests)>;

    //! Callback for a test sequence using test impact analysis.
    //! @param selectedTests The tests that have been selected for this run by test impact analysis.
    //! @param discardedTests The tests that have been rejected for this run by test impact analysis. 
    //! @param draftedTests The tests that have been drafted in for this run due to requirements outside of test impact analysis
    //! (e.g. test targets that have been added to the repository since the last test impact analysis sequence or test that failed
    //! to execute previously).
    //! These tests will be run with coverage instrumentation.
    //! @note discardedTests and draftedTests may contain overlapping tests.
    using ImpactAnalysisTestSequenceStartCallback = AZStd::function<void(
        const TestSelection& selectedTests,
        const AZStd::vector<AZStd::string>& discardedTests,
        const AZStd::vector<AZStd::string>& draftedTests)>;

    //! Callback for a test sequence using test impact analysis.
    //! @param selectedTests The tests that have been selected for this run by test impact analysis.
    //! @param discardedTests The tests that have been rejected for this run by test impact analysis.
    //! These tests will not be run without coverage instrumentation unless there is an entry in the draftedTests list.
    //! @param draftedTests The tests that have been drafted in for this run due to requirements outside of test impact analysis
    //! (e.g. test targets that have been added to the repository since the last test impact analysis sequence or test that failed
    //! to execute previously).
    //! @note discardedTests and draftedTests may contain overlapping tests.
    using SafeImpactAnalysisTestSequenceStartCallback = AZStd::function<void(
        const TestSelection& selectedTests,
        const TestSelection& discardedTests,
        const AZStd::vector<AZStd::string>& draftedTests)>;

    //! Callback for a test sequence ending.
    //! @param testRuns The test results of all test target runs.
    using TestSequenceEndCallback = AZStd::function<void(const AZStd::vector<TestRun>& testRuns)>;

    //! Callback for test run ending.
    //! testRun The test run that has ended.
    using TestRunEndCallback = AZStd::function<void(const TestRunEnd& testRunEnd)>;

    //!
    class Runtime
    {
    public:
        Runtime(
            RuntimeConfig&& config,
            ExecutionFailurePolicy executionFailurePolicy,
            ExecutionFailureDraftingPolicy executionFailureDraftingPolicy,
            TestFailurePolicy testFailurePolicy,
            TestShardingPolicy testShardingPolicy,
            TargetOutputCapture targetOutputCapture,
            AZStd::optional<size_t> maxConcurrency = AZStd::nullopt);

        ~Runtime();

        // selected tests (excluded/included)
        TestSequenceResult RegularTestSequence(
            const AZStd::unordered_set<AZStd::string> suitesFilter,
            AZStd::optional<TestSequenceStartCallback> testSequenceStartCallback,
            AZStd::optional<TestSequenceEndCallback> testSequenceEndCallback,
            AZStd::optional<TestRunEndCallback> testRunEndCallback,
            AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
            AZStd::optional<AZStd::chrono::milliseconds> globalTimeout);

        // selected tests (excluded/included)
        // discarded tests
        // drafted tests
        TestSequenceResult ImpactAnalysisTestSequence(
            const ChangeList& changeList,
            TestPrioritizationPolicy testPrioritizationPolicy,
            AZStd::optional<ImpactAnalysisTestSequenceStartCallback> testSequenceStartCallback,
            AZStd::optional<TestSequenceEndCallback> testSequenceEndCallback,
            AZStd::optional<TestRunEndCallback> testRunEndCallback,
            AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
            AZStd::optional<AZStd::chrono::milliseconds> globalTimeout);

        // selected tests (excluded/included)
        // discarded tests (excluded/included)
        // drafted tests
        TestSequenceResult SafeImpactAnalysisTestSequence(
            const ChangeList& changeList,
            const AZStd::unordered_set<AZStd::string> suitesFilter,
            TestPrioritizationPolicy testPrioritizationPolicy,
            AZStd::optional<SafeImpactAnalysisTestSequenceStartCallback> testSequenceStartCallback,
            AZStd::optional<TestSequenceEndCallback> testSequenceEndCallback,
            AZStd::optional<TestRunEndCallback> testRunEndCallback,
            AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
            AZStd::optional<AZStd::chrono::milliseconds> globalTimeout);

        // selected tests (excluded/included)
        TestSequenceResult SeededTestSequence(
            AZStd::optional<TestSequenceStartCallback> testSequenceStartCallback,
            AZStd::optional<TestSequenceEndCallback> testSequenceEndCallback,
            AZStd::optional<TestRunEndCallback> testRunEndCallback,
            AZStd::optional<AZStd::chrono::milliseconds> globalTimeout);

        bool HasImpactAnalysisData() const;

    private:
        RuntimeConfig m_config;
        ExecutionFailurePolicy m_executionFailurePolicy;
        ExecutionFailureDraftingPolicy m_executionFailureDraftingPolicy;
        TestFailurePolicy m_testFailurePolicy;
        TestShardingPolicy m_testShardingPolicy;
        TargetOutputCapture m_targetOutputCapture;
        size_t m_maxConcurrency;
        AZStd::unique_ptr<DynamicDependencyMap> m_dynamicDependencyMap;
    };
}
