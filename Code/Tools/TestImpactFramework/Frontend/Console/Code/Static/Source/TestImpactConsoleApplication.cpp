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

#include <TestImpactFramework/TestImpactException.h>
#include <TestImpactFramework/TestImpactChangeListException.h>
#include <TestImpactFramework/TestImpactConfigurationException.h>
#include <TestImpactFramework/TestImpactRuntimeException.h>
#include <TestImpactFramework/TestImpactConsoleApplication.h>
#include <TestImpactFramework/TestImpactChangeListSerializer.h>
#include <TestImpactFramework/TestImpactChangeList.h>
#include <TestImpactFramework/TestImpactRuntime.h>
#include <TestImpactFramework/TestImpactUtils.h>
#include <TestImpactFramework/TestImpactClientTestSelection.h>
#include <TestImpactFramework/TestImpactRuntime.h>

#include <TestImpactCommandLineOptions.h>
#include <TestImpactConfigurationFactory.h>
#include <TestImpactCommandLineOptionsException.h>

#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

#include <iostream>

namespace TestImpact
{
    namespace Console
    {
        AZStd::string GetChangeListString(const ChangeList& changeList)
        {
            AZStd::string output;

            const auto& outputFiles = [&output](const AZStd::vector<RepoPath>& files)
            {
                for (const auto& file : files)
                {
                    output += AZStd::string::format("\t%s\n", file.c_str());
                }
            };

            output += AZStd::string::format("Created files (%u):\n", changeList.m_createdFiles.size());
            outputFiles(changeList.m_createdFiles);

            output += AZStd::string::format("Updated files (%u):\n", changeList.m_updatedFiles.size());
            outputFiles(changeList.m_updatedFiles);

            output += AZStd::string::format("Deleted files (%u):\n", changeList.m_deletedFiles.size());
            outputFiles(changeList.m_deletedFiles);

            return output;
        }

