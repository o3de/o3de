/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactCommandLineOptions.h>
#include <TestImpactCommandLineOptionsException.h>

#include <AzCore/Settings/CommandLine.h>

namespace TestImpact
{
    //! Representation of a command line option value name and its typed value.
    template<typename T>
    using OptionValue = AZStd::pair<AZStd::string, T>;

    //! Representation of a binary state command line option with its two values.
    template<typename T>
    using BinaryStateOption = AZStd::pair<OptionValue<T>, OptionValue<T>>;

    //! Representation of the values for a binary state option.
    template<typename T>
    using BinaryStateValue = AZStd::pair<T, T>;

    //! Attempts to parse the specified binary state option.
    template<typename T>
    AZStd::optional<T> ParseBinaryStateOption(
        const AZStd::string& optionName, const AZStd::pair<OptionValue<T>, OptionValue<T>>& state, const AZ::CommandLine& cmd)
    {
        if (const auto numSwitchValues = cmd.GetNumSwitchValues(optionName); numSwitchValues)
        {
            AZ_TestImpact_Eval(
                numSwitchValues == 1,
                CommandLineOptionsException,
                AZStd::string::format("Unexpected number of parameters for %s option", optionName.c_str()));

            const auto option = cmd.GetSwitchValue(optionName, 0);
            if (const auto& [optionValueText, optionValue] = state.first; option == optionValueText)
            {
                return optionValue;
            }
            if (const auto& [optionValueText, optionValue] = state.second; option == optionValueText)
            {
                return optionValue;
            }

            throw CommandLineOptionsException(
                AZStd::string::format("Unexpected value for %s option: %s", optionName.c_str(), option.c_str()));
        }

        return AZStd::nullopt;
    }

    //! Attempts to pass an arbitrarily sized state option.
    template<typename T>
    AZStd::optional<T> ParseMultiStateOption(
        const AZStd::string& optionName, const AZStd::vector<AZStd::pair<AZStd::string, T>>& states, const AZ::CommandLine& cmd)
    {
        if (const auto numSwitchValues = cmd.GetNumSwitchValues(optionName); numSwitchValues)
        {
            AZ_TestImpact_Eval(
                numSwitchValues == 1,
                CommandLineOptionsException,
                AZStd::string::format("Unexpected number of parameters for %s option", optionName.c_str()));

            const auto option = cmd.GetSwitchValue(optionName, 0);
            for (const auto& state : states)
            {
                if (const auto& [optionValueText, optionValue] = state; option == optionValueText)
                {
                    return optionValue;
                }
            }

            throw CommandLineOptionsException(
                AZStd::string::format("Unexpected value for %s option: %s", optionName.c_str(), option.c_str()));
        }

        return AZStd::nullopt;
    }

    //! Attempts to pass a specialization of the binary state option where the command line values are "on" and "off".
    template<typename T>
    AZStd::optional<T> ParseOnOffOption(const AZStd::string& optionName, const AZStd::pair<T, T>& states, const AZ::CommandLine& cmd)
    {
        return ParseBinaryStateOption(optionName, BinaryStateOption<T>{ { "off", states.first }, { "on", states.second } }, cmd);
    }

    //! Attempts to pass a specialization of the binary state option where the command line values are "abort" and "continue".
    template<typename T>
    AZStd::optional<T> ParseAbortContinueOption(
        const AZStd::string& optionName, const AZStd::pair<T, T>& states, const AZ::CommandLine& cmd)
    {
        return ParseBinaryStateOption(optionName, BinaryStateOption<T>{ { "abort", states.first }, { "continue", states.second } }, cmd);
    }

    //! Attempts to pass a specialization of the binary state option where the command line values are "live" and "null".
    template<typename T>
    AZStd::optional<T> ParseLiveNullOption(const AZStd::string& optionName, const AZStd::pair<T, T>& states, const AZ::CommandLine& cmd)
    {
        return ParseBinaryStateOption(optionName, BinaryStateOption<T>{ { "live", states.first }, { "null" , states.second } }, cmd);
    }

    //! Attempts to parse a multi-value option.
    AZStd::set<AZStd::string> ParseMultiValueOption(const AZStd::string& optionName, const AZ::CommandLine& cmd);

    //! Attempts to parse a path option value.
    AZStd::optional<RepoPath> ParsePathOption(const AZStd::string& optionName, const AZ::CommandLine& cmd);

    //! Attempts to pass an unsigned integer option value.
    AZStd::optional<size_t> ParseUnsignedIntegerOption(const AZStd::string& optionName, const AZ::CommandLine& cmd);

    //! Attempts to parse an option value in seconds.
    AZStd::optional<AZStd::chrono::milliseconds> ParseSecondsOption(const AZStd::string& optionName, const AZ::CommandLine& cmd);

    //! Attempts to parse the file data into a JSON array of test names.
    AZStd::vector<ExcludedTarget> ParseExcludedTestTargetsFromFile(const AZStd::string& fileData);
} // namespace TestImpact
