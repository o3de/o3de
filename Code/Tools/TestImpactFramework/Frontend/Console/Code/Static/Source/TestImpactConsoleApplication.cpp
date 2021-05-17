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
#include <TestImpactFramework/TestImpactTestSelection.h>

#include <TestImpactChangeListFactory.h>
#include <TestImpactCommandLineOptions.h>
#include <TestImpactConfigurationFactory.h>
#include <TestImpactCommandLineOptionsException.h>

#include <AzCore/IO/Path/Path.h>
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

            const auto& outputFiles = [&output](const AZStd::vector<AZStd::string>& files)
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

                if (options.HasUnifiedDiffFile())
                {
                    const auto diff = ReadFileContents<CommandLineOptionsException>(*options.GetUnifiedDiffFile());
                    changeList = UnifiedDiff::ChangeListFactory(diff);
                    if (options.HasOutputChangeList())
                    {
                        if (options.GetOutputChangeList()->m_stdOut)
                        {
                            std::cout << "Change List:\n";
                            std::cout << GetChangeListString(*changeList).c_str();
                        }

                        if (options.GetOutputChangeList()->m_file.has_value())
                        {
                            TestImpact::WriteFileContents<CommandLineOptionsException>(TestImpact::SerializeChangeList(*changeList), options.GetOutputChangeList()->m_file.value());
                        }

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

                const auto testComplete = [](Test&& test)
                {
                    std::cout << "Test " << test.GetTargetName().c_str() << " completed\n";
                };

                switch (const auto type = *options.GetTestSequenceType())
                {
                case TestSequenceType::Regular:
                {
                    const auto sequenceStart = [&options](TestSelection&& testSelection)
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

                        std::cout << testSelection.GetIncludededTests().size() << " tests selected, " << testSelection.GetExcludedTests().size() << " excluded\n";
                    };

                    const auto result = runtime.RegularTestSequence(
                        options.GetSuitesFilter(),
                        options.GetTestTargetTimeout(),
                        options.GetGlobalTimeout(),
                        sequenceStart,
                        AZStd::nullopt,
                        testComplete);

                    return handleTestSequenceResult(result);
                }
                case TestSequenceType::Seed:
                {
                    const auto result = runtime.SeededTestSequence(
                        options.GetGlobalTimeout(),
                        AZStd::nullopt,
                        AZStd::nullopt,
                        AZStd::nullopt);

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
                            AZStd::nullopt,
                            AZStd::nullopt,
                            AZStd::nullopt);

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
