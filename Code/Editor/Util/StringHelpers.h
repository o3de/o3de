/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYCOMMONTOOLS_STRINGHELPERS_H
#define CRYINCLUDE_CRYCOMMONTOOLS_STRINGHELPERS_H
#pragma once

#include <CryString.h>

namespace StringHelpers
{
    // compares two strings to see if they are the same or different, case sensitive
    // returns 0 if the strings are the same, a -1 if the first string is bigger, or a 1 if the second string is bigger
    int Compare(const string& str0, const string& str1);
    int Compare(const wstring& str0, const wstring& str1);

    // compares two strings to see if they are the same or different, case is ignored
    // returns 0 if the strings are the same, a -1 if the first string is bigger, or a 1 if the second string is bigger
    int CompareIgnoreCase(const string& str0, const string& str1);
    int CompareIgnoreCase(const wstring& str0, const wstring& str1);

    // compares two strings to see if they are the same, case senstive
    // returns true if they are the same or false if they are different
    bool Equals(const string& str0, const string& str1);
    bool Equals(const wstring& str0, const wstring& str1);

    // compares two strings to see if they are the same, case is ignored
    // returns true if they are the same or false if they are different
    bool EqualsIgnoreCase(const string& str0, const string& str1);
    bool EqualsIgnoreCase(const wstring& str0, const wstring& str1);

    // checks to see if a string starts with a specified string, case sensitive
    // returns true if the string does start with a specified string or false if it does not
    bool StartsWith(const string& str, const string& pattern);
    bool StartsWith(const wstring& str, const wstring& pattern);

    // checks to see if a string starts with a specified string, case is ignored
    // returns true if the string does start with a specified string or false if it does not
    bool StartsWithIgnoreCase(const string& str, const string& pattern);
    bool StartsWithIgnoreCase(const wstring& str, const wstring& pattern);

    // checks to see if a string ends with a specified string, case sensitive
    // returns true if the string does end with a specified string or false if it does not
    bool EndsWith(const string& str, const string& pattern);
    bool EndsWith(const wstring& str, const wstring& pattern);

    // checks to see if a string ends with a specified string, case is ignored
    // returns true if the string does end with a specified string or false if it does not
    bool EndsWithIgnoreCase(const string& str, const string& pattern);
    bool EndsWithIgnoreCase(const wstring& str, const wstring& pattern);

    // checks to see if a string contains a specified string, case sensitive
    // returns true if the string does contain the specified string or false if it does not
    bool Contains(const string& str, const string& pattern);
    bool Contains(const wstring& str, const wstring& pattern);

    // checks to see if a string contains a specified string, case is ignored
    // returns true if the string does contain the specified string or false if it does not
    bool ContainsIgnoreCase(const string& str, const string& pattern);
    bool ContainsIgnoreCase(const wstring& str, const wstring& pattern);

    // checks to see if a string contains a wildcard string pattern, case sensitive
    // returns true if the string does match the wildcard string pattern or false if it does not
    bool MatchesWildcards(const string& str, const string& wildcards);
    bool MatchesWildcards(const wstring& str, const wstring& wildcards);

    // checks to see if a string contains a wildcard string pattern, case is ignored
    // returns true if the string does match the wildcard string pattern or false if it does not
    bool MatchesWildcardsIgnoreCase(const string& str, const string& wildcards);
    bool MatchesWildcardsIgnoreCase(const wstring& str, const wstring& wildcards);

    bool MatchesWildcardsIgnoreCaseExt(const string& str, const string& wildcards, std::vector<string>& wildcardMatches);
    bool MatchesWildcardsIgnoreCaseExt(const wstring& str, const wstring& wildcards, std::vector<wstring>& wildcardMatches);

    string TrimLeft(const string& s);
    wstring TrimLeft(const wstring& s);

    string TrimRight(const string& s);
    wstring TrimRight(const wstring& s);

    string Trim(const string& s);
    wstring Trim(const wstring& s);

    string RemoveDuplicateSpaces(const string& s);
    wstring RemoveDuplicateSpaces(const wstring& s);

    // converts a string with upper case characters to be all lower case
    // returns the string in all lower case
    string MakeLowerCase(const string& s);
    wstring MakeLowerCase(const wstring& s);

    // converts a string with lower case characters to be all upper case
    // returns the string in all upper case
    string MakeUpperCase(const string& s);
    wstring MakeUpperCase(const wstring& s);

