/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/functional.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/std/utility/expected.h>

namespace AZStd
{
    // Forward declare basic_fixed_string to avoid including fixed_string.h
    // in the header
    template <class Element, size_t MaxElementCount, class Traits>
    class basic_fixed_string;
}

namespace AZ::IO
{
    class GenericStream;
}

namespace AZ::Settings
{
    //! Settings structure which is used to determine how to parse Windows INI style config file(.cfg, .ini, etc...)
    //! It supports being able to supply a custom comment filter and section header filter
    //! The names of section headers are appended to the root Json pointer path member to form new root paths
    //! Ex. test.ini
    /*
    ; This is a comment
    # This is also a comment
    key1 = value1 ; inline comment
    [Section Header1]
    sectionKey1 = value2
    */
    struct ConfigParserSettings
    {
        //! Struct which contains a string view to the key part and the value part
        //! of a parsed INI file line.
        struct ConfigKeyValuePair
        {
            AZStd::string_view m_key;
            AZStd::string_view m_value;
        };

        //! Complete struct supplied to the ParseConfigEntry callback function
        //! representing a single config entry. It will always contains
        //! a key and value pair.
        //! The section header will be supplied if the key, value is underneath a section
        struct ConfigEntry
        {
            ConfigKeyValuePair m_keyValuePair;
            AZStd::string_view m_sectionHeader;
        };

        //! Callback invoked with each parsed key, value `(key=value)` entry from a Windows Style INI file
        //! with the section header if available `[section header]`
        //! This is the the only member that is required to be set within this struct
        //! All other members are defaulted to work with INI style files
        //! IMPORTANT: Any lambda functions or class instances that are larger than 16 bytes
        //! in size requires a memory allocation to store.
        //! So it is recommended that users binding a lambda bind at most 2 reference or pointer members
        //! to avoid dynamic heap allocations
        //!
        //! NOTE: This function will not be called if the key is empty
        //! @return True should be returned from this function to indicate that parsing of the config entry
        //! has succeeded
        using ParseConfigEntryFunc = AZStd::function<bool(const ConfigEntry&)>;

        ParseConfigEntryFunc m_parseConfigEntryFunc;

        //! Filters out line which start with a prefix of ';' or '#'
        //! If a line matches the comment filter and empty view is returned
        //! Otherwise the line is returned as is
        //! @param line line to parse for comments
        //! @return view of line without any comments
        static AZStd::string_view DefaultCommentPrefixFilter(AZStd::string_view line);

        //!  Matches a section header in the regular expression form of
        //! \[(?P<header>[^]]+)\] # '[' followed by 1 or more non-']' characters followed by a ']'
        //! If the line matches section header format, the section name portion of that line
        //! is returned in the output parameter otherwise an empty view is returned
        //! @param line to parse for section header
        //! @return section name header if the line starts contains an INI style section header
        static AZStd::string_view DefaultSectionHeaderFilter(AZStd::string_view line);

        //! Splits a line into two views of a key and a value pair
        //! Surrounding whitespace around the key and value are not part of the string views
        //! This function is invoked after the section header filter is invoked
        //! @param line to search for delimiter and split int key value params
        //! @return key, value pair structure representing the setting key and value
        static ConfigKeyValuePair DefaultDelimiterFunc(AZStd::string_view line);

        //! Callback function that is invoked when a line is read.
        //! returns a substr of the line which contains the non-commented portion of the line
        using CommentPrefixFunc = AZStd::function<AZStd::string_view(AZStd::string_view line)>;

        //! Callback function that is after a line has been filtered through the CommentPrefixFunc
        //! to determine if the text matches a section header
        //! NOTE: Leading and trailing whitespace will be removed from the line before invoking the callback
        //!
        //! @return returns a view of the section name if the line contains a section
        //! Otherwise an empty view is returned
        using SectionHeaderFunc = AZStd::function<AZStd::string_view(AZStd::string_view line)>;

        //! Callback function which is invoked after a line has been filtered through the SectionHeaderFunc
        //! to split the key,value pair of a line
        //! NOTE: Leading and trailing whitespace will be removed from the line before invoking the callback
        //!
        //! @return an instance of a ConfigKeyValuePair with the split key and value with surrounding whitespace
        //! trimmed
        using DelimiterFunc = AZStd::function<ConfigKeyValuePair(AZStd::string_view line)>;


        //! Function which is invoked to retrieve the non-commented section of a line
        //! The input line will never start with leading whitespace
        CommentPrefixFunc m_commentPrefixFunc = &DefaultCommentPrefixFilter;

        //! Invoked on the non-commented section of a line that has been read from the config file
        //! This function should examine the supplied line to determine if it matches a section header
        //! If so the section name should be returned
        //! Any sections names returned will be supplied as a section header to the Parse callback
        SectionHeaderFunc m_sectionHeaderFunc = &DefaultSectionHeaderFilter;

        //! Function which is invoked on the config line after filtering through the SectionHeaderFunc
        //! to determine the delimiter of the line
        //! The structure contains a functor which returns a ConfigKeyValuePair to split the key value pair
        DelimiterFunc m_delimiterFunc = &DefaultDelimiterFunc;
    };

    // Expected structure which encapsulates the result of the ParseConfigFile function
    using ParseErrorString = AZStd::basic_fixed_string<char, 512, AZStd::char_traits<char>>;
    using ParseOutcome = AZStd::expected<void, ParseErrorString>;

    //! Loads basic configuration files which have structures that model Windows INI files
    //! It is inspired by the Python configparser module: https://docs.python.org/3.10/library/configparser.html
    //! NOTE: There is a max line length for any one configuration entry of 4096
    //!       If a line greater than that is found parsing of the configuration file fails
    //! This function will NOT allocate any dynamic memory.
    //! It is safe to call without any AZ Allocators
    //! @param stream GenericStream derived class where the configuration data will be read from
    //! @param configParseSettings struct defining configuration on how the INI style file should be parsed
    //! @return success outcome if the configuration file was parsed without error,
    //! otherwise a failure string is provided with the parse error
    ParseOutcome ParseConfigFile(AZ::IO::GenericStream& stream, const ConfigParserSettings& configParserSettings);
} // namespace AZ::Settings
