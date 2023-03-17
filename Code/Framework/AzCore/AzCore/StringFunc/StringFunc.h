/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/IO/Path/Path_fwd.h>
#include <AzCore/std/function/function_fwd.h>
#include <AzCore/std/ranges/common_view.h>
#include <AzCore/std/ranges/join_with_view.h>
#include <AzCore/std/ranges/transform_view.h>
#include <AzCore/std/ranges/ranges_algorithm.h>
#include <AzCore/std/ranges/subrange.h>
#include <AzCore/std/string/fixed_string.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/optional.h>
#include <AzCore/Casting/numeric_cast.h>

//---------------------------------------------------------------------
// FILENAME CONSTRAINTS
//---------------------------------------------------------------------

#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
#   define AZ_CORRECT_FILESYSTEM_SEPARATOR '\\'
#   define AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING "\\"
#   define AZ_DOUBLE_CORRECT_FILESYSTEM_SEPARATOR "\\\\"
#   define AZ_WRONG_FILESYSTEM_SEPARATOR '/'
#   define AZ_WRONG_FILESYSTEM_SEPARATOR_STRING "/"
#   define AZ_NETWORK_PATH_START "\\\\"
#   define AZ_NETWORK_PATH_START_SIZE 2
#   define AZ_CORRECT_AND_WRONG_FILESYSTEM_SEPARATOR "/\\"
#else
#   define AZ_CORRECT_FILESYSTEM_SEPARATOR '/'
#   define AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING "/"
#   define AZ_DOUBLE_CORRECT_FILESYSTEM_SEPARATOR "//"
#   define AZ_WRONG_FILESYSTEM_SEPARATOR '\\'
#   define AZ_WRONG_FILESYSTEM_SEPARATOR_STRING "\\"
#   define AZ_NETWORK_PATH_START "\\\\"
#   define AZ_NETWORK_PATH_START_SIZE 2
#   define AZ_CORRECT_AND_WRONG_FILESYSTEM_SEPARATOR "/\\"
#endif
#define AZ_FILESYSTEM_EXTENSION_SEPARATOR '.'
#define AZ_FILESYSTEM_WILDCARD '*'
#define AZ_FILESYSTEM_DRIVE_SEPARATOR ':'
#define AZ_FILESYSTEM_INVALID_CHARACTERS "><|\"?*\r\n"

#define AZ_FILENAME_ALLOW_SPACES
#define AZ_SPACE_CHARACTERS " \t"
#define AZ_MATCH_ALL_FILES_WILDCARD_PATTERN  "*"
#define AZ_FILESYSTEM_SEPARATOR_WILDCARD AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING AZ_MATCH_ALL_FILES_WILDCARD_PATTERN

//These do not change per platform they relate to how we represent paths in our database
//which is not dependent on platform file system
#define AZ_CORRECT_DATABASE_SEPARATOR '/'
#define AZ_CORRECT_DATABASE_SEPARATOR_STRING "/"
#define AZ_DOUBLE_CORRECT_DATABASE_SEPARATOR "//"
#define AZ_WRONG_DATABASE_SEPARATOR '\\'
#define AZ_WRONG_DATABASE_SEPARATOR_STRING "\\"
#define AZ_CORRECT_AND_WRONG_DATABASE_SEPARATOR "/\\"
#define AZ_DATABASE_EXTENSION_SEPARATOR '.'
#define AZ_DATABASE_INVALID_CHARACTERS "><|\"?*\r\n"
//////////////////////////////////////////////////////////////////////////

//! Namespace for string functions.
/*!
The StringFunc namespace is where we put string functions that extend some std::string functionality
to regular c-strings, make easier implementations of common but complex AZStd::strings routines,
implement features that are not in std (or not yet in std), and a host of functions to more easily
manipulate file paths. Some simply do additional error checking on top of the std, or make it more
readable.
*/
namespace AZ
{
    // Forwards
    class Vector2;
    class Vector3;
    class Vector4;

    namespace StringFunc
    {
        /*! Checks if the string begins with the given prefix, if prefixValue is a prefix of searchValue, this function will return true
        * \param[in] searchValue The input string to examine for a given prefix
        * \param[in] prefixValue The prefix value to examine searchValue for
        * \param[in] bCaseSensitive If true, comparison will be case sensitive
        */
        bool StartsWith(AZStd::string_view searchValue, AZStd::string_view prefixValue, bool bCaseSensitive = false);

        /*! Checks if the string ends with the given suffix, if suffixValue is a suffix of searchValue, this function will return true
        * \param[in] searchValue The input string to examine for a given suffix
        * \param[in] suffixValue The suffix value to examine searchValue for
        * \param[in] bCaseSensitive If true, comparison will be case sensitive
        */
        bool EndsWith(AZStd::string_view searchValue, AZStd::string_view suffixValue, bool bCaseSensitive = false);

        //! Equal
        /*! Equality for non AZStd::strings.
        Ease of use to compare c-strings with case sensitivity.
        Example: Case Insensitive compare of a c-string with "Hello"
        StringFunc::Equal("hello", "Hello") = true
        Example: Case Sensitive compare of a c-string with "Hello"
        StringFunc::Equal("hello", "Hello", true) = false
        Example: Case Sensitive compare of the first 3 characters of a c-string with "Hello"
        StringFunc::Equal("Hello World", "Hello", true, 3) = true
        */
        bool Equal(const char* inA, const char* inB, bool bCaseSensitive = false, size_t n = 0);
        bool Equal(AZStd::string_view inA, AZStd::string_view inB, bool bCaseSensitive = false);

        //! Contains
        /*! Checks if the supplied character or string is contained within the @in parameter
        *
        Example: Case Insensitive contains finds character
        StringFunc::Contains("Hello", 'L') == true
        Example: Case Sensitive contains finds character
        StringFunc::Contains("Hello", 'l', true) = true
        Example: Case Insensitive contains does not find string
        StringFunc::Contains("Well Hello", "Mello") == false
        Example: Case Sensitive contains does not find character
        StringFunc::Contains("HeLlo", 'h', true) == false
        */
        bool Contains(AZStd::string_view in, char ch, bool bCaseSensitive = false);
        bool Contains(AZStd::string_view in, AZStd::string_view sv, bool bCaseSensitive = false);

        //! Find
        /*! Find for non AZStd::strings. Ease of use to find the first or last occurrence of a character or substring in a c-string with case sensitivity.
        Example: Case Insensitive find first occurrence of a character a in a c-string
        StringFunc::Find("Hello", 'l') == 2
        Example: Case Insensitive find last occurrence of a character a in a c-string
        StringFunc::Find("Hello", 'l') == 3
        Example: Case Sensitive find last occurrence of a character a in a c-string
        StringFunc::Find("HeLlo", 'L', true, true) == 2
        Example: Case Sensitive find first occurrence of substring "Hello" a in a c-string
        StringFunc::Find("Well Hello", "Hello", false, true) == 5
        */
        size_t Find(AZStd::string_view in, char c, size_t pos = 0, bool bReverse = false, bool bCaseSensitive = false);
        size_t Find(AZStd::string_view in, AZStd::string_view str, size_t pos = 0, bool bReverse = false, bool bCaseSensitive = false);

        //! First and Last Character
        /*! First and Last Characters in a c-string. Ease of use to get the first or last character
         *! of a c-string.
         Example: If the first character in a c-string is a 'd' do something
         StringFunc::FirstCharacter("Hello") == 'H'
         Example: If the last character in a c-string is a AZ_CORRECT_FILESYSTEM_SEPARATOR do something
         StringFunc::LastCharacter("Hello") == 'o'
         */
        char FirstCharacter(const char* in);
        char LastCharacter(const char* in);

