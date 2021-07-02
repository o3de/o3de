/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactFramework/TestImpactException.h>
#include <TestImpactFramework/TestImpactChangeListException.h>
#include <TestImpactFramework/TestImpactConfigurationException.h>
#include <TestImpactFramework/TestImpactRuntimeException.h>
#include <TestImpactFramework/TestImpactConsoleMain.h>
#include <TestImpactFramework/TestImpactChangeListSerializer.h>
#include <TestImpactFramework/TestImpactChangeList.h>
#include <TestImpactFramework/TestImpactRuntime.h>
#include <TestImpactFramework/TestImpactFileUtils.h>
#include <TestImpactFramework/TestImpactClientTestSelection.h>
#include <TestImpactFramework/TestImpactRuntime.h>

#include <TestImpactConsoleTestSequenceEventHandler.h>
#include <TestImpactCommandLineOptions.h>
#include <TestImpactRuntimeConfigurationFactory.h>
#include <TestImpactCommandLineOptionsException.h>

#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

#include <iostream>

namespace TestImpact
{
    namespace Console
    {
        //! Generates a string to be used for printing to the console for the specified change list.
        AZStd::string GenerateChangeListString(const ChangeList& changeList)
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

        //! Gets the appropriate console return code for the specified test sequence result.
        ReturnCode GetReturnCodeForTestSequenceResult(TestSequenceResult result)
        {
            switch (result)
            {
            case TestSequenceResult::Success:
                return ReturnCode::Success;
            case TestSequenceResult::Failure:
                return ReturnCode::TestFailure;
            case TestSequenceResult::Timeout:
                return ReturnCode::Timeout;
            default:
                std::cout << "Unexpected TestSequenceResult value: " << aznumeric_cast<size_t>(result) << std::endl;
                return ReturnCode::UnknownError;
            }
        }

        //! Wrapper around impact analysis sequences to handle the case where the safe mode option is active.
        ReturnCode WrappedImpactAnalysisTestSequence(
            TestSequenceEventHandler& sequenceEventHandler,
            const CommandLineOptions& options,
            Runtime& runtime,
            const AZStd::optional<ChangeList>& changeList)
        {
            // Even though it is possible for a regular run to be selected (see below) which does not actually require a change list,
            // consider any impact analysis sequence type without a change list to be an error
            AZ_TestImpact_Eval(
                changeList.has_value(),
                CommandLineOptionsException,
                "Expected a change list for impact analysis but none was provided");

            TestSequenceResult result = TestSequenceResult::Failure;
            if (options.HasSafeMode())
            {
                if (options.GetTestSequenceType() == TestSequenceType::ImpactAnalysis)
                {
                    auto [selectedResult, discardedResult] = runtime.SafeImpactAnalysisTestSequence(
                        changeList.value(),
                        options.GetTestPrioritizationPolicy(),
                        options.GetTestTargetTimeout(),
                        options.GetGlobalTimeout(),
                        AZStd::ref(sequenceEventHandler),
                        AZStd::ref(sequenceEventHandler),
                        AZStd::ref(sequenceEventHandler));

                    // Handling the possible timeout and failure permutations of the selected and discarded test results is splitting hairs
                    // so apply the following, admittedly arbitrary, rules to determine what the composite test sequence result should be
                    if (selectedResult == TestSequenceResult::Success && discardedResult == TestSequenceResult::Success)
                    {
                        // Trivial case: both sequences succeeded
                        result = TestSequenceResult::Success;
                    }
                    else if (selectedResult == TestSequenceResult::Failure || discardedResult == TestSequenceResult::Failure)
                    {
                        // One sequence failed whilst the other sequence either succeeded or timed out
                        result = TestSequenceResult::Failure;
                    }
                    else
                    {
                        // One or both sequences timed out or failed
                        result = TestSequenceResult::Timeout;
                    }
                }
                else if (options.GetTestSequenceType() == TestSequenceType::ImpactAnalysisNoWrite)
                {
                    // A no-write impact analysis sequence with safe mode enabled is functionally identical to a regular sequence type
                    // due to a) the selected tests being run without instrumentation and b) the discarded tests also being run without
                    // instrumentation
                    result = runtime.RegularTestSequence(
                        options.GetTestTargetTimeout(),
                        options.GetGlobalTimeout(),
                        AZStd::ref(sequenceEventHandler),
                        AZStd::ref(sequenceEventHandler),
                        AZStd::ref(sequenceEventHandler));
                }
                else
                {
                    throw(Exception("Unexpected sequence type"));
                }
            }
            else
            {
                Policy::DynamicDependencyMap dynamicDependencyMapPolicy;
                if (options.GetTestSequenceType() == TestSequenceType::ImpactAnalysis)
                {
                    dynamicDependencyMapPolicy = Policy::DynamicDependencyMap::Update;
                }
                else if (options.GetTestSequenceType() == TestSequenceType::ImpactAnalysisNoWrite)
                {
                    dynamicDependencyMapPolicy = Policy::DynamicDependencyMap::Discard;
                }
                else
                {
                    throw(Exception("Unexpected sequence type"));
                }

                result = runtime.ImpactAnalysisTestSequence(
                    changeList.value(),
                    options.GetTestPrioritizationPolicy(),
                    dynamicDependencyMapPolicy,
                    options.GetTestTargetTimeout(),
                    options.GetGlobalTimeout(),
                    AZStd::ref(sequenceEventHandler),
                    AZStd::ref(sequenceEventHandler),
                    AZStd::ref(sequenceEventHandler));
            }

            return GetReturnCodeForTestSequenceResult(result);
        };

