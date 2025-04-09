/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactFramework/TestImpactUtils.h>

#include <TestImpactTestSequenceNotificationHandler.h>
#include <TestImpactConsoleUtils.h>

#include <AzCore/IO/AnsiTerminalUtils.h>
#include <AzCore/IO/SystemFile.h>

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
            void TestSuiteSet(const SuiteSet& suiteSet, const SuiteLabelExcludeSet& suiteLabelExcludeSet)
            {
                std::cout << "Test suite set: " << SuiteSetAsString(suiteSet).c_str() << "\n";
                std::cout << "Test suite label exclude set: " << SuiteLabelExcludeSetAsString(suiteLabelExcludeSet).c_str() << "\n";
            }

            void ImpactAnalysisTestSelection(size_t numSelectedTests, size_t numDiscardedTests, size_t numExcludedTests, size_t numDraftedTests)
            {
                if (const size_t totalTests = numSelectedTests + numDiscardedTests;
                    totalTests > 0)
                {
                    const float saving = (1.0f - float(numSelectedTests) / totalTests) * 100.0f;

                    std::cout << numSelectedTests << " tests selected, " << numDiscardedTests << " tests discarded (" << saving << "% test saving)\n";
                    std::cout << "Of which " << numExcludedTests << " tests have been excluded and " << numDraftedTests << " tests have been drafted.\n";
                }
                else
                {
                    std::cout << "There are 0 total tests\n";
                }
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
        } // namespace Output

        TestSequenceNotificationHandlerBase::TestSequenceNotificationHandlerBase(const ConsoleOutputMode consoleOutputMode)
            : m_consoleOutputMode(consoleOutputMode)
        {
            TestSequenceNotificationsBaseBus::Handler::BusConnect();
            // Enable Virtual Terminal Processing, so that ANSI color sequences can be used
            AZ::IO::Posix::EnableVirtualTerminalProcessing(AZ::IO::Posix::Fileno(stdout));
        }

        TestSequenceNotificationHandlerBase::~TestSequenceNotificationHandlerBase()
        {
            TestSequenceNotificationsBaseBus::Handler::BusDisconnect();
        }

        void TestSequenceNotificationHandlerBase::OnTestRunComplete(
            const Client::TestRunBase& testRun,
            const size_t numTestRunsCompleted,
            const size_t totalNumTestRuns)
        {
            if (m_consoleOutputMode == ConsoleOutputMode::Buffered && testRun.GetResult() != Client::TestRunResult::AllTestsPass)
            {
                if (!testRun.GetStdOutput().empty())
                {
                    std::cout << testRun.GetStdOutput().c_str();
                }

                if (!testRun.GetStdError().empty())
                {
                    std::cout << testRun.GetStdError().c_str();
                }
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
                    AZ_Error(
                        "TestRunCompleteCallback",
                        false,
                        "Unexpected test result to handle: %u",
                        aznumeric_cast<AZ::u32>(testRun.GetResult()));
                }
            }

            const auto progress = AZStd::string::format("(%03zu/%03zu)", numTestRunsCompleted, totalNumTestRuns);
            std::cout << progress.c_str() << " " << result.c_str() << " " << testRun.GetTargetName().c_str() << " ("
                      << (testRun.GetDuration().count() / 1000.f) << "s)\n";

            PrintDivider();
        }

        void TestSequenceNotificationHandlerBase::OnRealtimeStdContent(
            const AZStd::string& stdOutDelta,
            const AZStd::string& stdErrDelta)
        {
            if (m_consoleOutputMode == ConsoleOutputMode::Realtime)
            {
                if (!stdOutDelta.empty())
                {
                    std::cout << stdOutDelta.c_str();
                }

                if (!stdErrDelta.empty())
                {
                    std::cout << stdErrDelta.c_str();
                }
            }
        }

        NonImpactAnalysisTestSequenceNotificationHandlerBase::NonImpactAnalysisTestSequenceNotificationHandlerBase(
            const ConsoleOutputMode consoleOutputMode)
            : TestSequenceNotificationHandlerBase(consoleOutputMode)
        {
            NonImpactAnalysisTestSequenceNotificationsBaseBus::Handler::BusConnect();
        }

        NonImpactAnalysisTestSequenceNotificationHandlerBase::~NonImpactAnalysisTestSequenceNotificationHandlerBase()
        {
            NonImpactAnalysisTestSequenceNotificationsBaseBus::Handler::BusDisconnect();
        }

        void NonImpactAnalysisTestSequenceNotificationHandlerBase::OnTestSequenceStart(
            const SuiteSet& suiteSet, const SuiteLabelExcludeSet& suiteLabelExcludeSet, const Client::TestRunSelection& selectedTests)
        {
            Output::TestSuiteSet(suiteSet, suiteLabelExcludeSet);
            std::cout << selectedTests.GetNumIncludedTestRuns() << " tests selected, " << selectedTests.GetNumExcludedTestRuns()
                      << " excluded.\n";

            PrintDivider();
        }

        RegularTestSequenceNotificationHandler::RegularTestSequenceNotificationHandler(const ConsoleOutputMode consoleOutputMode)
            : NonImpactAnalysisTestSequenceNotificationHandlerBase(consoleOutputMode)
        {
            RegularTestSequenceNotificationBus::Handler::BusConnect();
        }

        RegularTestSequenceNotificationHandler::~RegularTestSequenceNotificationHandler()
        {
            RegularTestSequenceNotificationBus::Handler::BusDisconnect();
        }

        void RegularTestSequenceNotificationHandler::OnTestSequenceComplete(const Client::RegularSequenceReport& sequenceReport)
        {
            Output::FailureReport(sequenceReport.GetSelectedTestRunReport());
            std::cout << "Updating and serializing the test impact analysis data, this may take a moment...\n";
        }

        SeedTestSequenceNotificationHandler::SeedTestSequenceNotificationHandler(const ConsoleOutputMode consoleOutputMode)
            : NonImpactAnalysisTestSequenceNotificationHandlerBase(consoleOutputMode)
        {
            SeedTestSequenceNotificationBus::Handler::BusConnect();
        }

        SeedTestSequenceNotificationHandler::~SeedTestSequenceNotificationHandler()
        {
            SeedTestSequenceNotificationBus::Handler::BusDisconnect();
        }

        void SeedTestSequenceNotificationHandler::OnTestSequenceComplete(const Client::SeedSequenceReport& sequenceReport)
        {
            Output::FailureReport(sequenceReport.GetSelectedTestRunReport());
        }

        ImpactAnalysisTestSequenceNotificationHandler::ImpactAnalysisTestSequenceNotificationHandler(
            const ConsoleOutputMode consoleOutputMode)
            : TestSequenceNotificationHandlerBase(consoleOutputMode)
        {
            ImpactAnalysisTestSequenceNotificationBus::Handler::BusConnect();
        }

        ImpactAnalysisTestSequenceNotificationHandler::~ImpactAnalysisTestSequenceNotificationHandler()
        {
            ImpactAnalysisTestSequenceNotificationBus::Handler::BusDisconnect();
        }

        void ImpactAnalysisTestSequenceNotificationHandler::OnTestSequenceStart(
            const SuiteSet& suiteSet,
            const SuiteLabelExcludeSet& suiteLabelExcludeSet,
            const Client::TestRunSelection& selectedTests,
            const AZStd::vector<AZStd::string>& discardedTests,
            const AZStd::vector<AZStd::string>& draftedTests)
        {
            Output::TestSuiteSet(suiteSet, suiteLabelExcludeSet);
            Output::ImpactAnalysisTestSelection(
                selectedTests.GetTotalNumTests(), discardedTests.size(), selectedTests.GetNumExcludedTestRuns(), draftedTests.size());

            PrintDivider();
        }

        void ImpactAnalysisTestSequenceNotificationHandler::OnTestSequenceComplete(
            const Client::ImpactAnalysisSequenceReport& sequenceReport)
        {
            std::cout << "Selected test run:\n";
            Output::FailureReport(sequenceReport.GetSelectedTestRunReport());

            std::cout << "Drafted test run:\n";
            Output::FailureReport(sequenceReport.GetDraftedTestRunReport());

            std::cout << "Updating and serializing the test impact analysis data, this may take a moment...\n";
        }

        SafeImpactAnalysisTestSequenceNotificationHandler::SafeImpactAnalysisTestSequenceNotificationHandler(ConsoleOutputMode consoleOutputMode)
            : TestSequenceNotificationHandlerBase(consoleOutputMode)
        {
            SafeImpactAnalysisTestSequenceNotificationBus::Handler::BusConnect();
        }

        SafeImpactAnalysisTestSequenceNotificationHandler::~SafeImpactAnalysisTestSequenceNotificationHandler()
        {
            SafeImpactAnalysisTestSequenceNotificationBus::Handler::BusDisconnect();
        }

        void SafeImpactAnalysisTestSequenceNotificationHandler::OnTestSequenceStart(
            const SuiteSet& suiteSet,
            const SuiteLabelExcludeSet& suiteLabelExcludeSet,
            const Client::TestRunSelection& selectedTests,
            const Client::TestRunSelection& discardedTests,
            const AZStd::vector<AZStd::string>& draftedTests)
        {
            Output::TestSuiteSet(suiteSet, suiteLabelExcludeSet);
            Output::ImpactAnalysisTestSelection(
                selectedTests.GetTotalNumTests(),
                discardedTests.GetTotalNumTests(),
                selectedTests.GetNumExcludedTestRuns() + discardedTests.GetNumExcludedTestRuns(),
                draftedTests.size());

            PrintDivider();
        }

        void SafeImpactAnalysisTestSequenceNotificationHandler::OnTestSequenceComplete(
            const Client::SafeImpactAnalysisSequenceReport& sequenceReport)
        {
            std::cout << "Selected test run:\n";
            Output::FailureReport(sequenceReport.GetSelectedTestRunReport());

            std::cout << "Discarded test run:\n";
            Output::FailureReport(sequenceReport.GetDiscardedTestRunReport());

            std::cout << "Drafted test run:\n";
            Output::FailureReport(sequenceReport.GetDraftedTestRunReport());

            std::cout << "Updating and serializing the test impact analysis data, this may take a moment...\n";
        }
    } // namespace Console
} // namespace TestImpact