        //! Append and Prepend
        /*! Append and Prepend to a AZStd::string. Increase readability and error checking.
         *! While appending has an ease of use operator +=, prepending currently does not.
         Example: Append character AZ_CORRECT_FILESYSTEM_SEPARATOR to a AZStd::string
         StringFunc::Append(s = "C:", AZ_CORRECT_FILESYSTEM_SEPARATOR); s == "C:\\"
         Example: Append string " World" to a AZStd::string
         StringFunc::Append(s = "Hello", " World");  s == "Hello World"
         Example: Prepend string AZ_NETWORK_PATH_START to a AZStd::string
         StringFunc::Prepend(s = "18machine", AZ_NETWORK_PATH_START); s = "\\\\18machine"
         */
        AZStd::string& Append(AZStd::string& inout, const char s);
        AZStd::string& Append(AZStd::string& inout, const char* str);
        AZStd::string& Prepend(AZStd::string& inout, const char s);
        AZStd::string& Prepend(AZStd::string& inout, const char* str);

        //! LChop and RChop a AZStd::string
        /*! Increase readability and error checking. RChop removes n characters from the right
         *! side (the end) of a AZStd::string. LChop removes n characters from the left side (the
         *! beginning) of a string.
         Example: Remove (Chop off) the last 3 characters from a AZStd::string
         StringFunc::RChop(s = "Hello", 3); s == "He"
         Example: Remove (Chop off) the first 3 characters from a AZStd::string
         StringFunc::LChop(s = "Hello", 3); s == "lo"
         */
        AZStd::string& LChop(AZStd::string& inout, size_t num = 1);
        AZStd::string& RChop(AZStd::string& inout, size_t num = 1);

        AZStd::string_view LChop(AZStd::string_view in, size_t num = 1);
        AZStd::string_view RChop(AZStd::string_view in, size_t num = 1);

        //! LKeep and RKeep a AZStd::string
        /*! Increase readability and error checking. RKeep keeps the string to the right of a position
         *! in a AZStd::string. LKeep keep the string to the left of a position in a AZStd::string.
         *! Optionally keep the character at position in the string.
         Example: Keep the string to the right of the 3rd character
         StringFunc::RKeep(s = "Hello World", 2); s == "o World"
         Example: Keep the string to the left of the 3rd character
         StringFunc::LKeep(s = "Hello", 3); s == "Hel"
         */
        AZStd::string& LKeep(AZStd::string& inout, size_t pos, bool bKeepPosCharacter = false);
        AZStd::string& RKeep(AZStd::string& inout, size_t pos, bool bKeepPosCharacter = false);

        //! Replace
        /*! Replace the first, last or all character(s) or substring(s) in a AZStd::string with
         *! case sensitivity.
         Example: Case Insensitive Replace all 'o' characters with 'a' character
         StringFunc::Replace(s = "Hello World", 'o', 'a'); s == "Hella Warld"
         Example: Case Sensitive Replace all 'o' characters with 'a' character
         StringFunc::Replace(s = "HellO World", 'O', 'o'); s == "Hello World"
         */
        bool Replace(AZStd::string& inout, const char replaceA, const char withB, bool bCaseSensitive = false, bool bReplaceFirst = false, bool bReplaceLast = false);
        bool Replace(AZStd::string& inout, const char* replaceA, const char* withB, bool bCaseSensitive = false, bool bReplaceFirst = false, bool bReplaceLast = false);

        /**
        * Simple string trimming (trims spaces, tabs, and line-feed/cr characters from both ends)
        *
        * \param[in, out] value     The value to trim
        * \param[in] leading        Flag to trim the leading whitespace characters from the string
        * \param[in] trailing       Flag to trim the trailing whitespace characters from the string
        * \returns The reference to the trimmed value
        */
        AZStd::string& TrimWhiteSpace(AZStd::string& value, bool leading, bool trailing);

        //! LStrip
        /*! Strips leading characters in the stripCharacters set
        * Example
        * Example: Case Insensitive Strip leading 'a' characters
        * StringFunc::LStrip(s = "Abracadabra", 'a'); s == "bracadabra"
        * Example: Case Sensitive Strip leading 'a' characters
        * StringFunc::LStrip(s = "Abracadabra", 'a'); s == "Abracadabra"
        */
        //! RStrip
        /*! Strips trailing characters in the stripCharacters set
        * Example
        * Example: Case Insensitive Strip trailing 'a' characters
        * StringFunc::RStrip(s = "AbracadabrA", 'a'); s == "Abracadabr"
        * Example: Case Sensitive Strip trailing 'a' characters
        * StringFunc::RStrip(s = "AbracadabrA", 'a'); s == "AbracadabrA"
        */
        //! StripEnds
        /*! Strips leading and trailing characters in the stripCharacters set
        Example: Case Insensitive Strip all 'a' characters
        StringFunc::StripEnds(s = "Abracadabra", 'a'); s == "bracadabr"
        Example: Case Sensitive Strip all 'a' characters
        StringFunc::StripEnds(s = "Abracadabra", 'a'); s == "Abracadabr"
        */
        AZStd::string_view LStrip(AZStd::string_view in, AZStd::string_view stripCharacters = " ");
        AZStd::string_view RStrip(AZStd::string_view in, AZStd::string_view stripCharacters = " ");
        AZStd::string_view StripEnds(AZStd::string_view in, AZStd::string_view stripCharacters = " ");

        //! Strip
        /*! Strip away the leading, trailing or all character(s) or substring(s) in a AZStd::string with
        *! case sensitivity.
        Example: Case Insensitive Strip all 'a' characters
        StringFunc::Strip(s = "Abracadabra", 'a'); s == "brcdbr"
        Example: Case Insensitive Strip first 'b' character (No Match)
        StringFunc::Strip(s = "Abracadabra", 'b', true, false); s == "Abracadabra"
        Example: Case Sensitive Strip first 'a' character (No Match)
        StringFunc::Strip(s = "Abracadabra", 'a', true, false); s == "Abracadabra"
        Example: Case Insensitive Strip last 'a' character
        StringFunc::Strip(s = "Abracadabra", 'a', false, false, true); s == "Abracadabr"
        Example: Case Insensitive Strip first and last 'a' character
        StringFunc::Strip(s = "Abracadabra", 'a', false, true, true); s == "bracadabr"
        Example: Case Sensitive Strip first and last 'l' character (No Match)
        StringFunc::Strip(s = "HeLlo HeLlo HELlO", 'l', true, true, true); s == "HeLlo HeLlo HELlO"
        Example: Case Insensitive Strip first and last "hello" character
        StringFunc::Strip(s = "HeLlo HeLlo HELlO", "hello", false, true, true); s == " HeLlo "
        */
        bool Strip(AZStd::string& inout, const char stripCharacter = ' ', bool bCaseSensitive = false, bool bStripBeginning = false, bool bStripEnding = false);
        bool Strip(AZStd::string& inout, const char* stripCharacters = " ", bool bCaseSensitive = false, bool bStripBeginning = false, bool bStripEnding = false);

        //! Tokenize
        /*! Tokenize a c-string, into a vector of AZStd::string(s) optionally keeping empty string
         *! and optionally keeping space only strings
         Example: Tokenize the words of a sentence.
         StringFunc::Tokenize("Hello World", d, ' '); s[0] == "Hello", s[1] == "World"
         Example: Tokenize a comma and end line delimited string
         StringFunc::Tokenize("Hello,World\nHello,World", d, ' '); s[0] == "Hello", s[1] == "World"
         s[2] == "Hello", s[3] == "World"
         */
        void Tokenize(AZStd::string_view in, AZStd::vector<AZStd::string>& tokens, const char delimiter, bool keepEmptyStrings = false, bool keepSpaceStrings = false);
        void Tokenize(AZStd::string_view in, AZStd::vector<AZStd::string>& tokens, AZStd::string_view delimiters = "\\//, \t\n", bool keepEmptyStrings = false, bool keepSpaceStrings = false);
        void Tokenize(AZStd::string_view in, AZStd::vector<AZStd::string>& tokens, const AZStd::vector<AZStd::string_view>& delimiters, bool keepEmptyStrings = false, bool keepSpaceStrings = false);

