/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactFramework/TestImpactException.h>
#include <TestImpactFramework/TestImpactChangeListException.h>
#include <TestImpactFramework/TestImpactConfigurationException.h>
#include <TestImpactFramework/TestImpactSequenceReportException.h>
#include <TestImpactFramework/TestImpactRuntimeException.h>
#include <TestImpactFramework/TestImpactConsoleMain.h>
#include <TestImpactFramework/TestImpactChangeListSerializer.h>
#include <TestImpactFramework/TestImpactChangeList.h>
#include <TestImpactFramework/TestImpactRuntime.h>
#include <TestImpactFramework/TestImpactUtils.h>
#include <TestImpactFramework/TestImpactClientTestSelection.h>
#include <TestImpactFramework/TestImpactRuntime.h>
#include <TestImpactFramework/TestImpactClientSequenceReportSerializer.h>

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

        //! Wrapper around sequence reports to optionally serialize them and transform the result into a return code.
        template<typename SequenceReportType>
        ReturnCode ConsumeSequenceReportAndGetReturnCode(const SequenceReportType& sequenceReport, const CommandLineOptions& options)
        {
            if (options.HasSequenceReportFilePath())
            {
                std::cout << "Exporting sequence report '" << options.GetSequenceReportFilePath().value().c_str() << "'" << std::endl;
                const auto sequenceReportJson = SerializeSequenceReport(sequenceReport);
                WriteFileContents<SequenceReportException>(sequenceReportJson, options.GetSequenceReportFilePath().value());
            }

            return GetReturnCodeForTestSequenceResult(sequenceReport.GetResult());
        }

        //! Wrapper around impact analysis sequences to handle the case where the safe mode option is active.
        ReturnCode WrappedImpactAnalysisTestSequence(
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

            if (options.HasSafeMode())
            {
                if (options.GetTestSequenceType() == TestSequenceType::ImpactAnalysis)
                {
                    return ConsumeSequenceReportAndGetReturnCode(
                        runtime.SafeImpactAnalysisTestSequence(
                            changeList.value(),
                            options.GetTestPrioritizationPolicy(),
                            options.GetTestTargetTimeout(),
                            options.GetGlobalTimeout(),
                            SafeImpactAnalysisTestSequenceStartCallback,
                            SafeImpactAnalysisTestSequenceCompleteCallback,
                            TestRunCompleteCallback),
                        options);
                }
                else if (options.GetTestSequenceType() == TestSequenceType::ImpactAnalysisNoWrite)
                {
                    // A no-write impact analysis sequence with safe mode enabled is functionally identical to a regular sequence type
                    // due to a) the selected tests being run without instrumentation and b) the discarded tests also being run without
                    // instrumentation
                    return ConsumeSequenceReportAndGetReturnCode(
                        runtime.RegularTestSequence(
                            options.GetTestTargetTimeout(),
                            options.GetGlobalTimeout(),
                            TestSequenceStartCallback,
                            RegularTestSequenceCompleteCallback,
                            TestRunCompleteCallback),
                        options);
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
        
                return ConsumeSequenceReportAndGetReturnCode(
                    runtime.ImpactAnalysisTestSequence(
                        changeList.value(),
                        options.GetTestPrioritizationPolicy(),
                        dynamicDependencyMapPolicy,
                        options.GetTestTargetTimeout(),
                        options.GetGlobalTimeout(),
                        ImpactAnalysisTestSequenceStartCallback,
                        ImpactAnalysisTestSequenceCompleteCallback,
                        TestRunCompleteCallback),
                    options);
            }
        };

        //! Entry point for the test impact analysis framework console front end application.
        ReturnCode Main(int argc, char** argv)
        {
            try
            {
                CommandLineOptions options(argc, argv);
                AZStd::optional<ChangeList> changeList;

                // If we have a change list, check to see whether or not the client has requested the printing of said change list
                if (options.HasChangeListFilePath())
                {
                    changeList = DeserializeChangeList(ReadFileContents<CommandLineOptionsException>(*options.GetChangeListFilePath()));
                }

                // As of now, there are no non-test operations but leave this door open for the future
                if (options.GetTestSequenceType() == TestSequenceType::None)
                {
                    return ReturnCode::Success;
                }

                std::cout << "Constructing in-memory model of source tree and test coverage for test suite ";
                std::cout << SuiteTypeAsString(options.GetSuiteFilter()).c_str() << ", this may take a moment...\n";
                Runtime runtime(
                    RuntimeConfigurationFactory(ReadFileContents<CommandLineOptionsException>(options.GetConfigurationFilePath())),
                    options.GetDataFilePath(),
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

                switch (const auto type = options.GetTestSequenceType())
                {
                case TestSequenceType::Regular:
                {
                    return ConsumeSequenceReportAndGetReturnCode(
                        runtime.RegularTestSequence(
                            options.GetTestTargetTimeout(),
                            options.GetGlobalTimeout(),
                            TestSequenceStartCallback,
                            RegularTestSequenceCompleteCallback,
                            TestRunCompleteCallback),
                        options);
                }
                case TestSequenceType::Seed:
                {
                    return ConsumeSequenceReportAndGetReturnCode(
                        runtime.SeededTestSequence(
                            options.GetTestTargetTimeout(),
                            options.GetGlobalTimeout(),
                            TestSequenceStartCallback,
                            SeedTestSequenceCompleteCallback,
                            TestRunCompleteCallback),
                        options);
                }
                case TestSequenceType::ImpactAnalysisNoWrite:
                case TestSequenceType::ImpactAnalysis:
                {
                    return WrappedImpactAnalysisTestSequence(options, runtime, changeList);
                }
                case TestSequenceType::ImpactAnalysisOrSeed:
                {
                    if (runtime.HasImpactAnalysisData())
                    {
                        return WrappedImpactAnalysisTestSequence(options, runtime, changeList);
                    }
                    else
                    {
                        return ConsumeSequenceReportAndGetReturnCode(
                            runtime.SeededTestSequence(
                                options.GetTestTargetTimeout(),
                                options.GetGlobalTimeout(),
                                TestSequenceStartCallback,
                                SeedTestSequenceCompleteCallback,
                                TestRunCompleteCallback),
                            options);
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
