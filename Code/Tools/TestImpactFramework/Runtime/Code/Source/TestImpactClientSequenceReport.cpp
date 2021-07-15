/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactFramework/TestImpactClientSequenceReport.h>

namespace TestImpact
{
    namespace Client
    {
        //! Calculates the final sequence result for a composite of multiple sequences.
        TestSequenceResult CalculateMultiTestSequenceResult(const AZStd::vector<TestSequenceResult>& results)
        {
            // Order of precedence:
            // 1. TestSequenceResult::Failure
            // 2. TestSequenceResult::Timeout
            // 3. TestSequenceResult::Success

            if (const auto it = AZStd::find(results.begin(), results.end(), TestSequenceResult::Failure);
                it != results.end())
            {
                return TestSequenceResult::Failure;
            }
            
            if (const auto it = AZStd::find(results.begin(), results.end(), TestSequenceResult::Timeout);
                it != results.end())
            {
                return TestSequenceResult::Timeout;
            }

            return TestSequenceResult::Success;
        }

        TestRunReport::TestRunReport(
            TestSequenceResult result,
            AZStd::chrono::high_resolution_clock::time_point startTime,
            AZStd::chrono::milliseconds duration,
            AZStd::vector<TestRun>&& passingTests,
            AZStd::vector<TestRunWithTestFailures>&& failingTests,
            AZStd::vector<TestRun>&& executionFailureTests,
            AZStd::vector<TestRun>&& timedOutTests,
            AZStd::vector<TestRun>&& unexecutedTests)
            : m_startTime(startTime)
            , m_result(result)
            , m_duration(duration)
            , m_passingTests(AZStd::move(passingTests))
            , m_failingTests(AZStd::move(failingTests))
            , m_executionFailureTests(AZStd::move(executionFailureTests))
            , m_timedOutTests(AZStd::move(timedOutTests))
            , m_unexecutedTests(AZStd::move(unexecutedTests))
        {
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

        size_t TestRunReport::GetNumPassingTests() const
        {
            return m_passingTests.size();
        }

        size_t TestRunReport::GetNumFailingTests() const
        {
            return m_failingTests.size();
        }

        size_t TestRunReport::GetNumTimedOutTests() const
        {
            return m_timedOutTests.size();
        }

        size_t TestRunReport::GetNumUnexecutedTests() const
        {
            return m_unexecutedTests.size();
        }

        const AZStd::vector<TestRun>& TestRunReport::GetPassingTests() const
        {
            return m_passingTests;
        }

        const AZStd::vector<TestRunWithTestFailures>& TestRunReport::GetFailingTests() const
        {
            return m_failingTests;
        }

        const AZStd::vector<TestRun>& TestRunReport::GetExecutionFailureTests() const
        {
            return m_executionFailureTests;
        }

        const AZStd::vector<TestRun>& TestRunReport::GetTimedOutTests() const
        {
            return m_timedOutTests;
        }

        const AZStd::vector<TestRun>& TestRunReport::GetUnexecutedTests() const
        {
            return m_unexecutedTests;
        }

        SequenceReport::SequenceReport(SuiteType suiteType, const TestRunSelection& selectedTests, TestRunReport&& selectedTestRunReport)
            : m_suite(suiteType)
            , m_selectedTests(selectedTests)
            , m_selectedTestRunReport(AZStd::move(selectedTestRunReport))
        {
        }

        TestSequenceResult SequenceReport::GetResult() const
        {
            return m_selectedTestRunReport.GetResult();
        }

        AZStd::chrono::high_resolution_clock::time_point SequenceReport::GetStartTime() const
        {
            return m_selectedTestRunReport.GetStartTime();
        }

        AZStd::chrono::high_resolution_clock::time_point SequenceReport::GetEndTime() const
        {
            return GetStartTime() + GetDuration();
        }

        AZStd::chrono::milliseconds SequenceReport::GetDuration() const
        {
            return m_selectedTestRunReport.GetDuration();
        }

        TestRunSelection SequenceReport::GetSelectedTests() const
        {
            return m_selectedTests;
        }

        TestRunReport SequenceReport::GetSelectedTestRunReport() const
        {
            return m_selectedTestRunReport;
        }

        size_t SequenceReport::GetTotalNumPassingTests() const
        {
            return m_selectedTestRunReport.GetNumPassingTests();
        }

        size_t SequenceReport::GetTotalNumFailingTests() const
        {
            return m_selectedTestRunReport.GetNumFailingTests();
        }

        size_t SequenceReport::GetTotalNumTimedOutTests() const
        {
            return m_selectedTestRunReport.GetNumTimedOutTests();
        }

        size_t SequenceReport::GetTotalNumUnexecutedTests() const
        {
            return m_selectedTestRunReport.GetNumUnexecutedTests();
        }

        DraftingSequenceReport::DraftingSequenceReport(
            SuiteType suiteType,
            const TestRunSelection& selectedTests,
            const AZStd::vector<AZStd::string>& draftedTests,
            TestRunReport&& selectedTestRunReport,
            TestRunReport&& draftedTestRunReport)
            : SequenceReport(suiteType, selectedTests, AZStd::move(selectedTestRunReport))
            , m_draftedTests(draftedTests)
            , m_draftedTestRunReport(AZStd::move(draftedTestRunReport))
        {
        }

        TestSequenceResult DraftingSequenceReport::GetResult() const
        {
            return CalculateMultiTestSequenceResult({SequenceReport::GetResult(), m_draftedTestRunReport.GetResult()});
        }

        AZStd::chrono::milliseconds DraftingSequenceReport::GetDuration() const
        {
            return SequenceReport::GetDuration() + m_draftedTestRunReport.GetDuration();
        }

        size_t DraftingSequenceReport::GetTotalNumPassingTests() const
        {
            return SequenceReport::GetTotalNumPassingTests() + m_draftedTestRunReport.GetNumPassingTests();
        }

        size_t DraftingSequenceReport::GetTotalNumFailingTests() const
        {
            return SequenceReport::GetTotalNumFailingTests() + m_draftedTestRunReport.GetNumFailingTests();
        }

        size_t DraftingSequenceReport::GetTotalNumTimedOutTests() const
        {
            return SequenceReport::GetTotalNumTimedOutTests() + m_draftedTestRunReport.GetNumTimedOutTests();
        }

        size_t DraftingSequenceReport::GetTotalNumUnexecutedTests() const
        {
            return SequenceReport::GetTotalNumUnexecutedTests() + m_draftedTestRunReport.GetNumUnexecutedTests();
        }

        const AZStd::vector<AZStd::string>& DraftingSequenceReport::GetDraftedTests() const
        {
            return m_draftedTests;
        }

        TestRunReport DraftingSequenceReport::GetDraftedTestRunReport() const
        {
            return m_draftedTestRunReport;
        }

        ImpactAnalysisSequenceReport::ImpactAnalysisSequenceReport(
            SuiteType suiteType,
            const TestRunSelection& selectedTests,
            const AZStd::vector<AZStd::string>& discardedTests,
            const AZStd::vector<AZStd::string>& draftedTests,
            TestRunReport&& selectedTestRunReport,
            TestRunReport&& draftedTestRunReport)
            : DraftingSequenceReport(
                  suiteType,
                  selectedTests,
                  draftedTests,
                  AZStd::move(selectedTestRunReport),
                  AZStd::move(draftedTestRunReport))
            , m_discardedTests(discardedTests)
        {
        }

        const AZStd::vector<AZStd::string>& ImpactAnalysisSequenceReport::GetDiscardedTests() const
        {
            return m_discardedTests;
        }

        SafeImpactAnalysisSequenceReport::SafeImpactAnalysisSequenceReport(
            SuiteType suiteType,
            const TestRunSelection& selectedTests,
            const TestRunSelection& discardedTests,
            const AZStd::vector<AZStd::string>& draftedTests,
            TestRunReport&& selectedTestRunReport,
            TestRunReport&& discardedTestRunReport,
            TestRunReport&& draftedTestRunReport)
            : DraftingSequenceReport(
                  suiteType,
                  selectedTests,
                  draftedTests,
                  AZStd::move(selectedTestRunReport),
                  AZStd::move(draftedTestRunReport))
            , m_discardedTests(discardedTests)
            , m_discardedTestRunReport(AZStd::move(discardedTestRunReport))
        {
        }

        TestSequenceResult SafeImpactAnalysisSequenceReport::GetResult() const
        {
            return CalculateMultiTestSequenceResult({ DraftingSequenceReport::GetResult(), m_discardedTestRunReport.GetResult() });
        }

        AZStd::chrono::milliseconds SafeImpactAnalysisSequenceReport::GetDuration() const
        {
            return DraftingSequenceReport::GetDuration() + m_discardedTestRunReport.GetDuration();
        }

        size_t SafeImpactAnalysisSequenceReport::GetTotalNumPassingTests() const
        {
            return DraftingSequenceReport::GetTotalNumPassingTests() + m_discardedTestRunReport.GetNumPassingTests();
        }

        size_t SafeImpactAnalysisSequenceReport::GetTotalNumFailingTests() const
        {
            return DraftingSequenceReport::GetTotalNumFailingTests() + m_discardedTestRunReport.GetNumFailingTests();
        }

        size_t SafeImpactAnalysisSequenceReport::GetTotalNumTimedOutTests() const
        {
            return DraftingSequenceReport::GetTotalNumTimedOutTests() + m_discardedTestRunReport.GetNumTimedOutTests();
        }

        size_t SafeImpactAnalysisSequenceReport::GetTotalNumUnexecutedTests() const
        {
            return DraftingSequenceReport::GetTotalNumUnexecutedTests() + m_discardedTestRunReport.GetNumUnexecutedTests();
        }
        
        const TestRunSelection SafeImpactAnalysisSequenceReport::GetDiscardedTests() const
        {
            return m_discardedTests;
        }

        TestRunReport SafeImpactAnalysisSequenceReport::GetDiscardedTestRunReport() const
        {
            return m_discardedTestRunReport;
        }
    } // namespace Client
} // namespace TestImpact