        //! TokenizeVisitor
        /*! Tokenize a string_view and invoke a handler for each token found.
         *! The keep empty string option will invoke the handler with empty strings
         *! The keep space string option will invoke the handler with space strings
         Example: Tokenize the words of a sentence.
         StringFunc::TokenizeVisitor("Hello World", visitor, ' '); Invokes visitor("Hello") and visitor("World")
         Example: Tokenize a comma and newline delimited string
         StringFunc::TokenizeVisitor("Hello,World\nHello,World", visitor, ',\n ');
         Invokes visitor("Hello"), visitor("World"), visitor("Hello"), visitor("World")
         Example: Tokenize a space delimited string while keeping empty strings and space strings
         StringFunc::TokenizeVisitor("Hello World  More   Tokens", visitor, ' ', true, true); Invokes visitor("Hello"), visitor("World"), visitor(""),
            visitor("More"), visitor(""), visitor(""), "visitor("Tokens")
         */
        //! TokenVisitor
        /*! Callback which is invoked each time a token is found within the string
        */
        using TokenVisitor = AZStd::function<void(AZStd::string_view)>;
        void TokenizeVisitor(AZStd::string_view in, const TokenVisitor& tokenVisitor, const char delimiter, bool keepEmptyStrings = false, bool keepSpaceStrings = false);
        void TokenizeVisitor(AZStd::string_view in, const TokenVisitor& tokenVisitor, AZStd::string_view delimiters = AZ_CORRECT_AND_WRONG_DATABASE_SEPARATOR ",\t\n",
            bool keepEmptyStrings = false, bool keepSpaceStrings = false);

        //! TokenizeVisitorReverse
        /*! Same as TokenizeVisitor, except that parsing of tokens start from the end of the view
         *! The keep empty string option will invoke the handler with empty strings
         *! The keep space string option will invoke the handler with space strings
        Example: Tokenize the words of a sentence.
        StringFunc::TokenizeVisitorReverse("Hello World", visitor, ' '); Invokes visitor("World") and visitor("Hello")
        Example: Tokenize a comma and newline delimited string
        StringFunc::TokenizeVisitorReverse("Hello,World\nHello,World", visitor, ',\n ');
        Invokes visitor("World"), visitor("Hello"), visitor("World"), visitor("Hello")
        Example: Tokenize a space delimited string while keeping empty strings and space strings
        StringFunc::TokenizeVisitorReverse("Hello World  More   Tokens", visitor, ' ', true, true); Invokes visitor("Tokens"), visitor(""), visitor(""),
            visitor("More"), visitor(""), visitor("World"), "visitor("Hello")
        */
        void TokenizeVisitorReverse(AZStd::string_view in, const TokenVisitor& tokenVisitor, const char delimiter, bool keepEmptyStrings = false, bool keepSpaceStrings = false);
        void TokenizeVisitorReverse(AZStd::string_view in, const TokenVisitor& tokenVisitor, AZStd::string_view delimiters = AZ_CORRECT_AND_WRONG_DATABASE_SEPARATOR ",\t\n",
            bool keepEmptyStrings = false, bool keepSpaceStrings = false);

        //! TokenizeFirst
        /*! Returns an optional to a string_view of the characters that is before the first delimiter found in the set of delimiters.
         *! The optional is valid unless the view is empty, A valid optional indicates that at least one character of the @inout parameter
         *! has been processed and removed from it. This allows TokenizeFirst to be invoked in a loop
         Example: Tokenize the words of a sentence.
         AZStd::string_view input = "Hello World";
         AZStd::optional<AZStd::string_view> output;
         output = StringFunc::TokenizeNext(input, " "); input = "World", output(valid) = "Hello"
         output = StringFunc::TokenizeNext(input, " "); input = "", output(valid) = "World"
         output = StringFunc::TokenizeNext(input, " "); input = "", output(invalid)
         Example: Tokenize a  delimited string with multiple whitespace
         AZStd::string_view input = "Hello World  More   Tokens";
         AZStd::optional<AZStd::string_view> output;
         output = StringFunc::TokenizeNext(input, ' '); input = "World  More   Tokens", output(valid) = "Hello"
         output = StringFunc::TokenizeNext(input, ' '); input = " More   Tokens", output(valid) = "World"
         output = StringFunc::TokenizeNext(input, ' '); input = "More   Tokens", output(valid) = ""
         output = StringFunc::TokenizeNext(input, ' '); input = "  Tokens", output(valid) = "More"
         output = StringFunc::TokenizeNext(input, ' '); input = " Tokens", output(valid) = ""
         output = StringFunc::TokenizeNext(input, ' '); input = "Tokens", output(valid) = ""
         output = StringFunc::TokenizeNext(input, ' '); input = "", output(valid) = "Tokens"
         output = StringFunc::TokenizeNext(input, ' '); input = "", output(invalid)
         */
        AZStd::optional<AZStd::string_view> TokenizeNext(AZStd::string_view& inout, const char delimiter);
        AZStd::optional<AZStd::string_view> TokenizeNext(AZStd::string_view& inout, AZStd::string_view delimiters);

        //! TokenizeLast
        /*! Returns an optional to a string_view of the characters that is after the last delimiter found in the set of delimiters.
         *! The optional is valid unless the view is empty, A valid optional indicates that at least one character of the @inout parameter
         *! has been processed and removed from it.

         Example: Tokenize the words of a sentence.
         AZStd::string_view input = "Hello World";
         AZStd::optional<AZStd::string_view> output;
         output = StringFunc::TokenizeLast(input, " "); input = "Hello", output(valid) = "World"
         output = StringFunc::TokenizeLast(input, " "); input = "", output(valid) = "Hello"
         output = StringFunc::TokenizeLast(input, " "); input = "", output(invalid)
         Example: Tokenize a  delimited string with multiple whitespace
         AZStd::string_view input = "Hello World  More   Tokens";
         AZStd::optional<AZStd::string_view> output;
         output = StringFunc::TokenizeLast(input, ' '); input = "Hello World  More  ", output(valid) = "Tokens"
         output = StringFunc::TokenizeLast(input, ' '); input = "Hello World  More ", output(valid) = ""
         output = StringFunc::TokenizeLast(input, ' '); input = "Hello World  More", output(valid) = ""
         output = StringFunc::TokenizeLast(input, ' '); input = "Hello World ", output(valid) = "More"
         output = StringFunc::TokenizeLast(input, ' '); input = "Hello World", output(valid) = ""
         output = StringFunc::TokenizeLast(input, ' '); input = "Hello", output(valid) = "World"
         output = StringFunc::TokenizeLast(input, ' '); input = "", output(valid) = "Hello"
         output = StringFunc::TokenizeLast(input, ' '); input = "", output(invalid)
         */
        AZStd::optional<AZStd::string_view> TokenizeLast(AZStd::string_view& inout, const char delimiter);
        AZStd::optional<AZStd::string_view> TokenizeLast(AZStd::string_view& inout, AZStd::string_view delimiters);

        //recognition and conversion from text

