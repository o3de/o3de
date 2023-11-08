/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/std/containers/span_fwd.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/string/fixed_string.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/std/utility/expected.h>

namespace AZ::Settings
{
    //! Stores the prefix used for option arguments
    //! The cap is MaxOptionPrefixes
    //! The prefixes can't be larger than 4 characters
    inline static constexpr size_t MaxOptionPrefixes = 8;
    inline static constexpr size_t MaxOptionPrefixSize = 4;
    using CommandLineOptionPrefixString = AZStd::basic_fixed_string<char, MaxOptionPrefixSize, AZStd::char_traits<char>>;
    using CommandLineOptionPrefixArray = AZStd::fixed_vector<CommandLineOptionPrefixString, MaxOptionPrefixes>;
    using CommandLineOptionPrefixSpan = AZStd::span<CommandLineOptionPrefixString const>;
}

namespace AZ::Settings::Platform
{
    CommandLineOptionPrefixArray GetDefaultOptionPrefixes();
}

namespace AZ::Settings
{
    //! Struct which contains a string view for option name and the value of a command line argument
    //! If the command line argument is a positional argument, then the m_option string_view is empty
    struct CommandLineArgument
    {
        AZStd::string_view m_option;
        AZStd::string_view m_value;
    };

    //! Settings structure which is used to parse CLI command line parameters
    //! It is setup to invoke an callback with each argument option name and value
    //! to allow parsing of the command line without allocating any heap memory
    //! This can be used to parse the command line in order to read in settings
    //! which can configure memory allocators before they are available without the need
    //! to allocate any heap memory as long as the user supplied callback itself is careful
    //! in how it stores its settings.
    //! Suggestion: Use structures such as fixed_vector and fixed_string to store the settings
    //! in the user callback to avoid heap allocations and rely entirely on stack memory instead
    struct CommandLineParserSettings
    {
        //! Callback invoked for positional argument or a completed option argument.
        //! A completed option argument will contain any value associated with the option name
        //! This is the the only member that is required to be set within this struct
        //! All other members are defaulted to work with command line arguments
        //! IMPORTANT: Any lambda functions or class instances that are larger than 16 bytes
        //! in size requires a memory allocation to store.
        //! So it is recommended that users binding a lambda bind at most 2 reference or pointer members
        //! to avoid dynamic heap allocations
        //!
        //! @return True should be returned from this function to indicate that parsing of the config entry
        //! has succeeded
        using ParseCommandLineEntryFunc = AZStd::function<bool(const CommandLineArgument&)>;

        //! Callback function which is invoked with the parsed command line argument
        ParseCommandLineEntryFunc m_parseCommandLineEntryFunc;

        //! Determines what kind of action should be taken for the option flag that was parsed
        //! An option of the form -key or --key
        enum class OptionAction
        {
            //! This indicates that the next "non-option" token should be parsed
            //! as the value for the current option being parsed
            //! A non-option token is one that doesn't start with an option prefix such as '-' or '--'
            //! For a token sequence of `--project-path <value>`
            //! "<value>" is used as value for "project-path" option
            //! However for a token sequence of `--project-path --project-cache-path <value>`
            //! the "project-path" option uses "" as its value because the very next token is an option of "--project-cache-path"
            //! the "project-cache-path" does have its value set to "<value>"
            NextTokenIsValueIfNonOption,
            //! This indicates that the option is a flag
            //! and contains no additional value parameters
            //! For a token sequence of `--force <value>`
            //! The "force" option uses a value of "". This allows the user to treated is a flag option
            //! The "<value>" token in this case is treated as positional parameter
            IsFlag,
            // This indicates the the next token should be parsed as the value in all cases
            // Even if the next token is an option (--foo)
            // So a token sequence of `-project-path --project-cache-path <value>`
            // will have the "project-path" option value set to "--project-cache-path"
            // The "<value>" token would then be parsed as a positional parameter
            NextTokenIsValue
        };
        //! Determines the default action the command line should take when it parses an option token WITHOUT a value
        //! @param option string containing the name of the parsed command line option
        //! @return The default action to take for the option.
        //! The default return value is to treat the next command line token as the value for the option string
        static OptionAction DefaultOptionAction(AZStd::string_view option);

