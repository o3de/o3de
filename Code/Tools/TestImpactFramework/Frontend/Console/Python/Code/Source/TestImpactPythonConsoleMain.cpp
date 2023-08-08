/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactFramework/TestImpactConsoleMain.h>
#include <TestImpactFramework/TestImpactChangeListException.h>
#include <TestImpactFramework/TestImpactChangeListSerializer.h>
#include <TestImpactFramework/TestImpactConfigurationException.h>
#include <TestImpactFramework/TestImpactSequenceReportException.h>
#include <TestImpactFramework/Python/TestImpactPythonRuntime.h>
#include <TestImpactFramework/Python/TestImpactPythonRuntimeConfigurationFactory.h>

#include <TestImpactConsoleUtils.h>
#include <TestImpactPythonCommandLineOptions.h>

namespace TestImpact::Console
{
    //! Entry point for the test impact analysis framework console front end application.
    ReturnCode Main(int argc, char** argv)
    {
        try
        {
            const PythonCommandLineOptions options(argc, argv);
            AZStd::optional<ChangeList> changeList;

            if (options.HasChangeListFilePath())
            {
                changeList = DeserializeChangeList(ReadFileContents<CommandLineOptionsException>(*options.GetChangeListFilePath()));
            }

            if (options.GetTestSequenceType() == TestSequenceType::None)
            {
                std::cout << "No test operations specified.";
                return ReturnCode::Success;
            }

            std::cout << "Constructing in-memory model of source tree and test coverage for test suite ";
            std::cout << SuiteSetAsString(options.GetSuiteSet()).c_str() << ", this may take a moment...\n";
            PythonRuntime runtime(
                PythonRuntimeConfigurationFactory(ReadFileContents<CommandLineOptionsException>(options.GetConfigurationFilePath())),
                options.GetDataFilePath(),
                options.GetPreviousRunDataFilePath(),
                options.GetExcludedTests(),
                options.GetSuiteSet(),
                options.GetSuiteLabelExcludeSet(),
                options.GetExecutionFailurePolicy(),
                options.GetFailedTestCoveragePolicy(),
                options.GetTestFailurePolicy(),
                options.GetIntegrityFailurePolicy(),
                options.GetTargetOutputCapture(),
                options.GetTestRunnerPolicy());

            if (runtime.HasImpactAnalysisData())
            {
                std::cout << "Test impact analysis data for this repository was found.\n";
            }
            else
            {
                std::cout
                    << "Test impact analysis data for this repository was not found, seed or regular sequence fallbacks will be used.\n";
            }

            // Use realtime console output as Python tests aren't run concurrently and can cause Jenkins to timeout on long
            // tests if nothing is outputted to the console
            const auto consoleOutputMode = ConsoleOutputMode::Realtime;

            switch (const auto type = options.GetTestSequenceType())
            {
            case TestSequenceType::Regular:
            {
                RegularTestSequenceNotificationHandler handler(consoleOutputMode);
                return ConsumeSequenceReportAndGetReturnCode(
                    runtime.RegularTestSequence(options.GetTestTargetTimeout(), options.GetGlobalTimeout()), options);
            }
            case TestSequenceType::Seed:
            {
                SeedTestSequenceNotificationHandler handler(consoleOutputMode);
                return ConsumeSequenceReportAndGetReturnCode(
                    runtime.SeededTestSequence(options.GetTestTargetTimeout(), options.GetGlobalTimeout()), options);
            }
            case TestSequenceType::ImpactAnalysisNoWrite:
            case TestSequenceType::ImpactAnalysis:
            {
                return WrappedImpactAnalysisTestSequence(options, runtime, changeList, consoleOutputMode);
            }
            case TestSequenceType::ImpactAnalysisOrSeed:
            {
                if (runtime.HasImpactAnalysisData())
                {
                    return WrappedImpactAnalysisTestSequence(options, runtime, changeList, consoleOutputMode);
                }
                else
                {
                    SeedTestSequenceNotificationHandler handler(consoleOutputMode);
                    return ConsumeSequenceReportAndGetReturnCode(
                        runtime.SeededTestSequence(options.GetTestTargetTimeout(), options.GetGlobalTimeout()), options);
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
            std::cout << PythonCommandLineOptions::GetCommandLineUsageString().c_str() << std::endl;
            return ReturnCode::InvalidArgs;
        }
        catch (const ChangeListException& e)
        {
            std::cout << e.what() << std::endl;
            return ReturnCode::InvalidChangeList;
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
} // namespace TestImpact::Console
