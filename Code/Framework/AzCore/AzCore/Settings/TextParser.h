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
    //! Settings structure for parsing a text file.
    //! By default the user supplied ParseTextEntryFunc callback is invoked for every line parsed
    //! The TokenDelimiterFunc can be modified to change the token used to determine a text entry
    //! For example if the TokenDelimiterFunc returns true of '\0', then every time a NUL delimited blob is found
    //! the ParseTextEntryFunc will be invoked
    struct TextParserSettings
    {
        //! Function which is invoked for each "text entry" of the text file
        //! A text entry is determined as a block of text delimited by the result of the TokenDelimiterFunc
        //! By default a "text entry" is line based, but other delimiters can be used such as NUL('\0')
        using ParseTextEntryFunc = AZStd::function<bool(AZStd::string_view token)>;

        //! Callback function which is invoked for each text entry found in the text file
        //! This is only required member that needs to be set for the TextParserSettings
        ParseTextEntryFunc m_parseTextEntryFunc;

        //! Returns if the character matches the linefeed character
        //! @param element character being checked
        //! @return true if the character matches line feed
        static bool DefaultTokenDelimiterFunc(char element);
        //! Token Delimiter which determines a token from a text file
        //! The default token delimiter is a linefeed '\n'
        using TokenDelimiterFunc = AZStd::function<bool(char element)>;

        //! Callback function which is invoked to determine the end of the text entry
        TokenDelimiterFunc m_tokenDelimiterFunc = &DefaultTokenDelimiterFunc;

        //! If set to true, the ParseTextEntryFunc callback will be invoked even when
        //! the empty text is empty("line" size = 0)
        bool m_invokeParseTextEntryForEmptyLine = false;
    };

    // Expected structure which encapsulates the result of the ParseConfigFile function
    using TextParseErrorString = AZStd::basic_fixed_string<char, 512, AZStd::char_traits<char>>;
    using TextParseOutcome = AZStd::expected<void, TextParseErrorString>;

    //! Loads text file and invokes parse entry config callback
    //! This function will NOT allocate any dynamic memory.
    //! It is safe to call without any AZ Allocators
    //! NOTE: The max length for any one text entry is 4096
    //!
    //! @param stream GenericStream derived class where the configuration data will be read from
    //! @param textParserSettings structure which contains functions on how the to split "lines" of text file
    //!        as well as the callback function to invoke on each split "line"
    //! @return success outcome if the text file was parsed without error,
    //! otherwise a failure string is provided with the parse error
    TextParseOutcome ParseTextFile(AZ::IO::GenericStream& stream, const TextParserSettings& textParserSettings);
} // namespace AZ::Settings
