/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactClientTestRun.h>
#include <TestImpactFramework/TestImpactClientTestSelection.h>
#include <TestImpactFramework/TestImpactTestSequence.h>

namespace TestImpact
{
    namespace Client
    {
        //! Report detailing the result and duration of a given set of test runs along with the details of each individual test run.
        class TestRunReport
        {
        public:
            //! Constructs the report for the given set of test runs that were run together in the same set.
            //! @param result The result of this set of test runs.
            //! @param startTime The time point his set of test runs started.
            //! @param duration The duration this set of test runs took to complete.
            //! @param passingTests The set of test runs that executed successfully with no failing tests.
            //! @param failing tests The set of test runs that executed successfully but had one or more failing tests.
            //! @param executionFailureTests The set of test runs that failed to execute.
            //! @param timedOutTests The set of test runs that executed successfully but were terminated prematurely due to timing out.
            //! @param unexecutedTests The set of test runs that were queued up for execution but did not get the opportunity to execute.
            TestRunReport(
                TestSequenceResult result,
                AZStd::chrono::high_resolution_clock::time_point startTime,
                AZStd::chrono::milliseconds duration,
                AZStd::vector<TestRun>&& passingTests,
                AZStd::vector<TestRunWithTestFailures>&& failingTests,
                AZStd::vector<TestRun>&& executionFailureTests,
                AZStd::vector<TestRun>&& timedOutTests,
                AZStd::vector<TestRun>&& unexecutedTests);

            //! Returns the result of this sequence of test runs.
            TestSequenceResult GetResult() const;

            //! Returns the time this sequence of test runs started relative to T0.
            AZStd::chrono::high_resolution_clock::time_point GetStartTime() const;

            //! Returns the time this sequence of test runs ended relative to T0.
            AZStd::chrono::high_resolution_clock::time_point GetEndTime() const;

            //! Returns the duration this sequence of test runs took to complete.
            AZStd::chrono::milliseconds GetDuration() const;

            //! Returns the number of passing test runs.
            size_t GetNumPassingTests() const;

            //! Returns the number of failing test runs.
            size_t GetNumFailingTests() const;

            //! Returns the number of timed out test runs.
            size_t GetNumTimedOutTests() const;

            //! Returns the number of unexecuted test runs.
            size_t GetNumUnexecutedTests() const;

            //! Returns the set of test runs that executed successfully with no failing tests.
            AZStd::vector<TestRun> GetPassingTests() const;

            //! Returns the set of test runs that executed successfully but had one or more failing tests.
            AZStd::vector<TestRunWithTestFailures> GetFailingTests() const;

            //! Returns the set of test runs that failed to execute.
            AZStd::vector<TestRun> GetExecutionFailureTests() const;

            //! Returns the set of test runs that executed successfully but were terminated prematurely due to timing out.
            AZStd::vector<TestRun> GetTimedOutTests() const;

            //! Returns the set of test runs that were queued up for execution but did not get the opportunity to execute.
            AZStd::vector<TestRun> GetUnexecutedTests() const;
        private:
            TestSequenceResult m_result;
            AZStd::chrono::high_resolution_clock::time_point m_startTime;
            AZStd::chrono::milliseconds m_duration;
            AZStd::vector<TestRun> m_passingTests;
            AZStd::vector<TestRunWithTestFailures> m_failingTests;
            AZStd::vector<TestRun> m_executionFailureTests;
            AZStd::vector<TestRun> m_timedOutTests;
            AZStd::vector<TestRun> m_unexecutedTests;
        };

        //! Report detailing the test selection and test run report of a typical test run sequence.
        class SequenceReport
        {
        public:
            SequenceReport(SuiteType suiteType, const TestRunSelection& selectedTests, TestRunReport&& selectedTestRunReport);

            TestRunSelection GetSelectedTests() const;
            TestRunReport GetSelectedTestRunReport() const;

            AZStd::chrono::high_resolution_clock::time_point GetStartTime() const;
            AZStd::chrono::high_resolution_clock::time_point GetEndTime() const;

            virtual TestSequenceResult GetResult() const;
            virtual AZStd::chrono::milliseconds GetDuration() const;
            virtual size_t GetTotalNumPassingTests() const;
            virtual size_t GetTotalNumFailingTests() const;
            virtual size_t GetTotalNumTimedOutTests() const;
            virtual size_t GetTotalNumUnexecutedTests() const;

        private:
            SuiteType m_suite;
            TestRunSelection m_selectedTests;
            TestRunReport m_selectedTestRunReport;
        };

        class DraftingSequenceReport
            : public SequenceReport
        {
        public:
            DraftingSequenceReport(
                SuiteType suiteType,
                const TestRunSelection& selectedTests,
                const AZStd::vector<AZStd::string>& draftedTests,
                TestRunReport&& selectedTestRunReport,
                TestRunReport&& draftedTestRunReport);

            TestSequenceResult GetResult() const override;
            AZStd::chrono::milliseconds GetDuration() const override;
            size_t GetTotalNumPassingTests() const override;
            size_t GetTotalNumFailingTests() const override;
            size_t GetTotalNumTimedOutTests() const override;
            size_t GetTotalNumUnexecutedTests() const override;

            const AZStd::vector<AZStd::string>& GetDraftedTests() const;
            TestRunReport GetDraftedTestRunReport() const;

        private:
            AZStd::vector<AZStd::string> m_draftedTests;
            TestRunReport m_draftedTestRunReport;
        };

        class ImpactAnalysisSequenceReport
            : public DraftingSequenceReport
        {
        public:
            ImpactAnalysisSequenceReport(
                SuiteType suiteType,
                const TestRunSelection& selectedTests,
                const AZStd::vector<AZStd::string>& discardedTests,
                const AZStd::vector<AZStd::string>& draftedTests,
                TestRunReport&& selectedTestRunReport,
                TestRunReport&& draftedTestRunReport);

            const AZStd::vector<AZStd::string>& GetDiscardedTests() const;
        private:
            AZStd::vector<AZStd::string> m_discardedTests;
        };

        class SafeImpactAnalysisSequenceReport
            : public DraftingSequenceReport
        {
        public:
            SafeImpactAnalysisSequenceReport(
                SuiteType suiteType,
                const TestRunSelection& selectedTests,
                const TestRunSelection& discardedTests,
                const AZStd::vector<AZStd::string>& draftedTests,
                TestRunReport&& selectedTestRunReport,
                TestRunReport&& discardedTestRunReport,
                TestRunReport&& draftedTestRunReport);

            TestSequenceResult GetResult() const override;
            AZStd::chrono::milliseconds GetDuration() const override;
            size_t GetTotalNumPassingTests() const override;
            size_t GetTotalNumFailingTests() const override;
            size_t GetTotalNumTimedOutTests() const override;
            size_t GetTotalNumUnexecutedTests() const override;

            const TestRunSelection GetDiscardedTests() const;
            TestRunReport GetDiscardedTestRunReport() const;

        private:
            TestRunSelection m_discardedTests;
            TestRunReport m_discardedTestRunReport;
        };
    } // namespace Client
} // namespace TestImpact