        //! Recognition of basic types
        /*! Provides for recognition and conversion of basic types (int float and bool) from c-string
        Example: Recognize a integer from a c-string
        StringFunc::LooksLikeInt("123") == true
        StringFunc::ToInt("123") == 123
        Example: Recognize a bool from a c-string
        StringFunc::LooksLikeBool("true") == true
        StringFunc::ToBool("true") == true
        Example: Recognize a float from a c-string
        StringFunc::LooksLikeFloat("1.23") == true
        StringFunc::ToFloat("1.23") == 1.23f
        */
        int ToInt(const char* in);
        bool LooksLikeInt(const char* in, int* pInt = nullptr);
        double ToDouble(const char* in);
        bool LooksLikeDouble(const char* in, double* pDouble = nullptr);
        float ToFloat(const char* in);
        bool LooksLikeFloat(const char* in, float* pFloat = nullptr);
        bool ToBool(const char* in);
        bool LooksLikeBool(const char* in, bool* pBool = nullptr);

        bool LooksLikeVector2(const char* in, AZ::Vector2* outVector = nullptr);
        AZ::Vector2 ToVector2(const char* in);

        bool LooksLikeVector3(const char* in, AZ::Vector3* outVector = nullptr);
        AZ::Vector3 ToVector3(const char* in);

        bool LooksLikeVector4(const char* in, AZ::Vector4* outVector = nullptr);
        AZ::Vector4 ToVector4(const char* in);

        //! ToHexDump and FromHexDump
        /*! Convert a c-string to and from hex
        Example:convert a c-string to hex
        StringFunc::ToHexDump("abcCcdeEfFF",a); a=="6162634363646545664646"
        StringFunc::FromHexDump("6162634363646545664646", a); a=="abcCcdeEfFF"
        */
        bool ToHexDump(const char* in, AZStd::string& out);
        bool FromHexDump(const char* in, AZStd::string& out);


        //! Join
        /*! Joins a collection of strings that are convertible to string view. Applies a separator string in between elements.
        Note:
            The joinTarget variable will be appended to, if you need it cleared first
            you must reset it yourself before calling join
        Example: Join a list of the strings "test", "string" and "joining"
            AZStd::list<AZStd::string> example;
            // add three strings: "test", "string" and "joining"
            AZStd::string output;
            Join(output, example.begin(), example.end(), " -- ");
            // output == "test -- string -- joining"
            AZStd::fixed_string<128> fixedOutput;
            Join(fixedOutput, example.begin(), example.end(), ",");
            // fixedOutput == "test,string,joining"
        */
        template<typename StringType, typename ConvertableToStringViewIterator, typename SeparatorString>
        inline void Join(
            StringType& joinTarget,
            ConvertableToStringViewIterator iteratorBegin,
            ConvertableToStringViewIterator iteratorEnd,
            const SeparatorString& separator)
        {
            using CharType = typename StringType::value_type;
            using StringViewType = AZStd::basic_string_view<CharType>;
            StringViewType separatorView;
            if constexpr (AZStd::convertible_to<SeparatorString, CharType>)
            {
                separatorView = StringViewType{ &separator, 1 };
            }
            else
            {
                separatorView = separator;
            }

            for (bool prependSeparator{}; iteratorBegin != iteratorEnd; ++iteratorBegin)
            {
                if (prependSeparator)
                {
                    joinTarget += separatorView;
                }
                joinTarget += StringViewType(*iteratorBegin);
                prependSeparator = true;
            }
        }

        template<typename StringType, typename Range, typename SeparatorString,
            class = AZStd::enable_if_t<AZStd::ranges::input_range<Range> &&
            AZStd::convertible_to<AZStd::ranges::range_value_t<Range>,
                AZStd::basic_string_view<typename StringType::value_type, typename StringType::traits_type>>
        >>
        void Join(StringType& joinTarget, Range&& stringViewConvertibleRange, const SeparatorString& separator)
        {
            Join(joinTarget,
                AZStd::ranges::begin(stringViewConvertibleRange),
                AZStd::ranges::end(stringViewConvertibleRange),
                separator);
        }

        //////////////////////////////////////////////////////////////////////////
        //! StringFunc::NumberFormatting Namespace
        /*! For string functions supporting string representations of numbers
        */
        namespace NumberFormatting
        {
            //! GroupDigits
            /*! Modifies the string representation of a number to add group separators, typically commas in thousands, e.g. 123456789.00 becomes 123,456,789.00
            *
            * \param buffer - The buffer containing the number which is to be modified in place
            * \param bufferSize - The length of the buffer in bytes
            * \param decimalPosHint - Optional position where the decimal point (or end of the number if there is no decimal) is located, will improve performance if supplied
            * \param digitSeparator - Grouping separator to use (default is comma ',')
            * \param decimalSeparator - Decimal separator to use (default is period '.')
            * \param groupingSize - Number of digits to group together (default is 3, i.e. thousands)
            * \param firstGroupingSize - If > 0, an alternative grouping size to use for the first group (some languages use this, e.g. Hindi groups 12,34,56,789.00)
            * \returns The length of the string in the buffer (including terminating null byte) after modifications
            */
            int GroupDigits(char* buffer, size_t bufferSize, size_t decimalPosHint = 0, char digitSeparator = ',', char decimalSeparator = '.', int groupingSize = 3, int firstGroupingSize = 0);
        }

        //////////////////////////////////////////////////////////////////////////
        //! StringFunc::AssetPath Namespace
        /*! For string functions that support asset path calculations
        */
        namespace AssetPath
        {
            //! CalculateBranchToken
            /*! Calculate the branch token that is used for asset processor connection negotiations
            *
            * \param engineRootPath - The absolute path to the engine root to base the token calculation on
            * \param token       - The result of the branch token calculation
            */
            void CalculateBranchToken(AZStd::string_view engineRootPath, AZStd::string& token);
            void CalculateBranchToken(AZStd::string_view engineRootPath, AZ::IO::FixedMaxPathString& token);
        }

        //////////////////////////////////////////////////////////////////////////
        //! StringFunc::AssetDatabasePath Namespace
        /*! For string functions for physical paths and how the database expects them to appear.
         *! The AssetDatabse expects physical path to be expressed in term of three things:
         *!     1. The AssetDatabaseRoot: which is a simple string name that maps to a folder under
         *!     the ProjectRoot() where the asset resides, currently the only root we have is "intermediateassets"
         *!     and it currently maps to a folder called "\\intermediateassets\\"
         *!     2. The AssetDatabasePath: which is a string of the relative path from the AssetDatabaseRoot. This
         *!     includes the filename without the extension if it has one. It has no starting separator
         */
        namespace AssetDatabasePath
        {
            //! Normalize
            /*! Normalizes a asset database path and returns returns StringFunc::AssetDatabasePath::IsValid()
            *! strips all AZ_DATABASE_INVALID_CHARACTERS
            *! all AZ_WRONG_DATABASE_SEPARATOR's are replaced with AZ_CORRECT_DATABASE_SEPARATOR's
            *! make sure it has an ending AZ_CORRECT_DATABASE_SEPARATOR
            *! if AZ_FILENAME_ALLOW_SPACES is not set then it strips AZ_SPACE_CHARACTERS
            Example: Normalize a AZStd::string root
            StringFunc::Root::Normalize(a="C:/p4/game/") == true; a=="C:/p4/game/"
            StringFunc::Root::Normalize(a="/p4/game/") == false; Doesn't have a drive
            StringFunc::Root::Normalize(a="C:/p4/game") == true; a=="C:/p4/game/"
            */
            bool Normalize(AZStd::string& inout);

