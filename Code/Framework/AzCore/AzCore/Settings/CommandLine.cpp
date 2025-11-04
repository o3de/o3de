/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/AzCore_Traits_Platform.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/Settings/CommandLine.h>
#include <AzCore/StringFunc/StringFunc.h>

namespace AZ
{
    namespace
    {
        static const AZStd::string m_emptyValue;
        // helper utility to return a lower version of the string without altering the original.
        // regular to_lower operates directly on the input.
        AZStd::string ToLower(AZStd::string_view inStr)
        {
            AZStd::string lowerStr = inStr;
            AZStd::to_lower(lowerStr.begin(), lowerStr.end());

            return lowerStr;
        }

        AZStd::string QuoteArgument(AZStd::string_view arg)
        {
            return !arg.empty() ? AZStd::string::format(R"("%.*s")", aznumeric_cast<int>(arg.size()), arg.data()) : AZStd::string{ arg };
        }
    }

    CommandLine::CommandLine()
    {
        m_optionPrefixes = Settings::Platform::GetDefaultOptionPrefixes();
    }

    CommandLine::CommandLine(Settings::CommandLineOptionPrefixSpan optionPrefixes)
    {
        // Set the option prefixes on the parser settings
        m_optionPrefixes.assign(optionPrefixes.begin(), optionPrefixes.end());
    }

    void CommandLine::ParseArgument(AZStd::string_view newOption, AZStd::string_view newValue)
    {
        // Remove any surrounding double quotes from the value
        AZStd::string_view unquotedValue = Settings::UnquoteArgument(newValue);

        // If the option is an empty string, then a positional argument is being parsed
        // store it unwrapped from any double quotes, but don't perform tokenization to split on commas
        // or semicolons
        if (newOption.empty())
        {
            m_allValues.push_back({ newOption, unquotedValue });
            return;
        }

        if (unquotedValue != newValue || unquotedValue.empty())
        {
            m_allValues.push_back({ ToLower(newOption), unquotedValue });
        }
        else
        {
            AZStd::vector<AZStd::string_view> tokens;
            auto splitArgument = [&tokens](AZStd::string_view token)
            {
                tokens.emplace_back(AZ::StringFunc::StripEnds(token));
            };
            AZ::StringFunc::TokenizeVisitor(newValue, splitArgument, ",;");

            // we do it this way because you are allowed to do odd things like
            // -root=abc -root=hij,klm -root whee -root fun;days
            // and roots value should be { abc, hij, klm, whee, fun, days }
            for (AZStd::string_view optionValue : tokens)
            {
                m_allValues.push_back({ ToLower(newOption), optionValue });
            }
        }
    }

    void CommandLine::Parse(int argc, char** argv)
    {
        m_allValues.clear();
        if (argc == 0)
        {
            return;
        }

        Settings::CommandLineParserSettings parserSettings;
        parserSettings.m_optionPrefixes = m_optionPrefixes;
        parserSettings.m_parseCommandLineEntryFunc = [this](Settings::CommandLineArgument argument)
        {
            ParseArgument(argument.m_option, argument.m_value);
            return true;
        };

        // For legacy reasons, the executable name argument(arg0) has not been parsed by this overload
        // of the command line class
        // So skip the first argument and continue parsing from the second argument(arg1)
        --argc;
        ++argv;
        Settings::ParseCommandLine(argc, argv, parserSettings);
    }

    void CommandLine::Parse(const ParamContainer& commandLine)
    {
        m_allValues.clear();

        Settings::CommandLineParserSettings parserSettings;
        parserSettings.m_optionPrefixes = m_optionPrefixes;
        parserSettings.m_parseCommandLineEntryFunc = [this](Settings::CommandLineArgument argument)
        {
            ParseArgument(argument.m_option, argument.m_value);
            return true;
        };

        AZStd::vector<AZStd::string_view> commandLineView(commandLine.begin(), commandLine.end());
        Settings::ParseCommandLine(commandLineView, parserSettings);
    }

    void CommandLine::Parse(AZStd::span<const AZStd::string_view> commandLine)
    {
        m_allValues.clear();

        // Create lambda which parses command line argument
        Settings::CommandLineParserSettings parserSettings;
        parserSettings.m_optionPrefixes = m_optionPrefixes;
        parserSettings.m_parseCommandLineEntryFunc = [this](Settings::CommandLineArgument argument)
        {
            ParseArgument(argument.m_option, argument.m_value);
            return true;
        };

        Settings::ParseCommandLine(commandLine, parserSettings);
    }

