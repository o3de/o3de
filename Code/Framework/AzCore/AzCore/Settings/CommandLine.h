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

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/fixed_string.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>

namespace AZ
{
    /**
    * Given a commandline, this allows uniform parsing of it into parameter values and extra values.
    * When parsed, the commandline becomes a series of "switches" or "extra values"
    * For example, a switch may be specified as either 
    *      /switchName=value1[,value2...]
    *      /switchname value[,value2...]
    *      --switchname value[,value2...]
    *    or other combinations of the above
    * You may NOT use a colon as a switch separator since file names may easily contain them.
    * any additional parameters which are not associated with any switch are considered "misc values"
    */
    class CommandLine
    {
    public:
        AZ_CLASS_ALLOCATOR(CommandLine, AZ::SystemAllocator, 0);

        CommandLine();

        /**
         * Initializes a CommandLine instance which uses the provided commandLineOptionPreix for parsing switches
         */
        CommandLine(AZStd::string_view commandLineOptionPrefix);

        using ParamContainer = AZStd::vector<AZStd::string>;
        using ParamMap = AZStd::unordered_map<AZStd::string, ParamContainer>;
        /**
        * Construct a command line parser.
        * It will load parameters from the given ARGC/ARGV parameters instead of process command line.
        */
        void Parse(int argc, char** argv);
        void Parse(const ParamContainer& commandLine);

        /**
        * Will dump command line parameters from the CommandLine in a format such that switches
        * are prefixed with the option prefix followed by their value(s) which are comma separated
        * The result of this function can be supplied to Parse() to re-create an eqiuvlanet command line obect
        * Ex, If the command line has the current list of parsed miscellaneous values and switches of
        * MiscValue = ["Foo", "Bat"]
        * Switches = ["GameFolder" : [], "RemoteIp" : ["10.0.0.1"], "ScanFolders" : ["\a\b\c", "\d\e\f"]
        * CommandLineOptionPrefix = "-/"
        *
        * Then the resulting dumped value would be
        * Dump = ["", "Foo", "Bat", "-GameFolder", "-RemoteIp", "10.0.0.1", "-ScanFolders", "\a\b\c", "-ScanFolders", "\d\e\f"]
        * NOTE: The first parameter is always empty string as the Parse function skips over it
        */
        void Dump(ParamContainer& commandLineDumpOutput);

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
        * @param index The 0-based index to retrieve the switch value for
        * @return The value at that index.  This will Assert if you attempt to index out of bounds
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

        /*
        * Return the list of parsed switches
        */
        const ParamMap& GetSwitchList() const;

    private:
        void AddArgument(AZStd::string currentArg, AZStd::string& currentSwitch);

        ParamMap m_switches;
        ParamContainer m_miscValues;
        AZStd::string m_emptyValue;

        inline static constexpr size_t MaxCommandOptionPrefixes = 8;
        AZStd::fixed_string<MaxCommandOptionPrefixes> m_commandLineOptionPrefix;
    };
}