            //! IsValid
            /*! Returns if this string has the requirements to be a root
            *! fail if no drive
            *! make sure it has an ending AZ_CORRECT_DATABASE_SEPARATOR
            *! makes sure all AZ_WRONG_DATABASE_SEPARATOR's are replaced with AZ_CORRECT_DATABASE_SEPARATOR's
            *! if AZ_FILENAME_ALLOW_SPACES is not set then it makes sure it doesn't have any AZ_SPACE_CHARACTERS
            Example: Is the AZStd::string a valid root
            StringFunc::Root::IsValid(a="C:/p4/game/") == true
            StringFunc::Root::IsValid(a="C:/p4/game/") == false; Wrong separators
            StringFunc::Root::IsValid(a="/p4/game/") == false; Doesn't have a drive
            StringFunc::Root::IsValid(a="C:/p4/game") == false; Doesn't have an ending separator
            */
            bool IsValid(const char* in);

            //! Split
            /*! Splits a full path into pieces, returns if it was successful
            *! EX: StringFunc::AssetDatabasePath::Split("C:/project/intermediateassets/somesubdir/anothersubdir/someFile.ext", &root, &path, &file, &extension) = true; root=="intermediateassets", path=="somesubdir/anothersubdir/", file=="someFile", extension==".ext"
            */
            bool Split(const char* in, AZStd::string* pDstProjectRootOut = nullptr, AZStd::string* pDstDatabaseRootOut = nullptr, AZStd::string* pDstDatabasePathOut = nullptr, AZStd::string* pDstDatabaseFileOut = nullptr, AZStd::string* pDstFileExtensionOut = nullptr);

            //! Join
            /*! Joins two pieces of a asset database path returns if it was successful
            *! This has similar behavior to Python pathlib '/' operator and os.path.join
            *! Specifically, that it uses the last absolute path as the anchor for the resulting path
            *! https://docs.python.org/3/library/pathlib.html#pathlib.PurePath
            *! This means that joining StringFunc::AssetDatabasePath::Join("/etc/", "/usr/bin") results in "/usr/bin"
            *! not "/etc/usr/bin"
            *! EX: StringFunc::AssetDatabasePath::Join("p4/game","info/some.file", a) == true; a== "p4/game/info/some.file"
            *! EX: StringFunc::AssetDatabasePath::Join("p4/game/info", "game/info/some.file", a) == true; a== "p4/game/info/game/info/some.file"
            *! EX: StringFunc::AssetDataPathPath::Join("p4/game/info", "/game/info/some.file", a) == true; a== "/game/info/some.file"
            *! (Replaces root directory part)
            */
            bool Join(const char* pFirstPart, const char* pSecondPart, AZStd::string& out, bool bCaseInsensitive = true, bool bNormalize = true);
        };

        //////////////////////////////////////////////////////////////////////////
        //! StringFunc::Root Namespace
        /*! For string functions that deal with path roots
        */
        namespace Root
        {
            //! Normalize
            /*! Normalizes a root and returns returns StringFunc::Root::IsValid()
             *! strips all AZ_FILESYSTEM_INVALID_CHARACTERS
             *! all AZ_WRONG_FILESYSTEM_SEPARATOR's are replaced with AZ_CORRECT_FILESYSTEM_SEPARATOR's
             *! make sure it has an ending AZ_CORRECT_FILESYSTEM_SEPARATOR
             *! if AZ_FILENAME_ALLOW_SPACES is not set then it strips AZ_SPACE_CHARACTERS
             Example: Normalize a AZStd::string root
             StringFunc::Root::Normalize(a="C:\\p4/game/") == true; a=="C:\\p4\\game\\"
             StringFunc::Root::Normalize(a="\\p4/game/") == false; Doesn't have a drive
             StringFunc::Root::Normalize(a="C:\\p4/game") == true; a=="C:\\p4\\game\\"
             */
            bool Normalize(AZStd::string& inout);

            //! IsValid
            /*! Returns if this string has the requirements to be a root
            *! fail if no drive
            *! make sure it has an ending AZ_CORRECT_FILESYSTEM_SEPARATOR
            *! makes sure all AZ_WRONG_FILESYSTEM_SEPARATOR's are replaced with AZ_CORRECT_FILESYSTEM_SEPARATOR's
            *! if AZ_FILENAME_ALLOW_SPACES is not set then it makes sure it doesn't have any AZ_SPACE_CHARACTERS
            Example: Is the AZStd::string a valid root
            StringFunc::Root::IsValid(a="C:\\p4\\game\\") == true
            StringFunc::Root::IsValid(a="C:/p4/game/") == false; Wrong separators
            StringFunc::Root::IsValid(a="\\p4\\game\\") == false; Doesn't have a drive
            StringFunc::Root::IsValid(a="C:\\p4\\game") == false; Doesn't have an ending separator
            */
            bool IsValid(const char* in);
        }

        //////////////////////////////////////////////////////////////////////////
        //! StringFunc::RelativePath Namespace
        /*! For string functions that deal with relative paths
        */
        namespace RelativePath
        {
            //! Normalize
            /*! Normalizes a relative path and returns returns StringFunc::RelativePath::IsValid()
             *! strips all AZ_FILESYSTEM_INVALID_CHARACTERS
             *! makes sure it does not have a drive
             *! makes sure it does not start with a AZ_CORRECT_FILESYSTEM_SEPARATOR
             *! make sure it ends with a AZ_CORRECT_FILESYSTEM_SEPARATOR
             *! makes sure all AZ_WRONG_FILESYSTEM_SEPARATOR's are replaced with AZ_CORRECT_FILESYSTEM_SEPARATOR's
             Example: Normalize a AZStd::string root
             StringFunc::RelativePath::Normalize(a="\\p4/game/") == true; a=="p4\\game\\"
             StringFunc::RelativePath::Normalize(a="p4/game/") == true; a=="p4\\game\\"
             StringFunc::RelativePath::Normalize(a="C:\\p4/game") == false; Has a drive
             */
            bool Normalize(AZStd::string& inout);

            //! IsValid
            /*! Returns if this string has the requirements to be a relative path
             *! makes sure it has no AZ_FILESYSTEM_INVALID_CHARACTERS
             *! makes sure all AZ_WRONG_FILESYSTEM_SEPARATOR's are replaced with AZ_CORRECT_FILESYSTEM_SEPARATOR's
             *! fails if it HasDrive()
             *! make sure it has an ending AZ_CORRECT_FILESYSTEM_SEPARATOR
             *! if AZ_FILENAME_ALLOW_SPACES is not set then it makes sure it doesn't have any spaces
             Example: Is the AZStd::string a valid root
             StringFunc::Root::IsValid(a="p4\\game\\") == true
             StringFunc::Root::IsValid(a="p4/game/") == false; Wrong separators
             StringFunc::Root::IsValid(a="\\p4\\game\\") == false; Starts with a separator
             StringFunc::Root::IsValid(a="C:\\p4\\game") == false; Has a drive
             */
            bool IsValid(const char* in);
        }

