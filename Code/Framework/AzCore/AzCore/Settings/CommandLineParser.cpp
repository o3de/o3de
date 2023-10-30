/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Settings/CommandLineParser.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/std/string/conversions.h>

namespace AZ::Settings
{
    //! Structure which tracks the current state of parsing the command line
    //! It stores whether the remaining arguments should be treated as positional
    //! in case a '--' argument has been parsed
    //! It also tracks the previous command token if it is an option, so that it can
    //! be paired with a value
    struct CommandArgumentParserState
    {
        //! The current argument is the final token in the command line token sequence
        //! The reason this needs to be tracked is just in case an option argument
        //! is the final token of a command line sequence, the callback function
        //! for the user would still need to trigger in order to allow handling of
        //! that argument
        bool m_finalToken{};
        //! Set to true if the psuedo argument '--' has been parsed
        //! This indicates to the argument parser that the remaining
        //! arguments should be treated as positional arguments only
        bool m_parseRemainingArgsAsPositional{};

        //! A view of the option being parsed
        //! This is used for maintaining state for space separated options
        //! Ex. --foo bar
        //! When the "bar" argument is parsed the current switch would be set to "foo"
        AZStd::string_view m_currentOption;

        //! Determines how the next token should be parsed for the current option
        CommandLineParserSettings::OptionAction m_currentOptionAction{ CommandLineParserSettings::OptionAction::NextTokenIsValueIfNonOption };
    };

    auto CommandLineParserSettings::DefaultOptionDelimiter(AZStd::string_view token) -> AZStd::optional<CommandLineArgument>
    {
        constexpr AZStd::string_view CommandLineArgumentDelimiters{ "=" };

        // If no characters in the delimiter set can be found in the token,
        // then return a nullopt to indicate that the option cannot be split
        if (CommandLineArgumentDelimiters.find_first_of(token) == AZStd::string_view::npos)
        {
            return AZStd::nullopt;
        }

        CommandLineArgument argument;

        // Moves optionToken past any delimiters via the StringFunc::TokenizeNext function
        // If there are no delimiters in the string, then optionToken would be pointing to the end of view
        // where return value would be view pointing to the entire token
        // Therefore return value would always point to the option and the optionToken would always point to the value
        if (auto argumentOpt = AZ::StringFunc::TokenizeNext(token, CommandLineArgumentDelimiters); argumentOpt.has_value())
        {
            // The TokenizeNext function moves the string_view to next token after the equal sign
            // so optionToken now actually refers to the argument value
            argument.m_option = AZ::StringFunc::StripEnds(argumentOpt.value());
            argument.m_value = AZ::StringFunc::StripEnds(token);
        }

        return argument;
    };

    auto CommandLineParserSettings::DefaultOptionAction(AZStd::string_view) -> OptionAction
    {
        // Use the next token as the value for the option if it is a non-option token
        // i.e `--foo <value>` will use <value> as the value for option "foo"
        // but `--foo --bar` will use empty string as the value for option "foo" since the next token is an option
        return OptionAction::NextTokenIsValueIfNonOption;
    };

    //! Removes surrounding double quotes around a command line argument value
    AZStd::string_view UnquoteArgument(AZStd::string_view arg)
    {
        if (arg.size() < 2)
        {
            return arg;
        }

        return arg.front() == '"' && arg.back() == '"' ? AZStd::string_view{ arg.begin() + 1, arg.end() - 1 } : arg;
    }

    static CommandLineParseOutcome InvokeCommandLineEntryCallback(
        const CommandLineParserSettings::ParseCommandLineEntryFunc& parseCommandLineEntryFunc,
        const CommandLineArgument& commandLineArgument)
    {
        CommandLineParseOutcome parseOutcome;
        if (!parseCommandLineEntryFunc(commandLineArgument))
        {
            parseOutcome = AZStd::unexpected(CommandLineParseErrorString("The Parse Command Line Entry callback returned false for"));
            if (commandLineArgument.m_option.empty())
            {
                parseOutcome.error() += R"( positional argument value=")";
                parseOutcome.error() += commandLineArgument.m_value;
                parseOutcome.error() += R"(")";
            }
            else
            {
                parseOutcome.error() += R"( option argument name=")";
                parseOutcome.error() += commandLineArgument.m_option;
                if (!commandLineArgument.m_value.empty())
                {
                    // Append the value to failure message if there is one
                    parseOutcome.error() += R"(", value=")";
                    parseOutcome.error() += commandLineArgument.m_value;
                }

