/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/GenericStreams.h>
#include <AzCore/Settings/ConfigParser.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/string/fixed_string.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/StringFunc/StringFunc.h>

namespace AZ::Settings
{
    AZStd::string_view ConfigParserSettings::DefaultCommentPrefixFilter(AZStd::string_view line)
    {
        constexpr AZStd::string_view commentPrefixes = ";#";
        for (char commentPrefix : commentPrefixes)
        {
            if (size_t commentOffset = line.find(commentPrefix); commentOffset != AZStd::string_view::npos)
            {
                line = line.substr(0, commentOffset);
            }
        }

        return line;
    }

    AZStd::string_view ConfigParserSettings::DefaultSectionHeaderFilter(AZStd::string_view line)
    {
        AZStd::string_view sectionName;
        constexpr char sectionHeaderStart = '[';
        constexpr char sectionHeaderEnd = ']';
        if (line.starts_with(sectionHeaderStart) && line.ends_with(sectionHeaderEnd))
        {
            // View substring of line between the section '[' and ']' characters
            sectionName = line.substr(1);
            sectionName = sectionName.substr(0, sectionName.size() - 1);
            // strip any leading and trailing whitespace from the section name
            size_t startIndex = 0;
            for (; startIndex < sectionName.size() && isspace(sectionName[startIndex]); ++startIndex);
            sectionName = sectionName.substr(startIndex);
            size_t endIndex = sectionName.size();
            for (; endIndex > 0 && isspace(sectionName[endIndex - 1]); --endIndex);
            sectionName = sectionName.substr(0, endIndex);
        }

        return sectionName;
    }

    auto ConfigParserSettings::DefaultDelimiterFunc(AZStd::string_view line) -> ConfigKeyValuePair
    {
        constexpr AZStd::string_view CommandLineArgumentDelimiters{ "=:" };
        ConfigKeyValuePair keyValuePair;
        keyValuePair.m_value = line;

        // Splits the line on the first delimiter and stores that in the keyValuePair key variable
        // The StringFunc::TokenizeNext function updates the key parameter in place
        // to contain all the text after the first delimiter
        // So if keyValuePair.m_key="foo = Hello Ice Cream=World:17", the call to TokenizeNext would
        // split the value as follows
        // keyValuePair.m_key = "foo"
        // keyValuePair.m_value = "Hello Ice Cream=World:17"
        if (auto keyOpt = AZ::StringFunc::TokenizeNext(keyValuePair.m_value, CommandLineArgumentDelimiters);
            keyOpt.has_value())
        {
            // Strip the whitespace around the key part
            keyValuePair.m_key = AZ::StringFunc::StripEnds(*keyOpt);
        }

        // Strip white spaces around the value part
        keyValuePair.m_value = AZ::StringFunc::StripEnds(keyValuePair.m_value);
        return keyValuePair;
    };