        //////////////////////////////////////////////////////////////////////////
        //! StringFunc::Path Namespace
        /*! For string functions that deal with general pathing.
         *! A path is made up of one or more "component" parts:
         *! a root (which can be made of one or more components)
         *! the relative (which can be made of one or more components)
         *! the full name (which can be only 1 component, and is the only required component)
         *! The general form of a path is:
         *! |                               full path                                                   |
         *! |                       path                                    |       fullname            |
         *! |           root                |       relativepath            | filename  |   extension   |
         *! |   drive       |               folder path                     |
         *! |   drive       |
         *! |   component   |   component   |   component   |   component   |
         *! On PC and similar platforms,
         *! [drive [A-Z] AZ_FILESYSTEM_DRIVE_SEPARATOR or]AZ_CORRECT_FILESYSTEM_SEPARATOR rootPath AZ_CORRECT_FILESYSTEM_SEPARATOR relativePath AZ_CORRECT_FILESYSTEM_SEPARATOR filename<AZ_FILESYSTEM_EXTENSION_SEPARATOR ext>
         *!         NETWORK_START[machine] AZ_CORRECT_FILESYSTEM_SEPARATOR network share
         *! Example: "C:\\p4\\some.file"
         *! Example: "\\\\18byrne\\share\\p4\\some.file"
         *!
         *! For ease of understanding all examples assume Windows PC:
         *! AZ_CORRECT_FILESYSTEM_SEPARATOR is '\\' character
         *! AZ_WRONG_FILESYSTEM_SEPARATOR is '/' character
         *! AZ_FILESYSTEM_EXTENSION_SEPARATOR is '.' character
         *! AZ_FILESYSTEM_DRIVE_SEPARATOR is ':' character
         *! AZ_NETWORK_START "\\\\" string
         *!
         *! "root" is defined as the beginning of a valid path (it is a point from which a relative path/resource can be appended)
         *! a root must have a "drive"
         *! a root may or may not have a AZ_FILESYSTEM_DRIVE_SEPARATOR
         *! a root may or may not have a AZ_NETWORK_START
         *! a root always ends with a AZ_CORRECT_FILESYSTEM_SEPARATOR
         *! => C:\\p4\\Main\\Source\\GameAssets\\
         *! => D:\\
         *! => \\\\18username\\p4\\Main\\Source\\GameAssets\\
         *!
         *! "filename" is defined as all the characters after the last AZ_CORRECT_FILESYSTEM_SEPARATOR up to
         *! but not including AZ_FILESYSTEM_EXTENSION_SEPARATOR if any.
         *! EX: D:\\p4\\Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml
         *! => some
         *!
         *! "extension" is defined as the last AZ_FILESYSTEM_EXTENSION_SEPARATOR + all characters afterward
         *! Note extension is considered part of the fullname component and its size takes away
         *! from the MAX_FILE_NAME_LEN, so filename + AZ_FILESYSTEM_EXTENSION_SEPARATOR + extension (if any), has to be <= MAX_FILE_NAME_LEN
         *! Note all functions that deal with extensions should be tolerant of missing AZ_FILESYSTEM_EXTENSION_SEPARATOR
         *! EX. ReplaceExtension("xml") and (".xml") both should arrive at the same result
         *! Note also when stripping name the extension is also stripped
         *! EX: D:\\p4\\Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml
         *! => .xml
         *!
         *! "fullname" is defined as name + ext (if any)
         *! EX: D:\\p4\\Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml
         *! => some.xml
         *!
         *! "relativePath" is defined as anything in between a root and the filename
         *! Note: relative paths DO NOT start with a AZ_CORRECT_FILESYSTEM_SEPARATOR but ALWAYS end with one
         *! EX: D:\\p4\\Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml
         *! => gameinfo\\Characters\\
         *!
         *! "path" is defined as all characters up to and including the last AZ_CORRECT_FILESYSTEM_SEPARATOR
         *! EX: D:\\p4\\Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml
         *! => D:\\p4\\Main\\Source\\GameAssets\\gameinfo\\Characters\\
         *!
         *! "folder path" is defined as all characters in between and including the first and last AZ_CORRECT_FILESYSTEM_SEPARATOR
         *! EX: D:\\p4\\Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml
         *! => \\p4\\Main\\Source\\GameAssets\\gameinfo\\Characters\\
         *! EX: /app_home/gameinfo/Characters/some.xml
         *! => /gameinfo/Characters/
         *!
         *! "drive" on PC and similar platforms
         *! is defined as all characters up to and including the first AZ_FILESYSTEM_DRIVE_SEPARATOR or
         *! all characters up to but not including the first AZ_CORRECT_FILESYSTEM_SEPARATOR after a NETWORK_START
         *! D:\\p4\\Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml
         *! => D:
         *! EX: \\\\18username\\p4\\Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml
         *! => \\\\18username
         *!
         *! "fullpath" is defined as having a root, relative path and filename
         *! EX: D:\\p4\\Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml
         *!
         *! "component" refers to any piece of a path. All components have a AZ_CORRECT_FILESYSTEM_SEPARATOR except file name.
         *! EX: D:\\p4\\Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml
         *! => "D:\\", "p4\\", "Main\\", "Source\\", "GameAssets\\", "gameinfo\\", "Characters\\", "some.xml"
         *! EX: /app_home/gameinfo/Characters/some.xml
         *! => "/app_home/", "gameinfo/", "Characters/", "some.xml"
         */
        namespace Path
        {
            using FixedString = AZ::IO::FixedMaxPathString;
            //! Normalize
            /*! Normalizes a path and returns returns StringFunc::Path::IsValid()
             *! strips all AZ_FILESYSTEM_INVALID_CHARACTERS
             *! all AZ_WRONG_FILESYSTEM_SEPARATOR's are replaced with AZ_CORRECT_FILESYSTEM_SEPARATOR's
             *! all AZ_DOUBLE_CORRECT_SEPARATOR's (not in the "drive") are replaced with AZ_CORRECT_FILESYSTEM_SEPARATOR's
             *! makes sure it does not have a drive
             *! makes sure it does not start with a AZ_CORRECT_FILESYSTEM_SEPARATOR
             Example: Normalize a AZStd::string path
             StringFunc::Path::Normalize(a="\\p4/game/") == true; a=="p4\\game\\"
             StringFunc::Path::Normalize(a="p4/game/") == true; a=="p4\\game\\"
             StringFunc::Path::Normalize(a="C:\\p4/game") == true; a=="C:\\p4\\game\\"
             */
            bool Normalize(AZStd::string& inout);
            bool Normalize(FixedString& inout);

            //! IsValid
            /*! Returns if this string has the requirements to be a path
             *! make sure its not empty
             *! make sure it has no AZ_FILESYSTEM_INVALID_CHARACTERS
             *! make sure it has no WRONG_SEPARATORS
             *! make sure it HasDrive() if bHasDrive is set
             *! if AZ_FILENAME_ALLOW_SPACES is not set then it makes sure it doesn't have any spaces
             *! make sure total length <= AZ_MAX_PATH_LEN
             Example: Is the AZStd::string a valid root
             StringFunc::Path::IsValid(a="p4\\game\\") == true
             StringFunc::Path::IsValid(a="p4/game/") == false; Wrong separators
             StringFunc::Path::IsValid(a="C:\\p4\\game\\") == true
             StringFunc::Path::IsValid(a="C:\\p4\\game") == true
             StringFunc::Path::IsValid(a="C:\\p4\\game\\some.file") == true
             */
            bool IsValid(const char* in, bool bHasDrive = false, bool bHasExtension = false, AZStd::string* errors = NULL);

            //! ConstructFull
            /*! Constructs a full path from pieces and does some minimal smart normalization to make it easier, returns if it was successful
             *! EX: StringFunc::Path::ContructFull("C:\\p4", "some.file", a) == true; a == "C:\\p4\\some.file"
             *! EX: StringFunc::Path::ContructFull("C:\\p4", "game/info\\some.file", a, true) == true; a == "C:\\p4\\game\\info\\some.file"
             *! EX: StringFunc::Path::ContructFull("C:\\p4", "/some", "file", a, true) == true; a== "C:\\p4\\some.file"
             *! EX: StringFunc::Path::ContructFull("C:\\p4", "some", ".file", a) == true; a== "C:\\p4\\some.file"
             *! EX: StringFunc::Path::ContructFull("C:\\p4", "info", "some", "file", a) == true; a=="C:\\p4\\info\\some.file"
             */
            bool ConstructFull(const char* pRootPath, const char* pFileName, AZStd::string& out, bool bNormalize = false);
            bool ConstructFull(const char* pRootPath, const char* pFileName, const char* pFileExtension, AZStd::string& out, bool bNormalize = false);
            bool ConstructFull(const char* pRoot, const char* pRelativePath, const char* pFileName, const char* pFileExtension, AZStd::string& out, bool bNormalize = false);