    // replace a specified character in a string with a specified replacement character
    // returns string with specified character replaced
    string Replace(const string& s, char oldChar, char newChar);
    wstring Replace(const wstring& s, wchar_t oldChar, wchar_t newChar);

    void ConvertStringByRef(string& out, const string& in);
    void ConvertStringByRef(wstring& out, const string& in);
    void ConvertStringByRef(string& out, const wstring& in);
    void ConvertStringByRef(wstring& out, const wstring& in);

    template <typename O, typename I>
    O ConvertString(const I& in)
    {
        O out;
        ConvertStringByRef(out, in);
        return out;
    }
    
    void Split(const string& str, const string& separator, bool bReturnEmptyPartsToo, std::vector<string>& outParts);
    void Split(const wstring& str, const wstring& separator, bool bReturnEmptyPartsToo, std::vector<wstring>& outParts);

    void SplitByAnyOf(const string& str, const string& separators, bool bReturnEmptyPartsToo, std::vector<string>& outParts);
    void SplitByAnyOf(const wstring& str, const wstring& separators, bool bReturnEmptyPartsToo, std::vector<wstring>& outParts);

    string FormatVA(const char* const format, va_list parg);
    wstring FormatVA(const wchar_t* const format, va_list parg);

    inline string Format(const char* const format, ...)
    {
        if ((format == 0) || (format[0] == 0))
        {
            return string();
        }
        va_list parg;
        va_start(parg, format);
        const string result = FormatVA(format, parg);
        va_end(parg);
        return result;
    }

    inline wstring Format(const wchar_t* const format, ...)
    {
        if ((format == 0) || (format[0] == 0))
        {
            return wstring();
        }
        va_list parg;
        va_start(parg, format);
        const wstring result = FormatVA(format, parg);
        va_end(parg);
        return result;
    }

    //////////////////////////////////////////////////////////////////////////

    void SafeCopy(char* const pDstBuffer, const size_t dstBufferSizeInBytes, const char* const pSrc);
    void SafeCopy(wchar_t* const pDstBuffer, const size_t dstBufferSizeInBytes, const wchar_t* const pSrc);

    void SafeCopyPadZeros(char* const pDstBuffer, const size_t dstBufferSizeInBytes, const char* const pSrc);
    void SafeCopyPadZeros(wchar_t* const pDstBuffer, const size_t dstBufferSizeInBytes, const wchar_t* const pSrc);

    //////////////////////////////////////////////////////////////////////////

    // ASCII
    // ANSI (system default Windows ANSI code page)
    // UTF-8
    // UTF-16

    // ASCII -> UTF-16

    bool Utf16ContainsAsciiOnly(const wchar_t* wstr);
    string ConvertAsciiUtf16ToAscii(const wchar_t* wstr);
    wstring ConvertAsciiToUtf16(const char* str);

    // UTF-8 <-> UTF-16
#if defined(AZ_PLATFORM_WINDOWS)
    wstring ConvertUtf8ToUtf16(const char* str);
    string ConvertUtf16ToUtf8(const wchar_t* wstr);

    inline string ConvertUtfToUtf8(const char* str)
    {
        return string(str);
    }

    inline string ConvertUtfToUtf8(const wchar_t* wstr)
    {
        return ConvertUtf16ToUtf8(wstr);
    }

    inline wstring ConvertUtfToUtf16(const char* str)
    {
        return ConvertUtf8ToUtf16(str);
    }

    inline wstring ConvertUtfToUtf16(const wchar_t* wstr)
    {
        return wstring(wstr);
    }

    // ANSI <-> UTF-8, UTF-16

    wstring ConvertAnsiToUtf16(const char* str);
    string ConvertUtf16ToAnsi(const wchar_t* wstr, char badChar);

    inline string ConvertAnsiToUtf8(const char* str)
    {
        return ConvertUtf16ToUtf8(ConvertAnsiToUtf16(str).c_str());
    }
    inline string ConvertUtf8ToAnsi(const char* str, char badChar)
    {
        return ConvertUtf16ToAnsi(ConvertUtf8ToUtf16(str).c_str(), badChar);
    }
#endif

    // ANSI -> ASCII
    string ConvertAnsiToAscii(const char* str, char badChar);
}
#endif // CRYINCLUDE_CRYCOMMONTOOLS_STRINGHELPERS_H
