/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactFramework/TestImpactClientSequenceReport.h>

namespace TestImpact
{
    namespace Client
    {
        TestSequenceResult CalculateMultiTestSequenceResult(const AZStd::vector<TestSequenceResult>& results)
        {
            // Order of precedence:
            // 1. TestSequenceResult::Failure
            // 2. TestSequenceResult::Timeout
            // 3. TestSequenceResult::Success

            if (const auto it = AZStd::find(results.begin(), results.end(), TestSequenceResult::Failure); it != results.end())
            {
                return TestSequenceResult::Failure;
            }

            if (const auto it = AZStd::find(results.begin(), results.end(), TestSequenceResult::Timeout); it != results.end())
            {
                return TestSequenceResult::Timeout;
            }

            return TestSequenceResult::Success;
        }

        TestRunReport::TestRunReport(
            TestSequenceResult result,
            AZStd::chrono::high_resolution_clock::time_point startTime,
            AZStd::chrono::milliseconds duration,
            AZStd::vector<PassingTestRun>&& passingTestRuns,
            AZStd::vector<FailingTestRun>&& failingTestRuns,
            AZStd::vector<TestRunWithExecutionFailure>&& executionFailureTestRuns,
            AZStd::vector<TimedOutTestRun>&& timedOutTestRuns,
            AZStd::vector<UnexecutedTestRun>&& unexecutedTestRuns)
            : m_startTime(startTime)
            , m_result(result)
            , m_duration(duration)
            , m_passingTestRuns(AZStd::move(passingTestRuns))
            , m_failingTestRuns(AZStd::move(failingTestRuns))
            , m_executionFailureTestRuns(AZStd::move(executionFailureTestRuns))
            , m_timedOutTestRuns(AZStd::move(timedOutTestRuns))
            , m_unexecutedTestRuns(AZStd::move(unexecutedTestRuns))
        {
            for (const auto& failingTestRun : m_failingTestRuns)
            {
                m_totalNumPassingTests += failingTestRun.GetTotalNumPassingTests();
                m_totalNumFailingTests += failingTestRun.GetTotalNumFailingTests();
                m_totalNumDisabledTests += failingTestRun.GetTotalNumDisabledTests();
            }

            for (const auto& passingTestRun : m_passingTestRuns)
            {
                m_totalNumPassingTests += passingTestRun.GetTotalNumPassingTests();
                m_totalNumDisabledTests += passingTestRun.GetTotalNumDisabledTests();
            }
        }

        TestSequenceResult TestRunReport::GetResult() const
        {
            return m_result;
        }

        AZStd::chrono::high_resolution_clock::time_point TestRunReport::GetStartTime() const
        {
            return m_startTime;
        }

        AZStd::chrono::high_resolution_clock::time_point TestRunReport::GetEndTime() const
        {
            return m_startTime + m_duration;
        }

        AZStd::chrono::milliseconds TestRunReport::GetDuration() const
        {
            return m_duration;
        }

        size_t TestRunReport::GetTotalNumTestRuns() const
        {
            return
                GetNumPassingTestRuns() +
                GetNumFailingTestRuns() +
                GetNumExecutionFailureTestRuns() +
                GetNumTimedOutTestRuns() +
                GetNumUnexecutedTestRuns();
        }

        size_t TestRunReport::GetNumPassingTestRuns() const
        {
            return m_passingTestRuns.size();
        }

        size_t TestRunReport::GetNumFailingTestRuns() const
        {
            return m_failingTestRuns.size();
        }

        size_t TestRunReport::GetNumExecutionFailureTestRuns() const
        {
            return m_executionFailureTestRuns.size();
        }

        size_t TestRunReport::TestRunReport::GetNumTimedOutTestRuns() const
        {
            return m_timedOutTestRuns.size();
        }

        size_t TestRunReport::GetNumUnexecutedTestRuns() const
        {
            return m_unexecutedTestRuns.size();
        }

        const AZStd::vector<PassingTestRun>& TestRunReport::GetPassingTestRuns() const
        {
            return m_passingTestRuns;
        }

        const AZStd::vector<FailingTestRun>& TestRunReport::GetFailingTestRuns() const
        {
            return m_failingTestRuns;
        }

        const AZStd::vector<TestRunWithExecutionFailure>& TestRunReport::GetExecutionFailureTestRuns() const
        {
            return m_executionFailureTestRuns;
        }

        const AZStd::vector<TimedOutTestRun>& TestRunReport::GetTimedOutTestRuns() const
        {
            return m_timedOutTestRuns;
        }

        const AZStd::vector<UnexecutedTestRun>& TestRunReport::GetUnexecutedTestRuns() const
        {
            return m_unexecutedTestRuns;
        }

        size_t TestRunReport::GetTotalNumPassingTests() const
        {
            return m_totalNumPassingTests;
        }

        size_t TestRunReport::GetTotalNumFailingTests() const
        {
            return m_totalNumFailingTests;
        }

        size_t TestRunReport::GetTotalNumDisabledTests() const
        {
            return m_totalNumDisabledTests;
        }

        ImpactAnalysisSequenceReport::ImpactAnalysisSequenceReport(
            size_t maxConcurrency,
            AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
            AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
            ImpactAnalysisSequencePolicyState policyState,
            SuiteType suiteType,
            TestRunSelection selectedTestRuns,
            AZStd::vector<AZStd::string> discardedTestRuns,
            AZStd::vector<AZStd::string> draftedTestRuns,
            TestRunReport&& selectedTestRunReport,
            TestRunReport&& draftedTestRunReport)
            : DraftingSequenceReportBase(
                maxConcurrency,
                AZStd::move(testTargetTimeout),
                AZStd::move(globalTimeout),
                AZStd::move(policyState),
                suiteType,
                AZStd::move(selectedTestRuns),
                AZStd::move(draftedTestRuns),
                AZStd::move(selectedTestRunReport),
                AZStd::move(draftedTestRunReport))
            , m_discardedTestRuns(discardedTestRuns)
        {
        }