            //! Split
            /*! Splits a full path into pieces, returns if it was successful
            *! EX: StringFunc::Path::Split("C:\\p4\\game\\info\\some.file", &drive, &folderPath, &fileName, &extension) = true; drive==C: , folderPath=="\\p4\\game\\info\\", filename=="some", extension==".file"
            */
            bool Split(const char* in, AZStd::string* pDstDriveOut = nullptr, AZStd::string* pDstFolderPathOut = nullptr, AZStd::string* pDstNameOut = nullptr, AZStd::string* pDstExtensionOut = nullptr);

            //! Join
            /*! Joins two pieces of a path returns if it was successful
            *! This has similar behavior to Python pathlib '/' operator and os.path.join
            *! Specifically, that it uses the last absolute path as the anchor for the resulting path
            *! https://docs.python.org/3/library/pathlib.html#pathlib.PurePath
            *! This means that joining StringFunc::Path::Join("C:\\O3DE" "F:\\O3DE") results in "F:\\O3DE"
            *! not "C:\\O3DE\\F:\\O3DE"
            *! EX: StringFunc::Path::Join("C:\\p4\\game","info\\some.file", a) == true; a== "C:\\p4\\game\\info\\some.file"
            *! EX: StringFunc::Path::Join("C:\\p4\\game\\info", "game\\info\\some.file", a) == true; a== "C:\\p4\\game\\info\\game\\info\\some.file"
            *! EX: StringFunc::Path::Join("C:\\p4\\game\\info", "\\game\\info\\some.file", a) == true; a== "C:\\game\\info\\some.file"
            *! (Replaces root directory part)
            */
            bool Join(const char* pFirstPart, const char* pSecondPart, AZStd::string& out, bool bCaseInsensitive = true, bool bNormalize = true);
            bool Join(const char* pFirstPart, const char* pSecondPart, FixedString& out, bool bCaseInsensitive = true, bool bNormalize = true);

            //! HasDrive
            /*! returns if the c-string has a "drive"
            *! EX: StringFunc::Path::HasDrive("C:\\p4\\game\\info\\some.file") == true
            *! EX: StringFunc::Path::HasDrive("\\p4\\game\\info\\some.file") == false
            *! EX: StringFunc::Path::HasDrive("\\\\18usernam\\p4\\game\\info\\some.file") == true
            */
            bool HasDrive(const char* in, bool bCheckAllFileSystemFormats = false);

            //! HasExtension
            /*! returns if the c-string has an "extension"
            *! make sure its not empty
            *! make sure AZ_FILESYSTEM_EXTENSION_SEPARATOR is not the first or last character
            *! make sure AZ_FILESYSTEM_EXTENSION_SEPARATOR occurs after the last AZ_CORRECT_FILESYSTEM_SEPARATOR (if it has one)
            *! make sure its at most MAX_EXTENSION_LEN chars including the AZ_FILESYSTEM_EXTENSION_SEPARATOR character
            *! EX: StringFunc::Path::HasExtension("C:\\p4\\game\\info\\some.file") == true
            *! EX: StringFunc::Path::HasExtension("\\p4\\game\\info\\some") == false
            */
            bool HasExtension(const char* in);

            //! IsExtension
            /*! returns if the c-string has an "extension", optionally case case sensitive
            *! make sure its HasExtension()
            *! tolerate not having an AZ_FILESYSTEM_EXTENSION_SEPARATOR on the comparison
            *! EX: StringFunc::Path::IsExtension("C:\\p4\\game\\info\\some.file", "file") == true
            *! EX: StringFunc::Path::IsExtension("C:\\p4\\game\\info\\some.file", ".file") == true
            *! EX: StringFunc::Path::IsExtension("\\p4\\game\\info\\some") == false
            *! EX: StringFunc::Path::IsExtension("\\p4\\game\\info\\some.file", ".FILE", true) == false
            */
            bool IsExtension(const char* in, const char* pExtension, bool bCaseInsenitive = true);

            //! IsRelative
            /*! returns if the c-string fulfills the requirements to be "relative"
            *! make sure its not empty
            *! make sure its does not have a "drive"
            *! make sure its doesn't start with AZ_CORRECT_FILESYSTEM_SEPARATOR
            *! EX: StringFunc::Path::IsRelative("p4\\game\\info\\some.file") == true
            *! EX: StringFunc::Path::IsRelative("\\p4\\game\\info\\some.file") == false
            *! EX: StringFunc::Path::IsRelative("C:\\p4\\game\\info\\some.file") == false
            */
            bool IsRelative(const char* in);

            //! StripDrive
            /*! gets rid of the drive component if it has one, returns if it removed one or not
            *! EX: StringFunc::Path::StripDrive(a="C:\\p4\\game\\info\\some.file") == true; a=="\\p4\\game\\info\\some.file"
            */
            bool StripDrive(AZStd::string& inout);

            //! StripPath
            /*! gets rid of the path if it has one, returns if it removed one or not
            *! EX: StringFunc::Path::StripPath(a="C:\\p4\\game\\info\\some.file") == true; a=="some.file"
            */
            void StripPath(AZStd::string& out);

            //! StripFullName
            /*! gets rid of the full file name if it has one, returns if it removed one or not
            *! EX: StringFunc::Path::StripFullName(a="C:\\p4\\game\\info\\some.file") == true; a=="C:\\p4\\game\\info"
            */
            void StripFullName(AZStd::string& out);

            //! StripExtension
            /*! gets rid of the extension if it has one, returns if it removed one or not
            *! EX: StringFunc::Path::StripExtension(a="C:\\p4\\game\\info\\some.file") == true; a=="C:\\p4\\game\\info\\some"
            */
            void StripExtension(AZStd::string& inout);

            //! StripComponent
            /*! gets rid of the first or last if it has one, returns if it removed one or not
            *! EX: StringFunc::Path::StripComponent(a="D:\\p4\\Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml") == true; a=="p4\\Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml"
            *!  EX: StringFunc::Path::StripComponent(a="p4\\Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml") == true; a=="Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml"
            *!  EX: StringFunc::Path::StripComponent(a="Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml") == true; a=="Source\\GameAssets\\gameinfo\\Characters\\some.xml"
            *!  EX: StringFunc::Path::StripComponent(a="Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml", true) == true; a=="Main\\Source\\GameAssets\\gameinfo\\Characters"
            *!  EX: StringFunc::Path::StripComponent(a="Main\\Source\\GameAssets\\gameinfo\\Characters", true) == true; a=="Main\\Source\\GameAssets\\gameinfo"
            */
            bool StripComponent(AZStd::string& inout, bool bLastComponent = false);

            //! GetDrive
            /*! if a c-string has a drive put it in AZStd::string, returns if it was sucessful
            *! EX: StringFunc::Path::GetDrive("C:\\p4\\game\\info\\some.file",a) == true; a=="C:"
            *! EX: StringFunc::Path::GetDrive("\\\\18username\\p4\\game\\info\\some.file",a) == true; a=="\\\18username"
            */
            bool GetDrive(const char* in, AZStd::string& out);

