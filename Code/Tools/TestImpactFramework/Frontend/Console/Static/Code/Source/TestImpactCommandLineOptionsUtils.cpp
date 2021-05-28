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

#include <TestImpactCommandLineOptionsUtils.h>

namespace TestImpact
{
    //! Attempts to parse a path option value.
    AZStd::optional<RepoPath> ParsePathOption(const AZStd::string& optionName, const AZ::CommandLine& cmd)
    {
        const auto numSwitchValues = cmd.GetNumSwitchValues(optionName);
        if (numSwitchValues)
        {
            AZ_TestImpact_Eval(
                numSwitchValues == 1,
                CommandLineOptionsException,
                AZStd::string::format("Unexpected number of parameters for %s option", optionName.c_str()));

            const auto value = cmd.GetSwitchValue(optionName, 0);
            AZ_TestImpact_Eval(
                !value.empty(),
                CommandLineOptionsException,
                AZStd::string::format("%s file option value is empty", optionName.c_str()));

            return value;
        }

        return AZStd::nullopt;
    }

    //! Attempts to pass an unsigned integer option value.
    AZStd::optional<size_t> ParseUnsignedIntegerOption(const AZStd::string& optionName, const AZ::CommandLine& cmd)
    {
        const auto numSwitchValues = cmd.GetNumSwitchValues(optionName);
        if (numSwitchValues)
        {
            AZ_TestImpact_Eval(
                numSwitchValues == 1,
                CommandLineOptionsException,
                AZStd::string::format("Unexpected number of parameters for %s option", optionName.c_str()));

            const auto str = cmd.GetSwitchValue(optionName, 0);
            char* end = nullptr;
            auto value = strtoul(str.c_str(), &end, 0);

            AZ_TestImpact_Eval(
                end != str,
                CommandLineOptionsException,
                AZStd::string::format("Couldn't parse unsigned integer option value: %s", str.c_str()));

            return aznumeric_caster(value);
        }

        return AZStd::nullopt;
    }

    //! Attempts to parse an option value in seconds.
    AZStd::optional<AZStd::chrono::milliseconds> ParseSecondsOption(const AZStd::string& optionName, const AZ::CommandLine& cmd)
    {
        if (const auto option = ParseUnsignedIntegerOption(optionName, cmd);
            option.has_value())
        {
            return AZStd::chrono::seconds(option.value());
        }

        return AZStd::nullopt;
    }
}