                parseOutcome.error() += R"(")";
            }

            // Add newline to the end of the failure message
            parseOutcome.error() += '\n';
        }

        return parseOutcome;
    }

    //! Used by the subfunctions that are called by the ParseArgument function
    //! to indicate whether the ParseArgument function should continue
    //! to parse the current command line current token or continue
    enum class ParseArgumentAction
    {
        //! Indicate that the current token should continue to be parsed in ParseArgument
        ContinueParsing,
        //! Indicates the parsing of the current token should stop and move to the next token
        ParseNextToken
    };
    // Finish parsing an option from the previous call to ParseArgument
    static ParseArgumentAction FinishOptionParse(
        CommandLineParseOutcome& parseOutcome,
        CommandArgumentParserState& argumentParserState,
        AZStd::string_view currentToken,
        const CommandLineParserSettings& commandLineParserSettings,
        bool currentTokenIsOption)
    {
        ParseArgumentAction parseArgumentAction = ParseArgumentAction::ContinueParsing;
        CommandLineArgument commandLineArgument;
        // If the argument parser state parsed the previous token as an option
        // then depending on the OptionAction, the current token may be a value for that option
        if (!argumentParserState.m_currentOption.empty())
        {
            switch (argumentParserState.m_currentOptionAction)
            {
            case CommandLineParserSettings::OptionAction::NextTokenIsValueIfNonOption:
                commandLineArgument.m_option = AZStd::move(argumentParserState.m_currentOption);
                if (!currentTokenIsOption)
                {
                    commandLineArgument.m_value = currentToken;
                    // The current token is the value for the current option entry, so proceed to the next token
                    parseArgumentAction = ParseArgumentAction::ParseNextToken;
                }
                parseOutcome = InvokeCommandLineEntryCallback(commandLineParserSettings.m_parseCommandLineEntryFunc, commandLineArgument);
                break;
            case CommandLineParserSettings::OptionAction::NextTokenIsValue:
                commandLineArgument.m_option = AZStd::move(argumentParserState.m_currentOption);
                commandLineArgument.m_value = currentToken;
                parseOutcome = InvokeCommandLineEntryCallback(commandLineParserSettings.m_parseCommandLineEntryFunc, commandLineArgument);
                // The current token is the value for the current option entry, so proceed to the next token
                parseArgumentAction = ParseArgumentAction::ParseNextToken;
            }

            // Reset the current option parsing parameter back to an empty string as the next argument has now been processed
            argumentParserState.m_currentOption = {};
            argumentParserState.m_currentOptionAction = CommandLineParserSettings::OptionAction::NextTokenIsValueIfNonOption;
        }

        return parseArgumentAction;
    }

    static CommandLineParseOutcome ParseArgument(
        CommandArgumentParserState& argumentParserState,
        AZStd::string_view currentToken,
        const CommandLineParserSettings& commandLineParserSettings)
    {
        CommandLineParseOutcome parseOutcome;
        // Strip any surrounding whitespace from the token
        currentToken = AZ::StringFunc::StripEnds(currentToken);
        if (currentToken.empty())
        {
            // The token is empty so there is nothing to parse for the current token
            // return a successful expectation
            return parseOutcome;
        }

        CommandLineArgument commandLineArgument;

        // Determine if the argument should be parsed as positional argument or option
        bool tokenIsOption{};
        if (!argumentParserState.m_parseRemainingArgsAsPositional)
        {
            // If the token matches the positional argument separator update the parser state
            // to indicate that future tokens should be parsed as positional arguments
            // and return to continue to the next token
            if (currentToken == commandLineParserSettings.m_positionalArgSeparator)
            {
                argumentParserState.m_parseRemainingArgsAsPositional = true;
                argumentParserState.m_currentOption = {};
                argumentParserState.m_currentOptionAction = CommandLineParserSettings::OptionAction::NextTokenIsValueIfNonOption;
                return parseOutcome;
            }

            // Check if the token is an option
            for (const CommandLineOptionPrefixString& optionPrefix : commandLineParserSettings.m_optionPrefixes)
            {
                if (currentToken.starts_with(optionPrefix))
                {
                    // An option prefix has been found, so parse the token as an option
                    // by making a sub view after the option prefix token and break out of the loop
                    currentToken = currentToken.substr(optionPrefix.size());
                    tokenIsOption = argumentParserState.m_currentOptionAction != CommandLineParserSettings::OptionAction::NextTokenIsValue;
                    break;
                }
            }

            ParseArgumentAction parseArgumentAction =
                FinishOptionParse(parseOutcome, argumentParserState, currentToken, commandLineParserSettings, tokenIsOption);
            // If the current token was treated as the "value" for the previous token "option", then the parsing for the current token has
            // completed
            if (parseArgumentAction == ParseArgumentAction::ParseNextToken)
            {
                return parseOutcome;
            }

            if (tokenIsOption)
            {
                // Default the option field to be the current token
                commandLineArgument.m_option = currentToken;
                // If there is a delimiter function try to split the current token
                // into a option, value pair
                AZStd::optional<CommandLineArgument> commandArgumentOpt;
                if (commandLineParserSettings.m_optionDelimiterFunc)
                {
                    commandArgumentOpt = commandLineParserSettings.m_optionDelimiterFunc(currentToken);
                }

                // The option contains a delimiter, so store the complete argument into the command line entry structure
                if (commandArgumentOpt.has_value())
                {
                    commandLineArgument = AZStd::move(commandArgumentOpt.value());
                }
                else
                {
                    // The option wasn't split on a delimiter so it may be a case
                    // where the next token is the value for the option.
                    // such as --project-path <value>

                    CommandLineParserSettings::OptionAction optionAction = commandLineParserSettings.m_optionActionFunc
                        ? commandLineParserSettings.m_optionActionFunc(commandLineArgument.m_option)
                        : CommandLineParserSettings::OptionAction::NextTokenIsValueIfNonOption;

                    // Check if the option token is incomplete in that there would be no additional value token
                    // afterwards that can be associated with it.
                    // If the option token is the final token in the command line sequence, then it is always complete
                    // This allows the command line entry callback to be inovked at the end of this function
                    // in the scenario where the final token is an option
                    // i.e `app.exe register --project-path <value> --force`
                    const bool isOptionIncomplete = !argumentParserState.m_finalToken &&
                        (optionAction == CommandLineParserSettings::OptionAction::NextTokenIsValueIfNonOption ||
                         optionAction == CommandLineParserSettings::OptionAction::NextTokenIsValue);
                    if (isOptionIncomplete)
                    {
                        // Store the current option in the argument parser state
                        // and the next iteration of this function will parse the value
                        argumentParserState.m_currentOption = currentToken;
                        argumentParserState.m_currentOptionAction = optionAction;

                        // Return here as the option requires the next token to determine if it is complete
                        return parseOutcome;
                    }
                }
            }
        }

        // If the token is not an option or the remaining arguments are treated
        // as positional, then add set the value entry for the argument
        if (!tokenIsOption)
        {
            // The argument is treated has a positional in this case
            commandLineArgument.m_value = currentToken;
        }

        return InvokeCommandLineEntryCallback(commandLineParserSettings.m_parseCommandLineEntryFunc, commandLineArgument);
    }

    CommandLineParseOutcome ParseCommandLine(int argc, char** argv, const CommandLineParserSettings& commandLineParserSettings)
    {
        if (!commandLineParserSettings.m_parseCommandLineEntryFunc)
        {
            return AZStd::unexpected(
                CommandLineParseErrorString("A Parse Command Line Entry function needs to be supplied to parse a command line argument"));
        }

        // Stores parse failure message for each command argument token
        CommandLineParseOutcome parseOutcome;

        // Stores the current state of the command line parsing after each argument
        CommandArgumentParserState argumentParserState;
        for (int i = 0; i < argc; ++i)
        {
            argumentParserState.m_finalToken = i == argc - 1;
            if (auto argumentParseOutcome = ParseArgument(argumentParserState, argv[i], commandLineParserSettings); !argumentParseOutcome)
            {
                // Aggregate failure messages together for the parseOutcome
                if (parseOutcome)
                {
                    // For the first error move the error messages directly into the parseOutcome
                    parseOutcome = AZStd::unexpected(AZStd::move(argumentParseOutcome.error()));
                }
                {
                    // Errors after the first one are appended to the failure string
                    parseOutcome.error() += AZStd::move(argumentParseOutcome.error());
                }
            }
        }

        return parseOutcome;
    }

    CommandLineParseOutcome ParseCommandLine(
        AZStd::span<AZStd::string_view const> commandLine, const CommandLineParserSettings& commandLineParserSettings)
    {
        if (!commandLineParserSettings.m_parseCommandLineEntryFunc)
        {
            return AZStd::unexpected(
                CommandLineParseErrorString("A Parse Command Line Entry function needs to be supplied to parse a command line argument"));
        }

        // Stores parse failure message for each command argument token
        CommandLineParseOutcome parseOutcome;

        // Stores the current state of the command line parsing after each argument
        CommandArgumentParserState argumentParserState;
        for (int i = 0; i < commandLine.size(); ++i)
        {
            argumentParserState.m_finalToken = i == commandLine.size() - 1;
            if (auto argumentParseOutcome = ParseArgument(argumentParserState, commandLine[i], commandLineParserSettings);
                !argumentParseOutcome)
            {
                // Aggregate failure messages together for the parseOutcome
                if (parseOutcome)
                {
                    // For the first error move the error messages directly into the parseOutcome
                    parseOutcome = AZStd::unexpected(AZStd::move(argumentParseOutcome.error()));
                }
                {
                    // Errors after the first one are appended to the failure string
                    parseOutcome.error() += AZStd::move(argumentParseOutcome.error());
                }
            }
        }

        return parseOutcome;
    }
} // namespace AZ::Settings