        ImpactAnalysisSequenceReport::ImpactAnalysisSequenceReport(
            DraftingSequenceReportBase&& report, AZStd::vector<AZStd::string> discardedTestRuns)
            : DraftingSequenceReportBase(AZStd::move(report))
            , m_discardedTestRuns(AZStd::move(discardedTestRuns))
        {
        }

        const AZStd::vector<AZStd::string>& ImpactAnalysisSequenceReport::GetDiscardedTestRuns() const
        {
            return m_discardedTestRuns;
        }

        SafeImpactAnalysisSequenceReport::SafeImpactAnalysisSequenceReport(
            size_t maxConcurrency,
            AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
            AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
            SafeImpactAnalysisSequencePolicyState policyState,
            SuiteType suiteType,
            TestRunSelection selectedTestRuns,
            TestRunSelection discardedTestRuns,
            AZStd::vector<AZStd::string> draftedTestRuns,
            TestRunReport&& selectedTestRunReport,
            TestRunReport&& discardedTestRunReport,
            TestRunReport&& draftedTestRunReport)
            : DraftingSequenceReportBase(
                maxConcurrency,
                AZStd::move(testTargetTimeout),
                AZStd::move(globalTimeout),
                AZStd::move(policyState),
                suiteType,
                AZStd::move(selectedTestRuns),
                AZStd::move(draftedTestRuns),
                AZStd::move(selectedTestRunReport),
                AZStd::move(draftedTestRunReport))
            , m_discardedTestRuns(discardedTestRuns)
            , m_discardedTestRunReport(AZStd::move(discardedTestRunReport))
        {
        }

        SafeImpactAnalysisSequenceReport::SafeImpactAnalysisSequenceReport(
            DraftingSequenceReportBase&& report, TestRunSelection discardedTestRuns, TestRunReport&& discardedTestRunReport)
            : DraftingSequenceReportBase(AZStd::move(report))
            , m_discardedTestRuns(AZStd::move(discardedTestRuns))
            , m_discardedTestRunReport(AZStd::move(discardedTestRunReport))
        {
        }

        TestSequenceResult SafeImpactAnalysisSequenceReport::GetResult() const
        {
            return CalculateMultiTestSequenceResult({ DraftingSequenceReportBase::GetResult(), m_discardedTestRunReport.GetResult() });
        }

        AZStd::chrono::milliseconds SafeImpactAnalysisSequenceReport::GetDuration() const
        {
            return DraftingSequenceReportBase::GetDuration() + m_discardedTestRunReport.GetDuration();
        }

        size_t SafeImpactAnalysisSequenceReport::GetTotalNumTestRuns() const
        {
            return DraftingSequenceReportBase::GetTotalNumTestRuns() + m_discardedTestRunReport.GetTotalNumTestRuns();
        }

        size_t SafeImpactAnalysisSequenceReport::GetTotalNumPassingTests() const
        {
            return DraftingSequenceReportBase::GetTotalNumPassingTests() + m_discardedTestRunReport.GetTotalNumPassingTests();
        }

        size_t SafeImpactAnalysisSequenceReport::GetTotalNumFailingTests() const
        {
            return DraftingSequenceReportBase::GetTotalNumFailingTests() + m_discardedTestRunReport.GetTotalNumFailingTests();
        }

        size_t SafeImpactAnalysisSequenceReport::GetTotalNumDisabledTests() const
        {
            return DraftingSequenceReportBase::GetTotalNumDisabledTests() + m_discardedTestRunReport.GetTotalNumDisabledTests();
        }

        size_t SafeImpactAnalysisSequenceReport::GetTotalNumPassingTestRuns() const
        {
            return DraftingSequenceReportBase::GetTotalNumPassingTestRuns() + m_discardedTestRunReport.GetNumPassingTestRuns();
        }

        size_t SafeImpactAnalysisSequenceReport::GetTotalNumFailingTestRuns() const
        {
            return DraftingSequenceReportBase::GetTotalNumFailingTestRuns() + m_discardedTestRunReport.GetNumFailingTestRuns();
        }

        size_t SafeImpactAnalysisSequenceReport::GetTotalNumExecutionFailureTestRuns() const
        {
            return DraftingSequenceReportBase::GetTotalNumExecutionFailureTestRuns() + m_discardedTestRunReport.GetNumExecutionFailureTestRuns();
        }

        size_t SafeImpactAnalysisSequenceReport::GetTotalNumTimedOutTestRuns() const
        {
            return DraftingSequenceReportBase::GetTotalNumTimedOutTestRuns() + m_discardedTestRunReport.GetNumTimedOutTestRuns();
        }

        size_t SafeImpactAnalysisSequenceReport::GetTotalNumUnexecutedTestRuns() const
        {
            return DraftingSequenceReportBase::GetTotalNumUnexecutedTestRuns() + m_discardedTestRunReport.GetNumUnexecutedTestRuns();
        }
        
        const TestRunSelection SafeImpactAnalysisSequenceReport::GetDiscardedTestRuns() const
        {
            return m_discardedTestRuns;
        }

        TestRunReport SafeImpactAnalysisSequenceReport::GetDiscardedTestRunReport() const
        {
            return m_discardedTestRunReport;
        }
    } // namespace Client
} // namespace TestImpact