            //! GetParentDir
            /*! Retrieves the parent directory using the supplied @in string
            *!  If @in ends with a trailing slash, it is treated the same as a directory without the trailing slash
            *!  EX: StringFunc::Path::GetParentDir("D:\\p4\\Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml",a) == true; a=="D:\\p4\\Main\\Source\\GameAssets\\gameinfo\\Characters"
            *!  EX: StringFunc::Path::GetParentDir("D:\\p4\\Main\\Source\\GameAssets\\gameinfo\\Characters\\",a) == true; a=="D:\\p4\\Main\\Source\\GameAssets\\gameinfo"
            *!  EX: StringFunc::Path::GetParentDir("D:\\p4\\Main\\Source\\GameAssets\\gameinfo\\Characters",a) == true; a=="D:\\p4\\Main\\Source\\GameAssets\\gameinfo"
            *!  EX: StringFunc::Path::GetParentDir("D:\\",a) == true; a=="D:\\"
            */
            AZStd::optional<AZStd::string_view> GetParentDir(AZStd::string_view path);

            //! GetFullPath
            /*! if a c-string has a fullpath put it in AZStd::string, returns if it was sucessful
            *! EX: StringFunc::Path::GetFullPath("D:\\p4\\Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml",a) == true; a=="D:\\p4\\Main\\Source\\GameAssets\\gameinfo\\Characters"
            */
            bool GetFullPath(const char* in, AZStd::string& out);

            //! GetFolderPath
            /*! if a c-string has a folderpath put it in AZStd::string, returns if it was sucessful
            *! EX: StringFunc::Path::GetFolderPath("D:\\p4\\Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml",a) == true; a=="\\p4\\Main\\Source\\GameAssets\\gameinfo\\Characters"
            */
            bool GetFolderPath(const char* in, AZStd::string& out);

            //! GetFolder
            /*! if a c-string has a beginning or ending folder put it in AZStd::string, returns if it was sucessful
            *! EX: StringFunc::Path::GetFolder("D:\\p4\\Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml", a) == true; a=="p4"
            *! EX: StringFunc::Path::GetFolder("D:\\p4\\Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml", a, true) == true; a=="Characters"
            */
            bool GetFolder(const char* in, AZStd::string& out, bool bFirst = false);

            //! GetFullFileName
            /*! if a c-string has afull file name put it in AZStd::string, returns if it was sucessful
            *! EX: StringFunc::Path::GetFullFileName("D:\\p4\\Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml", a) == true; a=="some.xml"
            *! EX: StringFunc::Path::GetFullFileName("D:\\p4\\Main\\Source\\GameAssets\\gameinfo\\Characters\\", a) == false;
            */
            bool GetFullFileName(const char* in, AZStd::string& out);

            //! GetFileName
            /*! if a c-string has a file name put it in AZStd::string, returns if it was sucessful
            *! EX: StringFunc::Path::GetFileName("D:\\p4\\Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml", a) == true; a=="some"
            *! EX: StringFunc::Path::GetFileName("D:\\p4\\Main\\Source\\GameAssets\\gameinfo\\Characters\\", a) == false;
            */
            bool GetFileName(const char* in, AZStd::string& out);

            //! GetExtension
            /*! if a c-string has an extension put it in AZStd::string, returns if it was sucessful
            *! EX: StringFunc::Path::GetFileName("D:\\p4\\Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml", a) == true; a==".xml"
            *! EX: StringFunc::Path::GetFileName("D:\\p4\\Main\\Source\\GameAssets\\gameinfo\\Characters\\", a) == false;
            *! EX: StringFunc::Path::GetExtension("D:\\p4\\Main\\Source\\GameAssets\\gameinfo\\Characters\\some.xml", a, false) == true; a=="xml"
            */
            bool GetExtension(const char* in, AZStd::string& out, bool includeDot = true);

            //! ReplaceFullName
            /*! if a AZStd::string has a full name then replace it with pFileName and optionally with pExtension
            *! EX: StringFunc::Path::ReplaceFullName(a="D:\\p4\\some.file", "other", ".xml"); a=="D:\\p4\\other.xml"
            *! EX: StringFunc::Path::ReplaceFullName(a="D:\\p4\\some.file", "other.xml"); a=="D:\\p4\\other.xml"
            *! EX: StringFunc::Path::ReplaceFullName(a="D:\\p4\\some.file", "other.file", "xml"); a=="D:\\p4\\other.xml"
            *! EX: StringFunc::Path::ReplaceFullName(a="D:\\p4\\some.file", "other.file", ""); a=="D:\\p4\\other"
            */
            void ReplaceFullName(AZStd::string& inout, const char* pFileName, const char* pFileExtension = nullptr);

            //! ReplaceExtension
            /*! if a AZStd::string HasExtension() then replace it with pDrive
            *! EX: StringFunc::Path::ReplaceExtension(a="D:\\p4\\some.file", "xml"); a=="D:\\p4\\some.xml"
            *! EX: StringFunc::Path::ReplaceExtension(a="D:\\p4\\some.file", ".xml"); a=="D:\\p4\\some.xml"
            *! EX: StringFunc::Path::ReplaceExtension(a="D:\\p4\\some.file", ""); a=="D:\\p4\\some"
            */
            void ReplaceExtension(AZStd::string& inout, const char* pFileExtension);

            //! AppendSeparator
            /*! Appends the correct separator to the path
            *! EX: StringFunc::Path::AppendSeparator("C:\\project\\intermediateassets", &path);
            *! path=="C:\\project\\intermediateassets\\"
            */
            AZStd::string& AppendSeparator(AZStd::string& inout);

            //! MakeUniqueFilenameWithSuffix
            /*! given a directory path and an optional extension will return a unique filename
            *! EX: StringFunc::Path::MakeUniqueFilenameWithSuffix("c:\\folder\\NewFile.txt", "-copy")
            *! if "NewFile.txt" doesn't exist, returns "c:\\folder\\Newfile.txt"
            *! if "NewFile.txt" exists, returns "c:\\folder\\Newfile-copy1.txt"
            *! if both "NewFile.txt" and "NewFile-copy1.txt" exist, returns "c:\\folder\\Newfile-copy2.txt" etc.
            */
            AZ::IO::FixedMaxPath MakeUniqueFilenameWithSuffix(const AZ::IO::PathView& basePath, const AZStd::string_view& suffix = "");

        } // namespace Path

        namespace Json
        {
            //! ToEscapedString
            /* Escape a string to make it compatible for saving to the json format
            *! EX: StringFunc::Json::ToEscapeString(""'"")
            *! output=="\"'\""
            */
            AZStd::string& ToEscapedString(AZStd::string& inout);
        } // namespace Json

        namespace Base64
        {
            //! Base64Encode
            /* Encodes Binary data to base-64 format which allows it to be stored safely in json or xml data
            *! EX: StringFunc::Base64::Base64Encode(reinterpret_cast<AZ::u8*>("NUL\0InString"), AZ_ARRAY_SIZE("NUL\0InString") - 1);
            *! output = "TlVMAEluU3RyaW5n"
            */
            AZStd::string Encode(const AZ::u8* in, const size_t size);
            //! Base64Decode
            /* Decodes Base64 Text data to binary data and appends result int out array
            *! If the the supplied text is not valid Base64 then false will be result @out array will be unmodified
            *! EX: StringFunc::Base64::Base64Decode("TlVMAEluU3RyaW5n", strlen("TlVMAEluU3RyaW5n"));
            *! output = {'N','U', 'L', '\0', 'I', 'n', 'S', 't', 'r', 'i', 'n', 'g'}
            */
            bool Decode(AZStd::vector<AZ::u8>& out, const char* in, const size_t size);

        };

        namespace Utf8
        {
            /**
             * Check to see if a string contains any byte that cannot be encoded in 7-bit ASCII.
             * @param in A string with UTF-8 encoding.
             * @return true if the string passed in contains any byte that cannot be encoded in 7-bit ASCII, otherwise false.
             */
            bool CheckNonAsciiChar(const AZStd::string& in);
        }
    } // namespace StringFunc
} // namespace AzFramework