        //! Entry point for the test impact analysis framework console front end application.
        ReturnCode Main(int argc, char** argv)
        {
            try
            {
                CommandLineOptions options(argc, argv);
                AZStd::optional<ChangeList> changeList;

                // If we have a change list, check to see whether or not the client has requested the printing of said change list
                if (options.HasChangeListFile())
                {
                    changeList = DeserializeChangeList(ReadFileContents<CommandLineOptionsException>(*options.GetChangeListFile()));
                    if (options.HasOutputChangeList())
                    {
                        std::cout << "Change List:\n";
                        std::cout << GenerateChangeListString(*changeList).c_str();

                        if (options.GetTestSequenceType() == TestSequenceType::None)
                        {
                            return ReturnCode::Success;
                        }
                    }
                }

                // As of now, there are no other non-test operations other than printing a change list so getting this far is considered an error
                AZ_TestImpact_Eval(options.GetTestSequenceType() != TestSequenceType::None, CommandLineOptionsException, "No action specified");

                std::cout << "Constructing in-memory model of source tree and test coverage for test suite ";
                std::cout << GetSuiteTypeName(options.GetSuiteFilter()).c_str() << ", this may take a moment...\n";
                Runtime runtime(
                    RuntimeConfigurationFactory(ReadFileContents<CommandLineOptionsException>(options.GetConfigurationFile())),
                    options.GetSuiteFilter(),
                    options.GetExecutionFailurePolicy(),
                    options.GetFailedTestCoveragePolicy(),
                    options.GetTestFailurePolicy(),
                    options.GetIntegrityFailurePolicy(),
                    options.GetTestShardingPolicy(),
                    options.GetTargetOutputCapture(),
                    options.GetMaxConcurrency());

                if (runtime.HasImpactAnalysisData())
                {
                    std::cout << "Test impact analysis data for this repository was found.\n";
                }
                else
                {
                    std::cout << "Test impact analysis data for this repository was not found, seed or regular sequence fallbacks will be used.\n";
                }

                TestSequenceEventHandler sequenceEventHandler(options.GetSuiteFilter());

                switch (const auto type = options.GetTestSequenceType())
                {
                case TestSequenceType::Regular:
                {
                    const auto result = runtime.RegularTestSequence(
                        options.GetTestTargetTimeout(),
                        options.GetGlobalTimeout(),
                        AZStd::ref(sequenceEventHandler),
                        AZStd::ref(sequenceEventHandler),
                        AZStd::ref(sequenceEventHandler));

                    return GetReturnCodeForTestSequenceResult(result);
                }
                case TestSequenceType::Seed:
                {
                    const auto result = runtime.SeededTestSequence(
                        options.GetTestTargetTimeout(),
                        options.GetGlobalTimeout(),
                        AZStd::ref(sequenceEventHandler),
                        AZStd::ref(sequenceEventHandler),
                        AZStd::ref(sequenceEventHandler));

                    return GetReturnCodeForTestSequenceResult(result);
                }
                case TestSequenceType::ImpactAnalysisNoWrite:
                case TestSequenceType::ImpactAnalysis:
                {
                    return WrappedImpactAnalysisTestSequence(sequenceEventHandler, options, runtime, changeList);
                }
                case TestSequenceType::ImpactAnalysisOrSeed:
                {
                    if (runtime.HasImpactAnalysisData())
                    {
                        return WrappedImpactAnalysisTestSequence(sequenceEventHandler, options, runtime, changeList);
                    }
                    else
                    {
                        const auto result = runtime.SeededTestSequence(
                            options.GetTestTargetTimeout(),
                            options.GetGlobalTimeout(),
                            AZStd::ref(sequenceEventHandler),
                            AZStd::ref(sequenceEventHandler),
                            AZStd::ref(sequenceEventHandler));

                        return GetReturnCodeForTestSequenceResult(result);
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
    } // namespace Console 
} // namespace TestImpact