    void CommandLine::Dump(ParamContainer& commandLineDumpOutput) const
    {
        AZ_Error("CommandLine", !m_optionPrefixes.empty(),
            "Cannot dump command line switches from a command line with an empty option prefix");

        for (const CommandArgument& argument : m_allValues)
        {
            if (!argument.m_option.empty())
            {
                AZStd::string_view firstOptionPrefix = m_optionPrefixes.front();
                commandLineDumpOutput.emplace_back(AZStd::string(firstOptionPrefix) + argument.m_option);
            }
            if (!argument.m_value.empty())
            {
                commandLineDumpOutput.emplace_back(QuoteArgument(argument.m_value));
            }
        }
    }

    bool CommandLine::HasSwitch(AZStd::string_view switchName) const
    {
        auto commandArgumentIter = AZStd::find_if(m_allValues.begin(), m_allValues.end(),
            [optionName = ToLower(switchName)](const CommandArgument& argument) { return argument.m_option == optionName; });
        return commandArgumentIter != m_allValues.end();
    }

    AZStd::size_t CommandLine::GetNumSwitchValues(AZStd::string_view switchName) const
    {
        return AZStd::count_if(m_allValues.begin(), m_allValues.end(),
            [optionName = ToLower(switchName)](const CommandArgument& argument) { return argument.m_option == optionName; });
    }

    const AZStd::string& CommandLine::GetSwitchValue(AZStd::string_view switchName) const
    {
        const AZStd::size_t switchCount = GetNumSwitchValues(switchName);
        return switchCount > 0 ? GetSwitchValue(switchName, switchCount - 1) : m_emptyValue;
    }

    const AZStd::string& CommandLine::GetSwitchValue(AZStd::string_view switchName, AZStd::size_t index) const
    {
        AZStd::string optionName = ToLower(switchName);
        size_t currentPosIndex{};
        auto findArgumentAt = [&optionName, index, &currentPosIndex](const CommandArgument& argument)
        {
            if (argument.m_option == optionName)
            {
                return currentPosIndex++ == index;
            }

            return false;
        };
        auto commandArgumentIter = AZStd::find_if(m_allValues.begin(), m_allValues.end(), findArgumentAt);
        if (!optionName.empty())
        {
            AZ_Assert(index < currentPosIndex, R"(Invalid Command line optional argument lookup of "%s" at index %zu)",
                optionName.c_str(), index);
        }
        else
        {
            AZ_Assert(index < currentPosIndex, R"(Invalid Command line positional argument lookup at index %zu)", index);
        }
        if (commandArgumentIter == m_allValues.end())
        {
            return m_emptyValue;
        }
        return commandArgumentIter->m_value;
    }

    AZStd::size_t CommandLine::GetNumMiscValues() const
    {
        return GetNumSwitchValues("");
    }

    const AZStd::string& CommandLine::GetMiscValue(AZStd::size_t index) const
    {
        // Positional arguments option value is an empty string
        return GetSwitchValue("", index);
    }

    [[nodiscard]] bool CommandLine::empty() const
    {
        return m_allValues.empty();
    }

    auto CommandLine::size() const -> ArgumentVector::size_type
    {
        return m_allValues.size();
    }
    auto CommandLine::begin() -> ArgumentVector::iterator
    {
        return m_allValues.begin();
    }
    auto CommandLine::begin() const -> ArgumentVector::const_iterator
    {
        return m_allValues.begin();
    }
    auto CommandLine::cbegin() const -> ArgumentVector::const_iterator
    {
        return m_allValues.cbegin();
    }
    auto CommandLine::end() -> ArgumentVector::iterator
    {
        return m_allValues.end();
    }
    auto CommandLine::end() const -> ArgumentVector::const_iterator
    {
        return m_allValues.end();
    }
    auto CommandLine::cend() const -> ArgumentVector::const_iterator
    {
        return m_allValues.cend();
    }
    auto CommandLine::rbegin() -> ArgumentVector::reverse_iterator
    {
        return m_allValues.rbegin();
    }
    auto CommandLine::rbegin() const -> ArgumentVector::const_reverse_iterator
    {
        return m_allValues.rbegin();
    }
    auto CommandLine::crbegin() const -> ArgumentVector::const_reverse_iterator
    {
        return m_allValues.crbegin();
    }
    auto CommandLine::rend() -> ArgumentVector::reverse_iterator
    {
        return m_allValues.rend();
    }
    auto CommandLine::rend() const -> ArgumentVector::const_reverse_iterator
    {
        return m_allValues.rend();
    }
    auto CommandLine::crend() const -> ArgumentVector::const_reverse_iterator
    {
        return m_allValues.crend();
    }
}