        //! Callback function which is invoked when an option token by itself is parsed
        //! @return The type of action to take for the option, which is to either parse the next command line token
        //! as the value for the option or treat the option as just a CLI flag, such as `--quiet` or `-verbose`
        using OptionActionFunc = AZStd::function<OptionAction(AZStd::string_view option)>;

        //! Function invoked ONLY after parsing an option Token that was not split from a CLI delimiter such as an '='
        //! The algorithm that determines when this function is called is explained with the following sample command line arguments
        //! `Editor --project-path=<project-path> --force --engine-path <engine-path> positionalArgument
        //! For the "project-path" option, this function is NOT invoked since the value is gathered by spliting the CLI token on '='
        //! result: option="project-path", value="<project-path>"
        //! For the "force" option, this function IS invoked to determine how the next token should be treated
        //! If the function returns `OptionAction::NextTokenIsValueIfNonOption`, then the following "--engine-path" token is read as an option.
        //! If the function returns `OptionAction::IsFlag`, then the following "--engine-path" token is read as an option.
        //! If the function returns `OptionAction::NextTokenIsValue`, then the following "--engine-path" token is treated as the value for the
        //! "force" option
        OptionActionFunc m_optionActionFunc = &DefaultOptionAction;

        //! Splits a command line option token into an option and value
        //! Surrounding whitespace around the key and value are not part of the string views
        //! @param token to split into an option and value
        //! @return option, value pair structure representing the command line argument option name and value
        static AZStd::optional<CommandLineArgument> DefaultOptionDelimiter(AZStd::string_view token);

        //! Callback function which is invoked when an option argument(-short=value or --long=value) is processed
        //! to split the the option into an option name value pair
        //! @return Argument containing the option name and Token
        using OptionDelimiterFunc = AZStd::function<AZStd::optional<CommandLineArgument>(AZStd::string_view token)>;

        //! Function which is invoked to split an option into an option name and option value pair
        OptionDelimiterFunc m_optionDelimiterFunc = &DefaultOptionDelimiter;

        //! By default the option prefixes are "--" and "-"
        //! NOTE: The order is important here as double dash is checked first before single dash
        //! to make sure an argument such as `--foo bar` parses both dashes before the option name.
        //! Otherwise the option name would be parsed as "-foo" which is incorrect.
        //! The correct parsing is "foo" for the option name
        CommandLineOptionPrefixArray m_optionPrefixes{ Platform::GetDefaultOptionPrefixes() };

        //! Separator which is used to separate normal argument parsing(option/positional) from just positional argument parsing
        //! Positional arguments can still be parsed before the separator, however options will not be parsed
        //! (i.e `-- --foo bar`) will parse `--foo` as a positional argument not an option with a value of "bar"
        AZStd::string_view m_positionalArgSeparator = "--";
    };

    // Expected structure which encapsulates the result of the ParseCommandLine function
    using CommandLineParseErrorString = AZStd::basic_fixed_string<char, 512, AZStd::char_traits<char>>;
    using CommandLineParseOutcome = AZStd::expected<void, CommandLineParseErrorString>;

    //! Parses a set of command line arguments
    //! This function will NOT allocate any heap memory on its own
    //! It is safe to call without any AZ Allocators being initialized
    //! NOTE: All arguments are parsed including the argument at index 0
    //!
    //! @param commandLine A contiguous range of string_view arguments pointing to each command line token
    //! @param configParseSettings struct defining how the command line arguments should be parsed
    //! @return success outcome if the each token on the command line was parsed successfully,
    //! otherwise a failure string with the parse error is returned
    CommandLineParseOutcome ParseCommandLine(
        AZStd::span<AZStd::string_view const> commandLine, const CommandLineParserSettings& commandLineParserSettings);

    //! Parses a set of command line arguments using the defacto integer argument count parameter and char double pointer to the argument
    //! array
    //! NOTE: All arguments are parsed including the argument at index 0, which is normally used for the executable name
    //! @param argc number of command line arguments
    //! @param argv pointer to the begining of array of c-strings pointing to each command line argument
    CommandLineParseOutcome ParseCommandLine(int argc, char** argv, const CommandLineParserSettings& commandLineParserSettings);

    //! Removes surrounding double quotes around a command line argument value
    AZStd::string_view UnquoteArgument(AZStd::string_view arg);
} // namespace AZ::Settings
