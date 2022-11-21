/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactFramework/TestImpactUtils.h>

#include <TestImpactConsoleTestSequenceEventHandler.h>
#include <TestImpactConsoleUtils.h>

#include <iostream>

namespace TestImpact
{
    namespace Console
    {
        static void PrintDivider()
        {
            std::cout << "-----------------------------------------------------------------------------\n";
        }

        namespace Output
        {
            void TestSuiteFilter(SuiteType filter)
            {
                std::cout << "Test suite filter: " << SuiteTypeAsString(filter).c_str() << "\n";
            }

            void ImpactAnalysisTestSelection(size_t numSelectedTests, size_t numDiscardedTests, size_t numExcludedTests, size_t numDraftedTests)
            {
                const float totalTests = static_cast<float>(numSelectedTests + numDiscardedTests);
                const float saving = (1.0f - (numSelectedTests / totalTests)) * 100.0f;

                std::cout << numSelectedTests << " tests selected, " << numDiscardedTests << " tests discarded (" << saving << "% test saving)\n";
                std::cout << "Of which " << numExcludedTests << " tests have been excluded and " << numDraftedTests << " tests have been drafted.\n";
            }

            void FailureReport(const Client::TestRunReport& testRunReport)
            {
                std::cout << "Sequence completed in " << (testRunReport.GetDuration().count() / 1000.f) << "s with";

                if (!testRunReport.GetExecutionFailureTestRuns().empty() ||
                    !testRunReport.GetFailingTestRuns().empty() ||
                    !testRunReport.GetTimedOutTestRuns().empty() ||
                    !testRunReport.GetUnexecutedTestRuns().empty())
                {
                    std::cout << ":\n";
                    std::cout << SetColor(Foreground::White, Background::Red).c_str()
                        << testRunReport.GetFailingTestRuns().size()
                        << ResetColor().c_str() << " test failures\n";

                    std::cout << SetColor(Foreground::White, Background::Red).c_str()
                        << testRunReport.GetExecutionFailureTestRuns().size()
                        << ResetColor().c_str() << " execution failures\n";

                    std::cout << SetColor(Foreground::White, Background::Red).c_str()
                        << testRunReport.GetTimedOutTestRuns().size()
                        << ResetColor().c_str() << " test timeouts\n";

                    std::cout << SetColor(Foreground::White, Background::Red).c_str()
                        << testRunReport.GetUnexecutedTestRuns().size()
                        << ResetColor().c_str() << " unexecuted tests\n";

                    if (!testRunReport.GetFailingTestRuns().empty())
                    {
                        std::cout << "\nTest failures:\n";
                        for (const auto& testRunFailure : testRunReport.GetFailingTestRuns())
                        {
                            for (const auto& test : testRunFailure.GetTests())
                            {
                                if (test.GetResult() == Client::TestResult::Failed)
                                {
                                    std::cout << "  " << test.GetName().c_str() << "\n";
                                }
                            }
                        }
                    }

                    if (!testRunReport.GetExecutionFailureTestRuns().empty())
                    {
                        std::cout << "\nExecution failures:\n";
                        for (const auto& executionFailure : testRunReport.GetExecutionFailureTestRuns())
                        {
                            std::cout << "  " << executionFailure.GetTargetName().c_str() << "\n";
                            std::cout << executionFailure.GetCommandString().c_str() << "\n";
                        }
                    }

                    if (!testRunReport.GetTimedOutTestRuns().empty())
                    {
                        std::cout << "\nTimed out tests:\n";
                        for (const auto& testTimeout : testRunReport.GetTimedOutTestRuns())
                        {
                            std::cout << "  " << testTimeout.GetTargetName().c_str() << "\n";
                        }
                    }

                    if (!testRunReport.GetUnexecutedTestRuns().empty())
                    {
                        std::cout << "\nUnexecuted tests:\n";
                        for (const auto& unexecutedTest : testRunReport.GetUnexecutedTestRuns())
                        {
                            std::cout << "  " << unexecutedTest.GetTargetName().c_str() << "\n";
                        }
                    }
                }
                else
                {
                    std::cout << " " << SetColor(Foreground::White, Background::Green).c_str() << "100% passes!\n" << ResetColor().c_str() << "\n";
                }
            }
        }

        void TestSequenceStartCallback(SuiteType suiteType, const Client::TestRunSelection& selectedTests)
        {
            Output::TestSuiteFilter(suiteType);
            std::cout << selectedTests.GetNumIncludedTestRuns() << " tests selected, " << selectedTests.GetNumExcludedTestRuns()
                      << " excluded.\n";

            PrintDivider();
        }

        void TestSequenceCompleteCallback(SuiteType suiteType, const Client::TestRunSelection& selectedTests)
        {
            Output::TestSuiteFilter(suiteType);
            std::cout << selectedTests.GetNumIncludedTestRuns() << " tests selected, " << selectedTests.GetNumExcludedTestRuns() << " excluded.\n";
        }

        void ImpactAnalysisTestSequenceStartCallback(
            SuiteType suiteType,
            const Client::TestRunSelection& selectedTests,
            const AZStd::vector<AZStd::string>& discardedTests,
            const AZStd::vector<AZStd::string>& draftedTests)
        {
            Output::TestSuiteFilter(suiteType);
            Output::ImpactAnalysisTestSelection(
                selectedTests.GetTotalNumTests(), discardedTests.size(), selectedTests.GetNumExcludedTestRuns(), draftedTests.size());

            PrintDivider();
        }

        void SafeImpactAnalysisTestSequenceStartCallback(
            SuiteType suiteType,
            const Client::TestRunSelection& selectedTests,
            const Client::TestRunSelection& discardedTests,
            const AZStd::vector<AZStd::string>& draftedTests)
        {
            Output::TestSuiteFilter(suiteType);
            Output::ImpactAnalysisTestSelection(
                selectedTests.GetTotalNumTests(),
                discardedTests.GetTotalNumTests(),
                selectedTests.GetNumExcludedTestRuns() + discardedTests.GetNumExcludedTestRuns(),
                draftedTests.size());

            PrintDivider();
        }

        void RegularTestSequenceCompleteCallback(const Client::RegularSequenceReport& sequenceReport)
        {
            Output::FailureReport(sequenceReport.GetSelectedTestRunReport());
            std::cout << "Updating and serializing the test impact analysis data, this may take a moment...\n";
        }

        void SeedTestSequenceCompleteCallback(const Client::SeedSequenceReport& sequenceReport)
        {
            Output::FailureReport(sequenceReport.GetSelectedTestRunReport());
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

        void TestRunCompleteCallback(const Client::TestRunBase& testRun, size_t numTestRunsCompleted, size_t totalNumTestRuns)
        {
            const auto progress =
                AZStd::string::format("(%03zu/%03zu)", numTestRunsCompleted, totalNumTestRuns);

            if (!testRun.GetStdOutput().empty())
            {
                std::cout << testRun.GetStdOutput().c_str();
            }

            if (!testRun.GetStdError().empty())
            {
                std::cout << testRun.GetStdError().c_str();
            }

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
            default:
            {
                AZ_Error("TestRunCompleteCallback", false, "Unexpected test result to handle: %u", aznumeric_cast<AZ::u32>(testRun.GetResult()));
            }
            }

            std::cout << progress.c_str() << " " << result.c_str() << " " << testRun.GetTargetName().c_str() << " ("
                      << (testRun.GetDuration().count() / 1000.f) << "s)\n";

            PrintDivider();
        }
    } // namespace Console
} // namespace TestImpact
