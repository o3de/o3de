/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactCommandLineOptionsUtils.h>

#include <AzCore/std/string/conversions.h>
#include <AzCore/JSON/document.h>

namespace TestImpact
{
    AZStd::set<AZStd::string> ParseMultiValueOption(const AZStd::string& optionName, const AZ::CommandLine& cmd)
    {
        AZStd::set<AZStd::string> values;
        if (const auto numSwitchValues = cmd.GetNumSwitchValues(optionName);
            numSwitchValues > 0)
        {
            for (size_t i = 0; i < numSwitchValues; i++)
            {
                values.insert(cmd.GetSwitchValue(optionName, i));
            }
        }

        return values;
    }

    AZStd::optional<RepoPath> ParsePathOption(const AZStd::string& optionName, const AZ::CommandLine& cmd)
    {  
        if (const auto numSwitchValues = cmd.GetNumSwitchValues(optionName);
            numSwitchValues > 0)
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

    AZStd::optional<size_t> ParseUnsignedIntegerOption(const AZStd::string& optionName, const AZ::CommandLine& cmd)
    {
        if (const auto numSwitchValues = cmd.GetNumSwitchValues(optionName);
            numSwitchValues > 0)
        {
            AZ_TestImpact_Eval(
                numSwitchValues == 1,
                CommandLineOptionsException,
                AZStd::string::format("Unexpected number of parameters for %s option", optionName.c_str()));

            const auto strValue = cmd.GetSwitchValue(optionName, 0);
            size_t successfulParse = 0; // Will be non-zero if the parse was successful
            auto value = AZStd::stoul(strValue, &successfulParse, 0);

            AZ_TestImpact_Eval(
                successfulParse,
                CommandLineOptionsException,
                AZStd::string::format("Couldn't parse unsigned integer option value: %s", strValue.c_str()));

            return aznumeric_caster(value);
        }

        return AZStd::nullopt;
    }

    AZStd::optional<AZStd::chrono::milliseconds> ParseSecondsOption(const AZStd::string& optionName, const AZ::CommandLine& cmd)
    {
        if (const auto option = ParseUnsignedIntegerOption(optionName, cmd);
            option.has_value())
        {
            return AZStd::chrono::seconds(option.value());
        }

        return AZStd::nullopt;
    }

    AZStd::vector<ExcludedTarget> ParseExcludedTestTargetsFromFile(const AZStd::string& fileData)
    {
        rapidjson::Document excludeData;
        excludeData.Parse(fileData.c_str());
        AZStd::vector<ExcludedTarget> targetExcludeList;
        for (const auto& testExclude : excludeData["exclude"].GetArray())
        {
            ExcludedTarget excludedTarget;
            excludedTarget.m_name = testExclude["target"].GetString();
            if (testExclude.HasMember("tests"))
            {
                const auto& excludedTests = testExclude["tests"].GetArray();
                for (const auto& excludedTest : excludedTests)
                {
                    excludedTarget.m_excludedTests.push_back(excludedTest.GetString());
                }
            }

            targetExcludeList.push_back(excludedTarget);
        }

        return targetExcludeList;
    }
} // namespace TestImpact
