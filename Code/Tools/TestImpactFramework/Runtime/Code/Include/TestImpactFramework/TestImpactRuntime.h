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
#include <TestImpactFramework/TestImpactTestEnginePolicy.h>
#include <TestImpactFramework/TestImpactTest.h>
#include <TestImpactFramework/TestImpactFailureReport.h>

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

    //! Policy for reattempting the execution of test targets that failed to execute in previous runs.
    enum class ExecutionFailureDraftingPolicy
    {
        Never, //!< Do not attempt to execute historic execution failures.
        Always //!< Reattempt the exectution of historic execution failures.
    };

    //! Policy for prioritizing selected tests.
    enum class TestPrioritizationPolicy
    {
        None, //!< Do not attempt any test prioritization.
        DependencyLocality //!< Prioritize test targets according to the locality of the production targets they cover in the build dependency graph.
    };

    //! Callback for a test sequence that isn't using test impact analysis.
    //! @param tests The tests that will be run for this sequence.
    using TestSequenceStartCallback = AZStd::function<void(TestSelection&& tests)>;

    //! Callback for a test sequence using test impact analysis.
    //! @param selectedTests The tests that have been selected for this run by test impact analysis.
    //! @param discardedTests The tests that have been rejected for this run by test impact analysis. 
    //! @param draftedTests The tests that have been drafted in for this run due to requirements outside of test impact analysis
    //! (e.g. test targets that have been added to the repository since the last test impact analysis sequence or test that failed
    //! to execute previously).
    //! These tests will be run with coverage instrumentation.
    //! @note discardedTests and draftedTests may contain overlapping tests.
    using ImpactAnalysisTestSequenceStartCallback = AZStd::function<void(
        TestSelection&& selectedTests,
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
        TestSelection&& selectedTests,
        TestSelection&& discardedTests,
        AZStd::vector<AZStd::string>&& draftedTests)>;

    // reports:
    // execute failure: name, command string
    // runner failure (AzTest, OpenCppCoverage, gtest): name, command string, return code
    // test failure: name, failing tests grouped by suite

    //! Callback for a test sequence ending.
    //! @param testRuns The test results of all test target runs.
    using TestSequenceEndCallback = AZStd::function<void(FailureReport&& failureReport, AZStd::chrono::milliseconds duration)>;

    //! Callback for a test sequence ending.
    //! @param testRuns The test results of all test target runs.
    using ImpactAnalysisTestSequenceEndCallback = AZStd::function<void(ImpactAnalysisFailureReport&& failureReport, AZStd::chrono::milliseconds duration)>;

    //! Callback for completed tests.
    //! test The test that has completed.
    using TestCompleteCallback = AZStd::function<void(Test&& test)>;

    //!
    class Runtime
    {
    public:
        Runtime(
            RuntimeConfig&& config,
            ExecutionFailurePolicy executionFailurePolicy,
            ExecutionFailureDraftingPolicy executionFailureDraftingPolicy,
            TestFailurePolicy testFailurePolicy,
            IntegrityFailurePolicy integrationFailurePolicy,
            TestShardingPolicy testShardingPolicy,
            TargetOutputCapture targetOutputCapture,
            AZStd::optional<size_t> maxConcurrency = AZStd::nullopt);

        ~Runtime();

        TestSequenceResult RegularTestSequence(
            const AZStd::unordered_set<AZStd::string> suitesFilter,
            AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
            AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
            AZStd::optional<TestSequenceStartCallback> testSequenceStartCallback,
            AZStd::optional<TestSequenceEndCallback> testSequenceEndCallback,
            AZStd::optional<TestCompleteCallback> testRunEndCallback);

        TestSequenceResult ImpactAnalysisTestSequence(
            const ChangeList& changeList,
            TestPrioritizationPolicy testPrioritizationPolicy,
            AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
            AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
            AZStd::optional<ImpactAnalysisTestSequenceStartCallback> testSequenceStartCallback,
            AZStd::optional<ImpactAnalysisTestSequenceEndCallback> testSequenceEndCallback,
            AZStd::optional<TestCompleteCallback> testRunEndCallback);

        TestSequenceResult SafeImpactAnalysisTestSequence(
            const ChangeList& changeList,
            const AZStd::unordered_set<AZStd::string> suitesFilter,
            TestPrioritizationPolicy testPrioritizationPolicy,
            AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
            AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
            AZStd::optional<SafeImpactAnalysisTestSequenceStartCallback> testSequenceStartCallback,
            AZStd::optional<ImpactAnalysisTestSequenceEndCallback> testSequenceEndCallback,
            AZStd::optional<TestCompleteCallback> testRunEndCallback);

        TestSequenceResult SeededTestSequence(
            AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
            AZStd::optional<TestSequenceStartCallback> testSequenceStartCallback,
            AZStd::optional<TestSequenceEndCallback> testSequenceEndCallback,
            AZStd::optional<TestCompleteCallback> testRunEndCallback);

        bool HasImpactAnalysisData() const;

    private:
        RuntimeConfig m_config;
        ExecutionFailurePolicy m_executionFailurePolicy;
        ExecutionFailureDraftingPolicy m_executionFailureDraftingPolicy;
        TestFailurePolicy m_testFailurePolicy;
        IntegrityFailurePolicy m_integrationFailurePolicy;
        TestShardingPolicy m_testShardingPolicy;
        TargetOutputCapture m_targetOutputCapture;
        size_t m_maxConcurrency;
        AZStd::unique_ptr<DynamicDependencyMap> m_dynamicDependencyMap;
        AZStd::unique_ptr<TestEngine> m_testEngine;
        AZStd::unordered_set<const TestTarget*> m_testTargetExcludeList;
        AZStd::unordered_set<const TestTarget*> m_testTargetShardList;
    };
}
