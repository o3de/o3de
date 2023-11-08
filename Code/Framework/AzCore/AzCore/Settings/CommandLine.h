/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Settings/CommandLineParser.h>
#include <AzCore/std/containers/span.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/fixed_string.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>

namespace AZ
{
    /**
    * Given a commandline, this allows uniform parsing of it into parameter values and extra values.
    * When parsed, the commandline becomes a series of "options" or "extra values"
    * For example, a option may be specified as either
    *      /optionName=value1[,value2...]
    *      /optionname value[,value2...]
    *      --optionname value[,value2...]
    *    or other combinations of the above
    * You may NOT use a colon as a option separator since file names may easily contain them.
    * any additional parameters which are not associated with any option are considered "misc values"
    *
    * NOTE: "flags" options which doesn't specify a value need care when using
    * This is because the Parse() methods don't accept external metadata indicating actions to perform
    * on flag arguments unlike python argparse
    * https://docs.python.org/3/library/argparse.html#action
    * For example the command line of
    * `app.exe --verbose <PositionalArg>` would parse <PositionalArg> as the value of the "verbose" flag
    * The workaround is to either
    * 1. specify the flag options at the end of the command line
    * `app.exe <PositionalArg> --verbose`
    * 2. specify second option right after the previous flag option with and add a value to it
    * `app.exe --verbose --count 2 <PositionalArg>`
    * 3. specify a value for the 'verbose'option
    *     here only the 1 is parse as part of the verbose option, the <PositionalArg>
    *     separated by whitespace is a new argument
    * `app.exe --verbose 1 <PositionalArg>`  or `app.exe --verbose=1 <PositionalArg>`
    */
    class CommandLine
    {
    public:
        AZ_CLASS_ALLOCATOR(CommandLine, AZ::SystemAllocator);

        using ParamContainer = AZStd::vector<AZStd::string>;

        struct CommandArgument
        {
            AZStd::string m_option;
            AZStd::string m_value;
        };
        using ArgumentVector = AZStd::vector<CommandArgument>;

        CommandLine();

        /**
         * Initializes a CommandLine instance which uses the provided commandLineOptionPrefix for parsing switches
         */
        CommandLine(Settings::CommandLineOptionPrefixSpan optionPrefixes);
        /**
        * Construct a command line parser.
        * It will load parameters from the given ARGC/ARGV parameters instead of process command line.
        * Skips over the first parameter as it assumes it is the executable name
        */
        void Parse(int argc, char** argv);
        /**
         * Parses each element of the command line as parameter.
         * Unlike the ARGC/ARGV version above, this function doesn't skip over the first parameter
         * It allows for round trip conversion with the Dump() method
         */
        void Parse(AZStd::span<const AZStd::string_view> commandLine);
        void Parse(const ParamContainer& commandLine);

        /**
        * Will dump command line parameters from the CommandLine in a format such that switches
        * are prefixed with the option prefix followed by their value(s) which are comma separated
        * The result of this function can be supplied to Parse() to re-create an equivalent command line object
        * Ex, If the command line has the current list of parsed miscellaneous values and switches of
        * MiscValue = ["Foo", "Bat"]
        * Switches = ["GameFolder" : [], "RemoteIp" : ["10.0.0.1"], "ScanFolders" : ["\a\b\c", "\d\e\f"]
        * CommandLineOptionPrefix = "-/"
        *
        * Then the resulting dumped value would be
        * Dump = ["Foo", "Bat", "-GameFolder", "-RemoteIp", "10.0.0.1", "-ScanFolders", "\a\b\c", "-ScanFolders", "\d\e\f"]
        */
        void Dump(ParamContainer& commandLineDumpOutput) const;

        /**
        * Determines whether a switch is present in the command line
        */
        bool HasSwitch(AZStd::string_view switchName) const;

        /**
        * Get the number of values for the given switch.
        * @return 0 if the switch does not exist, otherwise the total number of values that appear after that switch
        */
        AZStd::size_t GetNumSwitchValues(AZStd::string_view switchName) const;

        /**
        * Get the actual value of a switch
        * @param switchName The switch to search for
        * @return The last value of the switch. This is follows the standard command line workflow
        * that the last one wins
        */
        const AZStd::string& GetSwitchValue(AZStd::string_view switchName) const;

        /**
        * Get the actual value of a switch
        * @param switchName The switch to search for
        * @param index The 0-based index to retrieve the switch value for
        * @return The value at that index. This will Assert if you attempt to index out of bounds
        */
        const AZStd::string& GetSwitchValue(AZStd::string_view switchName, AZStd::size_t index) const;

        /*
        * Get the number of misc values (values that are not associated with a switch)
        */
        AZStd::size_t GetNumMiscValues() const;

        /*
        * Given an index, return the actual value of the misc value at that index
        * This will assert if you attempt to index out of bounds.
        */
        const AZStd::string& GetMiscValue(AZStd::size_t index) const;


        // Range accessors
        [[nodiscard]] bool empty() const;
        auto size() const -> ArgumentVector::size_type;
        auto begin() -> ArgumentVector::iterator;
        auto begin() const -> ArgumentVector::const_iterator;
        auto cbegin() const -> ArgumentVector::const_iterator;
        auto end() -> ArgumentVector::iterator;
        auto end() const -> ArgumentVector::const_iterator;
        auto cend() const -> ArgumentVector::const_iterator;
        auto rbegin() -> ArgumentVector::reverse_iterator;
        auto rbegin() const -> ArgumentVector::const_reverse_iterator;
        auto crbegin() const -> ArgumentVector::const_reverse_iterator;
        auto rend() -> ArgumentVector::reverse_iterator;
        auto rend() const -> ArgumentVector::const_reverse_iterator;
        auto crend() const -> ArgumentVector::const_reverse_iterator;

    private:
        void ParseArgument(AZStd::string_view newOption, AZStd::string_view newValue);

        ArgumentVector m_allValues;

        // Stores the option prefixes which is used to configure the CommandLineParserSettings
        // to determine what kind of tokens are treated as command line options
        Settings::CommandLineOptionPrefixArray m_optionPrefixes;
    };
}
