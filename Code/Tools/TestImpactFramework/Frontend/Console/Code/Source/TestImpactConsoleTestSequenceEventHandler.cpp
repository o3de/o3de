/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactConsoleTestSequenceEventHandler.h>

#include <TestImpactConsoleUtils.h>

#include <iostream>

namespace TestImpact
{
    namespace Console
    {
        namespace Output
        {
            void TestSuiteFilter(SuiteType filter)
            {
                std::cout << "Test suite filter: " << GetSuiteTypeName(filter).c_str() << "\n";
            }

            void ImpactAnalysisTestSelection(size_t numSelectedTests, size_t numDiscardedTests, size_t numExcludedTests, size_t numDraftedTests)
            {
                const float totalTests = numSelectedTests + numDiscardedTests;
                const float saving = (1.0 - (numSelectedTests / totalTests)) * 100.0f;

                std::cout << numSelectedTests << " tests selected, " << numDiscardedTests << " tests discarded (" << saving << "% test saving)\n";
                std::cout << "Of which " << numExcludedTests << " tests have been excluded and " << numDraftedTests << " tests have been drafted.\n";
            }

            void FailureReport(const Client::SequenceFailure& failureReport, AZStd::chrono::milliseconds duration)
            {
                std::cout << "Sequence completed in " << (duration.count() / 1000.f) << "s with";

                if (!failureReport.GetExecutionFailures().empty() ||
                    !failureReport.GetTestRunFailures().empty() ||
                    !failureReport.GetTimedOutTests().empty() ||
                    !failureReport.GetUnexecutedTests().empty())
                {
                    std::cout << ":\n";
                    std::cout << SetColor(Foreground::White, Background::Red).c_str()
                        << failureReport.GetTestRunFailures().size()
                        << ResetColor().c_str() << " test failures\n";

                    std::cout << SetColor(Foreground::White, Background::Red).c_str()
                        << failureReport.GetExecutionFailures().size()
                        << ResetColor().c_str() << " execution failures\n";

                    std::cout << SetColor(Foreground::White, Background::Red).c_str()
                        << failureReport.GetTimedOutTests().size()
                        << ResetColor().c_str() << " test timeouts\n";

                    std::cout << SetColor(Foreground::White, Background::Red).c_str()
                        << failureReport.GetUnexecutedTests().size()
                        << ResetColor().c_str() << " unexecuted tests\n";

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
                    std::cout << SetColor(Foreground::White, Background::Green).c_str() << " \100% passes!\n" << ResetColor().c_str();
                }
            }
        }

        TestSequenceEventHandler::TestSequenceEventHandler(SuiteType suiteFilter)
            : m_suiteFilter(suiteFilter)
        {
        }

        // TestSequenceStartCallback
        void TestSequenceEventHandler::operator()(Client::TestRunSelection&& selectedTests)
        {
            ClearState();
            m_numTests = selectedTests.GetNumIncludedTestRuns();

            Output::TestSuiteFilter(m_suiteFilter);
            std::cout << selectedTests.GetNumIncludedTestRuns() << " tests selected, " << selectedTests.GetNumExcludedTestRuns() << " excluded.\n";
        }

        // ImpactAnalysisTestSequenceStartCallback
        void TestSequenceEventHandler::operator()(
            Client::TestRunSelection&& selectedTests,
            AZStd::vector<AZStd::string>&& discardedTests,
            AZStd::vector<AZStd::string>&& draftedTests)
        {
            ClearState();
            m_numTests = selectedTests.GetNumIncludedTestRuns() + draftedTests.size();

            Output::TestSuiteFilter(m_suiteFilter);
            Output::ImpactAnalysisTestSelection(
                selectedTests.GetTotalNumTests(), discardedTests.size(), selectedTests.GetNumExcludedTestRuns(), draftedTests.size());
        }

        // SafeImpactAnalysisTestSequenceStartCallback
        void TestSequenceEventHandler::operator()(
            Client::TestRunSelection&& selectedTests,
            Client::TestRunSelection&& discardedTests,
            AZStd::vector<AZStd::string>&& draftedTests)
        {
            ClearState();
            m_numTests = selectedTests.GetNumIncludedTestRuns() + draftedTests.size();

            Output::TestSuiteFilter(m_suiteFilter);
            Output::ImpactAnalysisTestSelection(
                selectedTests.GetTotalNumTests(),
                discardedTests.GetTotalNumTests(),
                selectedTests.GetNumExcludedTestRuns() + discardedTests.GetNumExcludedTestRuns(),
                draftedTests.size());
        }

        // TestSequenceCompleteCallback
        void TestSequenceEventHandler::operator()(
            Client::SequenceFailure&& failureReport,
            AZStd::chrono::milliseconds duration)
        {
            
            Output::FailureReport(failureReport, duration);
            std::cout << "Updating and serializing the test impact analysis data, this may take a moment...\n";
        }

        // SafeTestSequenceCompleteCallback
        void TestSequenceEventHandler::operator()(
            Client::SequenceFailure&& selectedFailureReport,
            Client::SequenceFailure&& discardedFailureReport,
            AZStd::chrono::milliseconds selectedDuration,
            AZStd::chrono::milliseconds discaredDuration)
        {
            std::cout << "Selected test run:\n";
            Output::FailureReport(selectedFailureReport, selectedDuration);

            std::cout << "Discarded test run:\n";
            Output::FailureReport(discardedFailureReport, discaredDuration);

            std::cout << "Updating and serializing the test impact analysis data, this may take a moment...\n";
        }

        // TestRunCompleteCallback
        void TestSequenceEventHandler::operator()([[maybe_unused]] Client::TestRun&& test)
        {
            m_numTestsComplete++;
            const auto progress = AZStd::string::format("(%03u/%03u)", m_numTestsComplete, m_numTests, test.GetTargetName().c_str());

            AZStd::string result;
            switch (test.GetResult())
            {
            case Client::TestRunResult::AllTestsPass:
            {
                result = SetColorForString(Foreground::White, Background::Green, "PASS");
                break;
            }
            case Client::TestRunResult::FailedToExecute:
            {
                result = SetColorForString(Foreground::White, Background::Red, "EXEC");
                break;
            }
            case Client::TestRunResult::NotRun:
            {
                result = SetColorForString(Foreground::White, Background::Yellow, "SKIP");
                break;
            }
            case Client::TestRunResult::TestFailures:
            {
                result = SetColorForString(Foreground::White, Background::Red, "FAIL");
                break;
            }
            case Client::TestRunResult::Timeout:
            {
                result = SetColorForString(Foreground::White, Background::Magenta, "TIME");
                break;
            }
            }

            std::cout << progress.c_str() << " " << result.c_str() << " " << test.GetTargetName().c_str() << " (" << (test.GetDuration().count() / 1000.f) << "s)\n";
        }

        void TestSequenceEventHandler::ClearState()
        {
            m_numTests = 0;
            m_numTestsComplete = 0;
        }
    } // namespace Console
} // namespace TestImpact
