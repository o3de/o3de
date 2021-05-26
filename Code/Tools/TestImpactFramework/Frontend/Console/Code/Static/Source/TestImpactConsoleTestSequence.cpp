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

#include <TestImpactConsoleTestSequence.h>

#include <iostream>

namespace TestImpact
{
    namespace Console
    {
        TestSequence::TestSequence(const AZStd::unordered_set<AZStd::string>* suiteFilter)
            : m_suiteFilter(suiteFilter)
        {
        }

        // TestSequenceStartCallback
        void TestSequence::operator()(Client::TestRunSelection&& selectedTests)
        {
            ClearState();

            std::cout << "Test suite filter: [";
            if (m_suiteFilter->empty())
            {
                std::cout << "*";
            }
            else
            {
                size_t suiteId = 0;
                for (const auto& suite : *m_suiteFilter)
                {
                    std::cout << suite.c_str();
                    if (suiteId + 1 < m_suiteFilter->size())
                    {
                        std::cout << ", ";
                    }
                    suiteId++;
                }
            }
            std::cout << "]\n";

            m_numTests = selectedTests.GetNumIncludedTestRuns();
            std::cout << m_numTests << " tests selected, " << selectedTests.GetNumExcludedTestRuns() << " excluded.\n";
        }

        // ImpactAnalysisTestSequenceStartCallback
        void TestSequence::operator()(
            Client::TestRunSelection&& selectedTests,
            AZStd::vector<AZStd::string>&& discardedTests,
            AZStd::vector<AZStd::string>&& draftedTests)
        {
            ClearState();

            const float totalTests = selectedTests.GetTotalNumTests() + discardedTests.size();
            const float saving = (1.0 - (selectedTests.GetTotalNumTests() / totalTests)) * 100.0f;
            m_numTests = selectedTests.GetNumIncludedTestRuns() + draftedTests.size();
            std::cout << selectedTests.GetTotalNumTests() << " tests selected, " << discardedTests.size() << " tests discarded (" << saving << "% test saving)\n";
            std::cout << "Of which " << selectedTests.GetNumExcludedTestRuns() << " tests have been excluded and " << draftedTests.size() << " tests have been drafted.\n";
        }

        // SafeImpactAnalysisTestSequenceStartCallback
        void TestSequence::operator()(
            [[maybe_unused]]Client::TestRunSelection&& selectedTests,
            [[maybe_unused]]Client::TestRunSelection&& discardedTests,
            [[maybe_unused]]AZStd::vector<AZStd::string>&& draftedTests)
        {
            const float totalTests = selectedTests.GetTotalNumTests() + discardedTests.GetTotalNumTests();
            const float saving = (1.0 - (selectedTests.GetTotalNumTests() / totalTests)) * 100.0f;
            m_numTests = selectedTests.GetNumIncludedTestRuns() + draftedTests.size();
            std::cout << selectedTests.GetTotalNumTests() << " tests selected, " << discardedTests.GetTotalNumTests() << " tests discarded (" << saving << "% test saving)\n";
            std::cout << "Of which " << (selectedTests.GetNumExcludedTestRuns() + discardedTests.GetNumExcludedTestRuns()) << " tests have been excluded and " << draftedTests.size() << " tests have been drafted.\n";
        }

