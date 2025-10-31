/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactChangeList.h>
#include <TestImpactFramework/TestImpactClientSequenceReportSerializer.h>
#include <TestImpactFramework/TestImpactConsoleMain.h>
#include <TestImpactFramework/TestImpactSequenceReportException.h>
#include <TestImpactFramework/TestImpactTestSequence.h>
#include <TestImpactFramework/TestImpactUtils.h>

#include <TestImpactCommandLineOptions.h>
#include <TestImpactCommandLineOptionsException.h>
#include <TestImpactTestSequenceNotificationHandler.h>

#include <AzCore/std/string/string.h>

#include <iostream>

namespace TestImpact::Console
{
    //! The set of available foreground colors.
    enum class Foreground
    {
        Black = 30,
        Red,
        Green,
        Yellow,
        Blue,
        Magenta,
        Cyan,
        White
    };

    //! The set of available background colors.
    enum class Background
    {
        Black = 40,
        Red,
        Green,
        Yellow,
        Blue,
        Magenta,
        Cyan,
        White
    };

    //! Returns a string to be used to set the specified foreground and background color.
    AZStd::string SetColor(Foreground foreground, Background background);

    //! Returns a string with the specified string set to the specified foreground and background color followed by a color reset.
    AZStd::string SetColorForString(Foreground foreground, Background background, const AZStd::string& str);

    //! Returns a string to be used to reset the color back to white foreground on black background. 
    AZStd::string ResetColor();

    //! Gets the appropriate console return code for the specified test sequence result.
    ReturnCode GetReturnCodeForTestSequenceResult(TestSequenceResult result);

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
    template<typename CommandLineOptions, typename Runtime> 
    ReturnCode WrappedImpactAnalysisTestSequence(
        const CommandLineOptions& options,
        Runtime& runtime,
        const AZStd::optional<ChangeList>& changeList,
        ConsoleOutputMode consoleOutputMode)
    {
        // Even though it is possible for a regular run to be selected (see below) which does not actually require a change list,
        // consider any impact analysis sequence type without a change list to be an error
        AZ_TestImpact_Eval(
            changeList.has_value(), CommandLineOptionsException, "Expected a change list for impact analysis but none was provided");

        if (options.HasSafeMode())
        {
            if (options.GetTestSequenceType() == TestSequenceType::ImpactAnalysis)
            {
                SafeImpactAnalysisTestSequenceNotificationHandler handler(consoleOutputMode);
                return ConsumeSequenceReportAndGetReturnCode(
                    runtime.SafeImpactAnalysisTestSequence(
                        changeList.value(),
                        options.GetTestPrioritizationPolicy(),
                        options.GetTestTargetTimeout(),
                        options.GetGlobalTimeout()),
                    options);
            }
            else if (options.GetTestSequenceType() == TestSequenceType::ImpactAnalysisNoWrite)
            {
                // A no-write impact analysis sequence with safe mode enabled is functionally identical to a regular sequence type
                // due to a) the selected tests being run without instrumentation and b) the discarded tests also being run without
                // instrumentation
                RegularTestSequenceNotificationHandler handler(consoleOutputMode);
                return ConsumeSequenceReportAndGetReturnCode(
                    runtime.RegularTestSequence(
                        options.GetTestTargetTimeout(),
                        options.GetGlobalTimeout()),
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

            ImpactAnalysisTestSequenceNotificationHandler handler(consoleOutputMode);
            return ConsumeSequenceReportAndGetReturnCode(
                runtime.ImpactAnalysisTestSequence(
                    changeList.value(),
                    options.GetTestPrioritizationPolicy(),
                    dynamicDependencyMapPolicy,
                    options.GetTestTargetTimeout(),
                    options.GetGlobalTimeout()),
                options);
        }
    };
} // namespace TestImpact::Console
