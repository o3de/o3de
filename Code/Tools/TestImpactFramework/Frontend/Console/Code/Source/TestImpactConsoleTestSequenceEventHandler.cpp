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

            void FailureReport(const Client::TestRunReport& testRunReport)
            {
                std::cout << "Sequence completed in " << (testRunReport.GetDuration().count() / 1000.f) << "s with";

                if (!testRunReport.GetExecutionFailureTests().empty() ||
                    !testRunReport.GetFailingTests().empty() ||
                    !testRunReport.GetTimedOutTests().empty() ||
                    !testRunReport.GetUnexecutedTests().empty())
                {
                    std::cout << ":\n";
                    std::cout << SetColor(Foreground::White, Background::Red).c_str()
                        << testRunReport.GetFailingTests().size()
                        << ResetColor().c_str() << " test failures\n";

                    std::cout << SetColor(Foreground::White, Background::Red).c_str()
                        << testRunReport.GetExecutionFailureTests().size()
                        << ResetColor().c_str() << " execution failures\n";

                    std::cout << SetColor(Foreground::White, Background::Red).c_str()
                        << testRunReport.GetTimedOutTests().size()
                        << ResetColor().c_str() << " test timeouts\n";

                    std::cout << SetColor(Foreground::White, Background::Red).c_str()
                        << testRunReport.GetUnexecutedTests().size()
                        << ResetColor().c_str() << " unexecuted tests\n";

                    if (!testRunReport.GetFailingTests().empty())
                    {
                        std::cout << "\nTest failures:\n";
                        for (const auto& testRunFailure : testRunReport.GetFailingTests())
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

                    if (!testRunReport.GetExecutionFailureTests().empty())
                    {
                        std::cout << "\nExecution failures:\n";
                        for (const auto& executionFailure : testRunReport.GetExecutionFailureTests())
                        {
                            std::cout << "  " << executionFailure.GetTargetName().c_str() << "\n";
                            std::cout << executionFailure.GetCommandString().c_str() << "\n";
                        }
                    }

                    if (!testRunReport.GetTimedOutTests().empty())
                    {
                        std::cout << "\nTimed out tests:\n";
                        for (const auto& testTimeout : testRunReport.GetTimedOutTests())
                        {
                            std::cout << "  " << testTimeout.GetTargetName().c_str() << "\n";
                        }
                    }

                    if (!testRunReport.GetUnexecutedTests().empty())
                    {
                        std::cout << "\nUnexecuted tests:\n";
                        for (const auto& unexecutedTest : testRunReport.GetUnexecutedTests())
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

        void TestSequenceStartCallback(SuiteType suiteType, const Client::TestRunSelection& selectedTests)
        {
        }

        void TestSequenceCompleteCallback(SuiteType suiteType, const Client::TestRunSelection& selectedTests)
        {
            ClearState();
            m_numTests = selectedTests.GetNumIncludedTestRuns();

            Output::TestSuiteFilter(m_suiteFilter);
            std::cout << selectedTests.GetNumIncludedTestRuns() << " tests selected, " << selectedTests.GetNumExcludedTestRuns() << " excluded.\n";
        }

        void ImpactAnalysisTestSequenceStartCallback(
            SuiteType suiteType,
            const Client::TestRunSelection& selectedTests,
            const AZStd::vector<AZStd::string>& discardedTests,
            const AZStd::vector<AZStd::string>& draftedTests)
        {
            ClearState();
            m_numTests = selectedTests.GetNumIncludedTestRuns() + draftedTests.size();

            Output::TestSuiteFilter(m_suiteFilter);
            Output::ImpactAnalysisTestSelection(
                selectedTests.GetTotalNumTests(), discardedTests.size(), selectedTests.GetNumExcludedTestRuns(), draftedTests.size());
        }

        void SafeImpactAnalysisTestSequenceStartCallback(
            SuiteType suiteType,
            const Client::TestRunSelection& selectedTests,
            const Client::TestRunSelection& discardedTests,
            const AZStd::vector<AZStd::string>& draftedTests)
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

        void TestSequenceCompleteCallback(const Client::SequenceReport& sequenceReport)
        {
            
            Output::FailureReport(sequenceReport.GetSelectedTestRunReport());
            std::cout << "Updating and serializing the test impact analysis data, this may take a moment...\n";
        }

        void ImpactAnalysisTestSequenceCompleteCallback(const Client::ImpactAnalysisSequenceReport& sequenceReport)
        {
            std::cout << "Selected test run:\n";
            Output::FailureReport(sequenceReport.GetSelectedTestRunReport());

            std::cout << "Drafted test run:\n";
            Output::FailureReport(sequenceReport.GetDraftedTestRunReport());

            std::cout << "Updating and serializing the test impact analysis data, this may take a moment...\n";
        }

        void SafeImpactAnalysisTestSequenceCompleteCallback(const Client::SafeImpactAnalysisSequenceReport& sequenceReport)
        {
            std::cout << "Selected test run:\n";
            Output::FailureReport(sequenceReport.GetSelectedTestRunReport());

            std::cout << "Discarded test run:\n";
            Output::FailureReport(sequenceReport.GetDiscardedTestRunReport());

            std::cout << "Drafted test run:\n";
            Output::FailureReport(sequenceReport.GetDraftedTestRunReport());

            std::cout << "Updating and serializing the test impact analysis data, this may take a moment...\n";
        }

        void TestRunCompleteCallback(const Client::TestRun& testRun, size_t numTestRunsCompleted, size_t totalNumTestRuns)
        {
            const auto progress =
                AZStd::string::format("(%03u/%03u)", numTestRunsCompleted, totalNumTestRuns, testRun.GetTargetName().c_str());

            AZStd::string result;
            switch (testRun.GetResult())
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

            std::cout << progress.c_str() << " " << result.c_str() << " " << testRun.GetTargetName().c_str() << " ("
                      << (testRun.GetDuration().count() / 1000.f) << "s)\n";
        }

        void TestSequenceEventHandler::ClearState()
        {
            m_numTests = 0;
            m_numTestsComplete = 0;
        }
    } // namespace Console
} // namespace TestImpact