    ParseOutcome ParseConfigFile(AZ::IO::GenericStream& configStream, const ConfigParserSettings& configParserSettings)
    {
        // The trace system is not being used here as this function is available
        // before any allocators or initialization of any systems take place

        ParseOutcome parseOutcome;
        if (!configParserSettings.m_parseConfigEntryFunc)
        {
            return AZStd::unexpected(ParseErrorString::format("The Parse Config Entry function is not valid for parsing a Windows Style INI line"));
        }

        // The ConfigFile is parsed into a fixed_vector of ConfigBufferMaxSize below
        // which avoids performing any dynamic memory allocations during parsing
        // Only the user supplied callback functions as part of the ConfigParserSettings can potentially allocate memory
        size_t remainingFileLength = configStream.GetLength();
        if (remainingFileLength == 0)
        {
            return AZStd::unexpected(ParseErrorString::format(R"(Config stream "%s" is empty, nothing to merge)" "\n", configStream.GetFilename()));
        }

        constexpr size_t ConfigBufferMaxSize = 4096;
        AZStd::fixed_vector<char, ConfigBufferMaxSize> configBuffer;
        size_t readSize = (AZStd::min)(configBuffer.max_size(), remainingFileLength);
        configBuffer.resize_no_construct(readSize);

        size_t bytesRead = configStream.Read(configBuffer.size(), configBuffer.data());
        remainingFileLength -= bytesRead;

        // Stores the current section header which needs to be stored in a fixed_string
        // since it is possible for the section header to have been read from a previous block
        AZStd::fixed_string<256> currentSectionHeader;
        do
        {
            decltype(configBuffer)::iterator frontIter{};
            for (frontIter = configBuffer.begin(); frontIter != configBuffer.end();)
            {
                if (AZStd::isspace(*frontIter, {}))
                {
                    ++frontIter;
                    continue;
                }

                // Find end of line
                auto lineStartIter = frontIter;
                auto lineEndIter = AZStd::find(frontIter, configBuffer.end(), '\n');
                bool foundNewLine = lineEndIter != configBuffer.end();
                if (!foundNewLine && remainingFileLength > 0)
                {
                    // No newline has been found in the remaining characters in the buffer,
                    // Read the next chunk(up to ConfigBufferMaxSize) of the config file and look for a new line
                    // if the file has remaining content
                    // Otherwise if the file has no more remaining content, then it is improperly terminated
                    // text file according to the posix standard
                    // Therefore the final text after the final newline will be parsed
                    break;
                }


                AZStd::string_view line(lineStartIter, lineEndIter);

                // Remove leading and surrounding spaces and carriage returns
                line = AZ::StringFunc::StripEnds(line, " \r");

                // Retrieve non-commented portion of line
                if (configParserSettings.m_commentPrefixFunc)
                {
                    line = configParserSettings.m_commentPrefixFunc(line);
                }
                // Lookup any new section names from the line
                if (configParserSettings.m_sectionHeaderFunc)
                {
                    AZStd::string_view sectionName = configParserSettings.m_sectionHeaderFunc(line);
                    if (!sectionName.empty())
                    {
                        currentSectionHeader = sectionName;
                        // If a section was found, then the line cannot contain a key, value
                        line = {};
                    }
                }

                // Create two views of a key and value from the line
                ConfigParserSettings::ConfigEntry configEntry;
                if (configParserSettings.m_delimiterFunc)
                {
                    configEntry.m_keyValuePair = configParserSettings.m_delimiterFunc(line);
                }

                // Assign the current section header
                configEntry.m_sectionHeader = currentSectionHeader;

                //! Invoke the parse config entry function to allow the caller to parse a single entry
                if (!configEntry.m_keyValuePair.m_key.empty() && !configParserSettings.m_parseConfigEntryFunc(configEntry))
                {
                    parseOutcome = AZStd::unexpected(ParseErrorString::format("The ParseConfigEntry callback returned false"
                        R"( indicating that the parsing of config entry (key="%.*s", value="%.*s" with section header "%.*s" has failed))",
                        AZ_STRING_ARG(configEntry.m_keyValuePair.m_key),
                        AZ_STRING_ARG(configEntry.m_keyValuePair.m_value),
                        AZ_STRING_ARG(configEntry.m_sectionHeader)));
                    // Do not break and continue to parse the config file
                }

                // Skip past the newline character if found
                frontIter = lineEndIter + (foundNewLine ? 1 : 0);
            }

            // Read in more data from the config file if available.
            // If the parsing was in the middle of a parsing line, then move the remaining data of that line to the beginning
            // of the fixed_vector buffer
            if (frontIter == configBuffer.begin())
            {
                parseOutcome = AZStd::unexpected(ParseErrorString::format(R"(The config stream "%s" contains a line which is longer than the max line length of %zu.)" "\n"
                    R"(Parsing will halt.)" "\n", configStream.GetFilename(), configBuffer.max_size()));
                break;
            }
            const size_t readOffset = AZStd::distance(frontIter, configBuffer.end());
            if (readOffset > 0)
            {
                memmove(configBuffer.begin(), frontIter, readOffset);
            }

            if (remainingFileLength == 0)
            {
                // There are no more bytes to read, no need to attempt to read from the stream
                break;
            }
            // Read up to minimum of remaining file length or 4K in the configBuffer
            readSize = (AZStd::min)(configBuffer.max_size() - readOffset, remainingFileLength);
            bytesRead = configStream.Read(readSize, configBuffer.data() + readOffset);
            // resize_no_construct fixes the size value
            configBuffer.resize_no_construct(readOffset + readSize);
            remainingFileLength -= bytesRead;
        } while (bytesRead > 0);

        return parseOutcome;
    }
} // namespace AZ::Settings