        // TestSequenceCompleteCallback
        void TestSequence::operator()(
            [[maybe_unused]]Client::SequenceFailure&& failureReport,
            [[maybe_unused]]AZStd::chrono::milliseconds duration)
        {
            std::cout << "Sequence completed in " << (duration.count() / 1000.f) << "s with";
            if (failureReport.GetExecutionFailures().size() || failureReport.GetTestRunFailures().size() || failureReport.GetTimedOutTests().size() || failureReport.GetUnexecutedTests().size())
            {
                std::cout << ":\n";
                std::cout << "\033[37;41m" << failureReport.GetTestRunFailures().size() << "\033[0m test failures\n";
                std::cout << "\033[37;41m" << failureReport.GetExecutionFailures().size() << "\033[0m execution failures\n";
                std::cout << "\033[37;41m" << failureReport.GetTimedOutTests().size() << "\033[0m test timeouts\n";
                std::cout << "\033[37;41m" << failureReport.GetUnexecutedTests().size() << "\033[0m unexecuted tests\n";

                if (!failureReport.GetTestRunFailures().empty())
                {
                    std::cout << "\nTest failures:\n";
                    for (const auto& testRunFailure : failureReport.GetTestRunFailures())
                    {
                        std::cout << "  " << testRunFailure.GetTargetName().c_str();
                        for (const auto& testCaseFailure : testRunFailure.GetTestCaseFailures())
                        {
                            std::cout << "." << testCaseFailure.GetName().c_str();
                            for (const auto& testFailure : testCaseFailure.GetTestFailures())
                            {
                                std::cout << "." << testFailure.GetName().c_str() << "\n";
                            }
                        }
                    }
                }

                if (!failureReport.GetExecutionFailures().empty())
                {
                    std::cout << "\nExecution failures:\n";
                    for (const auto& executionFailure : failureReport.GetExecutionFailures())
                    {
                        std::cout << "  " << executionFailure.GetTargetName().c_str() << "\n";
                        std::cout << executionFailure.GetCommandString().c_str() << "\n";
                    }
                }

                if (!failureReport.GetTimedOutTests().empty())
                {
                    std::cout << "\nTimed out tests:\n";
                    for (const auto& testTimeout : failureReport.GetTimedOutTests())
                    {
                        std::cout << "  " << testTimeout.GetTargetName().c_str() << "\n";
                    }
                }

                if (!failureReport.GetUnexecutedTests().empty())
                {
                    std::cout << "\nUnexecuted tests:\n";
                    for (const auto& unexecutedTest : failureReport.GetUnexecutedTests())
                    {
                        std::cout << "  " << unexecutedTest.GetTargetName().c_str() << "\n";
                    }
                }
            }
            else
            {
                std::cout << " \033[37;42m100% passes!\033[0m\n";
            }

            std::cout << "Updating and serializing the test impact analysis data, this may take a moment...\n";
        }

        // SafeTestSequenceCompleteCallback
        void TestSequence::operator()(
            [[maybe_unused]]Client::SequenceFailure&& selectedFailureReport,
            [[maybe_unused]]Client::SequenceFailure&& discardedFailureReport,
            [[maybe_unused]]AZStd::chrono::milliseconds duration)
        {

        }

        // TestRunCompleteCallback
        void TestSequence::operator()([[maybe_unused]] Client::TestRun&& test)
        {
            m_numTestsComplete++;
            const auto progress = AZStd::string::format("(%03u/%03u)", m_numTestsComplete, m_numTests, test.GetTargetName().c_str());

            AZStd::string result;
            switch (test.GetResult())
            {
            case Client::TestRunResult::AllTestsPass:
            {
                result = "\033[37;42mPASS\033[0m";
                break;
            }
            case Client::TestRunResult::FailedToExecute:
            {
                result = "\033[37;41mEXEC\033[0m";
                break;
            }
            case Client::TestRunResult::NotRun:
            {
                result = "\033[37;43mSKIP\033[0m";
                break;
            }
            case Client::TestRunResult::TestFailures:
            {
                result = "\033[37;41mFAIL\033[0m";
                break;
            }
            case Client::TestRunResult::Timeout:
            {
                result = "\033[37;41mTIME\033[0m";
                break;
            }
            }

            std::cout << progress.c_str() << " " << result.c_str() << " " << test.GetTargetName().c_str() << " (" << (test.GetDuration().count() / 1000.f) << "s)\n";
        }

        void TestSequence::ClearState()
        {
            m_numTests = 0;
            m_numTestsComplete = 0;
        }
    }
}
