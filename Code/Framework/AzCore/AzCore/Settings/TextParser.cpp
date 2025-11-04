/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/GenericStreams.h>
#include <AzCore/Settings/TextParser.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/string/fixed_string.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/StringFunc/StringFunc.h>

namespace AZ::Settings
{
    // Text Parsing support
    bool TextParserSettings::DefaultTokenDelimiterFunc(char element)
    {
        return element == '\n';
    }

    TextParseOutcome ParseTextFile(AZ::IO::GenericStream& textStream, const TextParserSettings& textParserSettings)
    {
        TextParseOutcome parseOutcome;
        if (!textParserSettings.m_parseTextEntryFunc)
        {
            return AZStd::unexpected(TextParseErrorString::format("The Parse Text Entry function is not valid for parsing a text file"));
        }

        // The TextFile is parsed into a fixed_vector of TextEntryBufferMaxSize below
        // which avoids performing any dynamic memory allocations during parsing
        // Only the user supplied callback functions as part of the TexctParserSettings can potentially allocate memory
        size_t remainingFileLength = textStream.GetLength();
        if (remainingFileLength == 0)
        {
            return AZStd::unexpected(TextParseErrorString::format(R"(text stream "%s" is empty, no text entries exist)" "\n", textStream.GetFilename()));
        }

        constexpr size_t TextEntryBufferMaxSize = 4096;
        AZStd::fixed_vector<char, TextEntryBufferMaxSize> textBuffer;
        size_t readSize = (AZStd::min)(textBuffer.max_size(), remainingFileLength);
        textBuffer.resize_no_construct(readSize);

        size_t bytesRead = textStream.Read(textBuffer.size(), textBuffer.data());
        remainingFileLength -= bytesRead;

        do
        {
            decltype(textBuffer)::iterator frontIter{};
            for (frontIter = textBuffer.begin(); frontIter != textBuffer.end();)
            {
                if (AZStd::isspace(*frontIter, {}))
                {
                    ++frontIter;
                    continue;
                }

                // Find end of text entry
                auto lineStartIter = frontIter;
                auto lineEndIter = AZStd::find_if(frontIter, textBuffer.end(), textParserSettings.m_tokenDelimiterFunc);
                bool foundTextEntry = lineEndIter != textBuffer.end();
                if (!foundTextEntry && remainingFileLength > 0)
                {
                    // No newline has been found in the remaining characters in the buffer,
                    // Read the next chunk(up to TextBufferMaxSize) of the text file and look for a new line
                    // if the file has remaining content
                    // Otherwise if the file has no more remaining content, then it is improperly terminated
                    // text file according to the posix standard
                    // Therefore the final text after the final newline will be parsed
                    break;
                }


                AZStd::string_view line(lineStartIter, lineEndIter);

                //! Invoke the parse text entry function to allow the caller to parse a single entry
                //! If m_invokeParseTextEntryForEmptyLine is false, empty entries are skipped
                if ((!line.empty() || textParserSettings.m_invokeParseTextEntryForEmptyLine)
                    && !textParserSettings.m_parseTextEntryFunc(line))
                {
                    parseOutcome = AZStd::unexpected(TextParseErrorString::format("The ParseTextEntry callback returned false"
                        R"( indicating that the parsing of text entry (value="%.*s") has failed)",
                        AZ_STRING_ARG(line)));
                    // Do not break and continue to parse the text file
                }

                // Skip past the text entry character if found
                frontIter = lineEndIter + (foundTextEntry ? 1 : 0);
            }

            // Read in more data from the text file if available.
            // If the parsing was in the middle of a parsing line, then move the remaining data of that line to the beginning
            // of the fixed_vector buffer
            if (frontIter == textBuffer.begin())
            {
                parseOutcome = AZStd::unexpected(TextParseErrorString::format(R"(The text stream "%s")"
                    "contains a line which is longer than the max line length of %zu.\n"
                    R"(Parsing will halt.)" "\n", textStream.GetFilename(), textBuffer.max_size()));
                break;
            }
            const size_t readOffset = AZStd::distance(frontIter, textBuffer.end());
            if (readOffset > 0)
            {
                memmove(textBuffer.begin(), frontIter, readOffset);
            }

            if (remainingFileLength == 0)
            {
                // There are no more bytes to read, no need to attempt to read from the stream
                break;
            }

            // Read up to minimum of remaining file length or 4K in the textBuffer
            readSize = (AZStd::min)(textBuffer.max_size() - readOffset, remainingFileLength);
            bytesRead = textStream.Read(readSize, textBuffer.data() + readOffset);
            // resize_no_construct fixes the size value
            textBuffer.resize_no_construct(readOffset + readSize);
            remainingFileLength -= bytesRead;
        } while (bytesRead > 0);

        return parseOutcome;
    }
} // namespace AZ::Settings