        ReturnCode Main(int argc, char** argv)
        {
            try
            {
                CommandLineOptions options(argc, argv);
                AZStd::optional<ChangeList> changeList;

                if (options.HasChangeListFile())
                {
                    changeList = DeserializeChangeList(ReadFileContents<CommandLineOptionsException>(*options.GetChangeListFile()));
                    if (options.HasOutputChangeList())
                    {
                        std::cout << "Change List:\n";
                        std::cout << GetChangeListString(*changeList).c_str();

                        if (!options.HasTestSequence())
                        {
                            return ReturnCode::Success;
                        }
                    }
                }

                AZ_TestImpact_Eval(options.HasTestSequence(), CommandLineOptionsException, "No action specified");

                Runtime runtime(
                    ConfigurationFactory(ReadFileContents<CommandLineOptionsException>(options.GetConfigurationFile())),
                    options.GetExecutionFailurePolicy(),
                    options.GetExecutionFailureDraftingPolicy(),
                    options.GetTestFailurePolicy(),
                    options.GetIntegrityFailurePolicy(),
                    options.GetTestShardingPolicy(),
                    options.GetTargetOutputCapture(),
                    options.GetMaxConcurrency());

                const auto handleTestSequenceResult = [](TestSequenceResult result)
                {
                    switch (result)
                    {
                    case TestSequenceResult::Success:
                        return ReturnCode::Success;
                    case TestSequenceResult::Failure:
                        return ReturnCode::TestFailure;
                    //case TestSequenceResult::Timeout:
                    //    return ReturnCode::TestFailure;
                    default:
                        std::cout << "Unexpected TestSequenceResult value: " << static_cast<size_t>(result) << std::endl;
                        return ReturnCode::UnknownError;
                    }
                };

                const auto impactAnalysisTestSequence = [&options, &runtime, &changeList, &handleTestSequenceResult]()
                {
                    AZ_TestImpact_Eval(changeList.has_value(), CommandLineOptionsException, "Expected a change list for impact analysis but none was provided");
                    TestSequenceResult result = TestSequenceResult::Failure;
                    if (options.HasSafeMode())
                    {
                        result = runtime.ImpactAnalysisTestSequence(
                            changeList.value(),
                            options.GetTestPrioritizationPolicy(),
                            options.GetTestTargetTimeout(),
                            options.GetGlobalTimeout(),
                            AZStd::nullopt,
                            AZStd::nullopt,
                            AZStd::nullopt);
                    }
                    else
                    {
                        result = runtime.SafeImpactAnalysisTestSequence(
                            changeList.value(),
                            options.GetSuitesFilter(),
                            options.GetTestPrioritizationPolicy(),
                            options.GetTestTargetTimeout(),
                            options.GetGlobalTimeout(),
                            AZStd::nullopt,
                            AZStd::nullopt,
                            AZStd::nullopt);
                    }

                    return handleTestSequenceResult(result);
                };

                size_t numTests = 0;
                size_t testsComplete = 0;

                const auto sequenceStart = [&options, &numTests](Client::TestRunSelection&& testSelection)
                {
                    std::cout << "Test suite filter:\n";
                    if (const auto& suiteFilter = options.GetSuitesFilter(); suiteFilter.empty())
                    {
                        std::cout << "  *\n";
                    }
                    else
                    {
                        for (const auto& suite : suiteFilter)
                        {
                            std::cout << "  " << suite.c_str() << "\n";
                        }
                    }

                    numTests = testSelection.GetNumIncludedTestRuns();
                    std::cout << numTests << " tests selected, " << testSelection.GetNumNumExcludedTestRuns() << " excluded\n";
                };

                const auto testComplete = [&numTests, &testsComplete](Client::TestRun&& test)
                {
                    testsComplete++;
                    const auto progress = AZStd::string::format("(%02u/%02u)", testsComplete, numTests, test.GetTargetName().c_str());
                    const auto result = (test.GetResult() == Client::TestRunResult::AllTestsPass) ? "\033[37;42mPASS\033[0m" : "\033[37;41mFAIL\033[0m";
                    std::wcout << progress.c_str() << " " << result << " "  << test.GetTargetName().c_str() << " (" << (test.GetDuration().count() / 1000.f) << "s)\n";
                };

                const auto sequenceComplete = []([[maybe_unused]] Client::RegularSequenceFailure&& failureReport, AZStd::chrono::milliseconds duration)
                {
                    std::cout << "DURATION: " << (duration.count() / 1000.f) << "s\n";
                };

                switch (const auto type = *options.GetTestSequenceType())
                {
                case TestSequenceType::Regular:
                {
                    const auto result = runtime.RegularTestSequence(
                        options.GetSuitesFilter(),
                        options.GetTestTargetTimeout(),
                        options.GetGlobalTimeout(),
                        sequenceStart,
                        sequenceComplete,
                        testComplete);

                    return handleTestSequenceResult(result);
                }
                case TestSequenceType::Seed:
                {
                    const auto result = runtime.SeededTestSequence(
                        options.GetGlobalTimeout(),
                        sequenceStart,
                        sequenceComplete,
                        testComplete);

                    return handleTestSequenceResult(result);
                }
                case TestSequenceType::ImpactAnalysis:
                {
                    return impactAnalysisTestSequence();
                }
                case TestSequenceType::ImpactAnalysisOrSeed:
                {
                    if (runtime.HasImpactAnalysisData())
                    {
                        return impactAnalysisTestSequence();
                    }
                    else
                    {
                        const auto result = runtime.SeededTestSequence(
                            options.GetGlobalTimeout(),
                            sequenceStart,
                            AZStd::nullopt,
                            testComplete);

                        return handleTestSequenceResult(result);
                    }
                }
                default:
                    std::cout << "Unexpected TestSequenceType value: " << static_cast<size_t>(type) << std::endl;
                    return ReturnCode::UnknownError;
                }
            }
            catch (const CommandLineOptionsException& e)
            {
                std::cout << e.what() << std::endl;
                std::cout << CommandLineOptions::GetCommandLineUsageString().c_str() << std::endl;
                return ReturnCode::InvalidArgs;
            }
            catch (const ChangeListException& e)
            {
                std::cout << e.what() << std::endl;
                return ReturnCode::InvalidUnifiedDiff;
            }
            catch (const ConfigurationException& e)
            {
                std::cout << e.what() << std::endl;
                return ReturnCode::InvalidConfiguration;
            }
            catch (const RuntimeException& e)
            {
                std::cout << e.what() << std::endl;
                return ReturnCode::RuntimeError;
            }
            catch (const Exception& e)
            {
                std::cout << e.what() << std::endl;
                return ReturnCode::UnhandledError;
            }
            catch (const std::exception& e)
            {
                std::cout << e.what() << std::endl;
                return ReturnCode::UnknownError;
            }
            catch (...)
            {
                std::cout << "An unknown error occurred" << std::endl;
                return ReturnCode::UnknownError;
            }
        }
    }
}
