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
            const AZStd::vector<TestRun>& GetPassingTests() const;

            //! Returns the set of test runs that executed successfully but had one or more failing tests.
            const AZStd::vector<TestRunWithTestFailures>& GetFailingTests() const;

            //! Returns the set of test runs that failed to execute.
            const AZStd::vector<TestRun>& GetExecutionFailureTests() const;

            //! Returns the set of test runs that executed successfully but were terminated prematurely due to timing out.
            const AZStd::vector<TestRun>& GetTimedOutTests() const;

            //! Returns the set of test runs that were queued up for execution but did not get the opportunity to execute.
            const AZStd::vector<TestRun>& GetUnexecutedTests() const;
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

        //! Report detailing a test run sequence of selected tests.
        class SequenceReport
        {
        public:
            //! Constructs the report for a sequence of selected tests.
            //! @param suiteType The suite from which the tests have been selected from.
            //! @param selectedTests The target names of the selected tests.
            //! @param selectedTestRunReport The report for the set of selected test runs.
            SequenceReport(SuiteType suiteType, const TestRunSelection& selectedTests, TestRunReport&& selectedTestRunReport);

            //! Returns the tests selected for running in the sequence.
            TestRunSelection GetSelectedTests() const;

            //! Returns the report for the selected test runs.
            TestRunReport GetSelectedTestRunReport() const;

            //! Returns the start time of the sequence.
            AZStd::chrono::high_resolution_clock::time_point GetStartTime() const;

            //! Returns the end time of the sequence.
            AZStd::chrono::high_resolution_clock::time_point GetEndTime() const;

            //! Returns the result of the sequence.
            virtual TestSequenceResult GetResult() const;

            //! Returns the entire duration the sequence took from start to finish.
            virtual AZStd::chrono::milliseconds GetDuration() const;

            //! Get the total number of tests in the sequence that passed.
            virtual size_t GetTotalNumPassingTests() const;

            //! Get the total number of tests in the sequence that contain one or more test failures.
            virtual size_t GetTotalNumFailingTests() const;

            //! Get the total number of tests in the sequence that timed out whilst in flight.
            virtual size_t GetTotalNumTimedOutTests() const;

            //! Get the total number of tests in the sequence that were queued for execution but did not get the oppurtunity to execute.
            virtual size_t GetTotalNumUnexecutedTests() const;

        private:
            SuiteType m_suite;
            TestRunSelection m_selectedTests;
            TestRunReport m_selectedTestRunReport;
        };

        //! Report detailing a test run sequence of selected and drafted tests.
        class DraftingSequenceReport
            : public SequenceReport
        {
        public:
            //! Constructs the report for a sequence of selected and drafted tests.
            //! @param suiteType The suite from which the tests have been selected from.
            //! @param selectedTests The target names of the selected tests.
            //! @param draftedTests The target names of the drafted tests.
            //! @param selectedTestRunReport The report for the set of selected test runs.
            //! @param draftedTestRunReport The report for the set of drafted test runs.
            DraftingSequenceReport(
                SuiteType suiteType,
                const TestRunSelection& selectedTests,
                const AZStd::vector<AZStd::string>& draftedTests,
                TestRunReport&& selectedTestRunReport,
                TestRunReport&& draftedTestRunReport);

            // SequenceReport overrides ...
            TestSequenceResult GetResult() const override;
            AZStd::chrono::milliseconds GetDuration() const override;
            size_t GetTotalNumPassingTests() const override;
            size_t GetTotalNumFailingTests() const override;
            size_t GetTotalNumTimedOutTests() const override;
            size_t GetTotalNumUnexecutedTests() const override;

             //! Returns the tests drafted for running in the sequence.
            const AZStd::vector<AZStd::string>& GetDraftedTests() const;

            //! Returns the report for the drafted test runs.
            TestRunReport GetDraftedTestRunReport() const;

        private:
            AZStd::vector<AZStd::string> m_draftedTests;
            TestRunReport m_draftedTestRunReport;
        };

        //! Report detailing an impact analysis sequence of selected, discarded and drafted tests.
        class ImpactAnalysisSequenceReport
            : public DraftingSequenceReport
        {
        public:
            //! Constructs the report for a sequence of selected and drafted tests.
            //! @param suiteType The suite from which the tests have been selected from.
            //! @param selectedTests The target names of the selected tests.
            //! @param discardedTests The target names of the discarded tests.
            //! @param draftedTests The target names of the drafted tests.
            //! @param selectedTestRunReport The report for the set of selected test runs.
            //! @param draftedTestRunReport The report for the set of drafted test runs.
            ImpactAnalysisSequenceReport(
                SuiteType suiteType,
                const TestRunSelection& selectedTests,
                const AZStd::vector<AZStd::string>& discardedTests,
                const AZStd::vector<AZStd::string>& draftedTests,
                TestRunReport&& selectedTestRunReport,
                TestRunReport&& draftedTestRunReport);

            //! Returns the tests discarded from running in the sequence.
            const AZStd::vector<AZStd::string>& GetDiscardedTests() const;
        private:
            AZStd::vector<AZStd::string> m_discardedTests;
        };

        //! Report detailing an impact analysis sequence of selected, discarded and drafted tests.
        class SafeImpactAnalysisSequenceReport
            : public DraftingSequenceReport
        {
        public:
            //! Constructs the report for a sequence of selected and drafted tests.
            //! @param suiteType The suite from which the tests have been selected from.
            //! @param selectedTests The target names of the selected tests.
            //! @param discardedTests The target names of the discarded tests.
            //! @param draftedTests The target names of the drafted tests.
            //! @param selectedTestRunReport The report for the set of selected test runs.
            //! @param discardedTestRunReport The report for the set of discarded test runs.
            //! @param draftedTestRunReport The report for the set of drafted test runs.
            SafeImpactAnalysisSequenceReport(
                SuiteType suiteType,
                const TestRunSelection& selectedTests,
                const TestRunSelection& discardedTests,
                const AZStd::vector<AZStd::string>& draftedTests,
                TestRunReport&& selectedTestRunReport,
                TestRunReport&& discardedTestRunReport,
                TestRunReport&& draftedTestRunReport);

            // DraftingSequenceReport overrides ...
            TestSequenceResult GetResult() const override;
            AZStd::chrono::milliseconds GetDuration() const override;
            size_t GetTotalNumPassingTests() const override;
            size_t GetTotalNumFailingTests() const override;
            size_t GetTotalNumTimedOutTests() const override;
            size_t GetTotalNumUnexecutedTests() const override;

            //! Returns the report for the discarded test runs.
            const TestRunSelection GetDiscardedTests() const;

            //! Returns the report for the discarded test runs.
            TestRunReport GetDiscardedTestRunReport() const;

        private:
            TestRunSelection m_discardedTests;
            TestRunReport m_discardedTestRunReport;
        };
    } // namespace Client
} // namespace TestImpact
