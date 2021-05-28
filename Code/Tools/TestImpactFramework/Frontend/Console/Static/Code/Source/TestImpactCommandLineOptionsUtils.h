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
        const AZStd::string& optionName,
        const AZStd::pair<OptionValue<T>,
        OptionValue<T>>& state, const AZ::CommandLine& cmd)
    {
        if (const auto numSwitchValues = cmd.GetNumSwitchValues(optionName);
            numSwitchValues)
        {
            AZ_TestImpact_Eval(
                numSwitchValues == 1,
                CommandLineOptionsException,
                AZStd::string::format("Unexpected number of parameters for %s option", optionName.c_str()));

            const auto option = cmd.GetSwitchValue(optionName, 0);
            if (option == state.first.first)
            {
                return state.first.second;
            }
            else if (option == state.second.first)
            {
                return state.second.second;
            }

            throw CommandLineOptionsException(
                AZStd::string::format("Unexpected value for %s option: %s", optionName.c_str(), option.c_str()));
        }

        return AZStd::nullopt;
    }

    //! Attempts to pass an arbitrarily sized state option.
    template<typename T>
    AZStd::optional<T> ParseMultiStateOption(
        const AZStd::string& optionName,
        const AZStd::vector<AZStd::pair<AZStd::string, T>>& states,
        const AZ::CommandLine& cmd)
    {
        if (const auto numSwitchValues = cmd.GetNumSwitchValues(optionName);
            numSwitchValues)
        {
            AZ_TestImpact_Eval(
                numSwitchValues == 1,
                CommandLineOptionsException,
                AZStd::string::format("Unexpected number of parameters for %s option", optionName.c_str()));

            const auto option = cmd.GetSwitchValue(optionName, 0);
            for (const auto& state : states)
            {
                if (option == state.first)
                {
                    return state.second;
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
        return ParseBinaryStateOption(optionName, BinaryStateOption<T>{ {"off", states.first}, { "on", states.second } }, cmd);
    }

    //! Attempts to pass a specialization of the binary state option where the command line values are "abort" and "continue".
    template<typename T>
    AZStd::optional<T> ParseAbortContinueOption(const AZStd::string& optionName, const AZStd::pair<T, T>& states, const AZ::CommandLine& cmd)
    {
        return ParseBinaryStateOption(optionName, BinaryStateOption<T>{ {"abort", states.first}, { "continue", states.second } }, cmd);
    }

    //! Attempts to parse a path option value.
    AZStd::optional<RepoPath> ParsePathOption(const AZStd::string& optionName, const AZ::CommandLine& cmd);

    //! Attempts to pass an unsigned integer option value.
    AZStd::optional<size_t> ParseUnsignedIntegerOption(const AZStd::string& optionName, const AZ::CommandLine& cmd);

    //! Attempts to parse an option value in seconds.
    AZStd::optional<AZStd::chrono::milliseconds> ParseSecondsOption(const AZStd::string& optionName, const AZ::CommandLine& cmd);
} // namespace TestImpact
