/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef _CRY_ENGINE_STRING_UTILS_HDR_
#define _CRY_ENGINE_STRING_UTILS_HDR_

#pragma once

#include "CryString.h"
#include <ISystem.h>
#include <algorithm>  // std::replace, std::min
#include "UnicodeFunctions.h"
#include "UnicodeIterator.h"
#include <set>

#if !defined(RESOURCE_COMPILER)
#   include "CryCrc32.h"
#endif

#if defined(LINUX) || defined(APPLE)
#   include <ctype.h>
#endif

namespace CryStringUtils
{
    // Convert a single ASCII character to lower case, this is compatible with the standard "C" locale (ie, only A-Z).
    inline char toLowerAscii(char c)
    {
        return (c >= 'A' && c <= 'Z') ? (c - 'A' + 'a') : c;
    }

    // Convert a single ASCII character to upper case, this is compatible with the standard "C" locale (ie, only A-Z).
    inline char toUpperAscii(char c)
    {
        return (c >= 'a' && c <= 'z') ? (c - 'a' + 'A') : c;
    }
}

// cry_strXXX() and CryStringUtils_Internal::strXXX():
//
// The functions copy characters from src to dst one by one until any of
// the following conditions is met:
//   1) the end of the destination buffer (minus one character) is reached.
//   2) the end of the source buffer is reached.
//   3) zero character is found in the source buffer.
//
// When any of 1), 2), 3) happens, the functions write the terminating zero
// character to the destination buffer and return.
//
// The functions guarantee writing the terminating zero character to the
// destination buffer (if the buffer can fit at least one character).
//
// The functions return false when a null pointer is passed or when
// clamping happened (i.e. when the end of the destination buffer is
// reached but the source has some characters left).

namespace CryStringUtils_Internal
{
    template <class TChar>
    inline bool strcpy_with_clamp(TChar* const dst, size_t const dst_size_in_bytes, const TChar* const src, size_t const src_size_in_bytes)
    {
        COMPILE_TIME_ASSERT(sizeof(TChar) == sizeof(char) || sizeof(TChar) == sizeof(wchar_t));

        if (!dst || dst_size_in_bytes < sizeof(TChar))
        {
            return false;
        }

        if (!src || src_size_in_bytes < sizeof(TChar))
        {
            dst[0] = 0;
            return src != 0;  // we return true for non-null src without characters
        }

        const size_t src_n = src_size_in_bytes / sizeof(TChar);
        const size_t n = (std::min)(dst_size_in_bytes / sizeof(TChar) - 1, src_n);

        for (size_t i = 0; i < n; ++i)
        {
            dst[i] = src[i];
            if (!src[i])
            {
                return true;
            }
        }

        dst[n] = 0;
        return n >= src_n || src[n] == 0;
    }

    template <class TChar>
    inline bool strcat_with_clamp(TChar* const dst, size_t const dst_size_in_bytes, const TChar* const src, size_t const src_size_in_bytes)
    {
        COMPILE_TIME_ASSERT(sizeof(TChar) == sizeof(char) || sizeof(TChar) == sizeof(wchar_t));

        if (!dst || dst_size_in_bytes < sizeof(TChar))
        {
            return false;
        }

        const size_t dst_n = dst_size_in_bytes / sizeof(TChar) - 1;

        size_t dst_len = 0;
        while (dst_len < dst_n && dst[dst_len])
        {
            ++dst_len;
        }

        if (!src || src_size_in_bytes < sizeof(TChar))
        {
            dst[dst_len] = 0;
            return src != 0;  // we return true for non-null src without characters
        }

        const size_t src_n = src_size_in_bytes / sizeof(TChar);
        const size_t n = (std::min)(dst_n - dst_len, src_n);
        TChar* dst_ptr = &dst[dst_len];

        for (size_t i = 0; i < n; ++i)
        {
            *dst_ptr++ = src[i];
            if (!src[i])
            {
                return true;
            }
        }

        *dst_ptr = 0;
        return n >= src_n || src[n] == 0;
    }

    // Compares characters as case-sensitive, locale agnostic.
    template<class CharType>
    struct SCharComparatorCaseSensitive
    {
        static bool IsEqual(CharType a, CharType b)
        {
            return a == b;
        }
    };

    // Compares characters as case-insensitive, uses the standard "C" locale.
    template<class CharType>
    struct SCharComparatorCaseInsensitive
    {
        static bool IsEqual(CharType a, CharType b)
        {
            if (a < 0x80 && b < 0x80)
            {
                a = (CharType)CryStringUtils::toLowerAscii(char(a));
                b = (CharType)CryStringUtils::toLowerAscii(char(b));
            }
            return a == b;
        }
    };

    // Template for wildcard matching, UCS code-point aware.
    // Can be used for ASCII and Unicode (UTF-8/UTF-16/UTF-32), but not for ANSI.
    // ? will match exactly one code-point.
    // * will match zero or more code-points.
    template <template<class> class CharComparator, class CharType>
    inline bool MatchesWildcards_Tpl(const CharType* pStr, const CharType* pWild)
    {
        const CharType* savedStr = 0;
        const CharType* savedWild = 0;

        while ((*pStr) && (*pWild != '*'))
        {
            if (!CharComparator<CharType>::IsEqual(*pWild, *pStr) && (*pWild != '?'))
            {
                return false;
            }

            // We need special handling of '?' for Unicode
            if (*pWild == '?' && *pStr > 127)
            {
                Unicode::CIterator<const CharType*> utf(pStr, pStr + 4);
                if (utf.IsAtValidCodepoint())
                {
                    pStr = (++utf).GetPosition();
                    --pStr;
                }
            }

            ++pWild;
            ++pStr;
        }

        while (*pStr)
        {
            if (*pWild == '*')
            {
                if (!*++pWild)
                {
                    return true;
                }
                savedWild = pWild;
                savedStr = pStr + 1;
            }
            else if (CharComparator<CharType>::IsEqual(*pWild, *pStr) || (*pWild == '?'))
            {
                // We need special handling of '?' for Unicode
                if (*pWild == '?' && *pStr > 127)
                {
                    Unicode::CIterator<const CharType*> utf(pStr, pStr + 4);
                    if (utf.IsAtValidCodepoint())
                    {
                        pStr = (++utf).GetPosition();
                        --pStr;
                    }
                }

                ++pWild;
                ++pStr;
            }
            else
            {
                pWild = savedWild;
                pStr = savedStr++;
            }
        }

        while (*pWild == '*')
        {
            ++pWild;
        }

        return *pWild == 0;
    }
} // namespace CryStringUtils_Internal


inline bool cry_strcpy(char* const dst, size_t const dst_size_in_bytes, const char* const src)
{
    return CryStringUtils_Internal::strcpy_with_clamp<char>(dst, dst_size_in_bytes, src, (size_t)-1);
}

inline bool cry_strcpy(char* const dst, size_t const dst_size_in_bytes, const char* const src, size_t const src_size_in_bytes)
{
    return CryStringUtils_Internal::strcpy_with_clamp<char>(dst, dst_size_in_bytes, src, src_size_in_bytes);
}

template <size_t SIZE_IN_CHARS>
inline bool cry_strcpy(char(&dst)[SIZE_IN_CHARS], const char* const src)
{
    return CryStringUtils_Internal::strcpy_with_clamp<char>(dst, SIZE_IN_CHARS, src, (size_t)-1);
}

template <size_t SIZE_IN_CHARS>
inline bool cry_strcpy(char(&dst)[SIZE_IN_CHARS], const char* const src, size_t const src_size_in_bytes)
{
    return CryStringUtils_Internal::strcpy_with_clamp<char>(dst, SIZE_IN_CHARS, src, src_size_in_bytes);
}


inline bool cry_wstrcpy(wchar_t* const dst, size_t const dst_size_in_bytes, const wchar_t* const src)
{
    return CryStringUtils_Internal::strcpy_with_clamp<wchar_t>(dst, dst_size_in_bytes, src, (size_t)-1);
}

inline bool cry_wstrcpy(wchar_t* const dst, size_t const dst_size_in_bytes, const wchar_t* const src, size_t const src_size_in_bytes)
{
    return CryStringUtils_Internal::strcpy_with_clamp<wchar_t>(dst, dst_size_in_bytes, src, src_size_in_bytes);
}

template <size_t SIZE_IN_WCHARS>
inline bool cry_wstrcpy(wchar_t(&dst)[SIZE_IN_WCHARS], const wchar_t* const src)
{
    return CryStringUtils_Internal::strcpy_with_clamp<wchar_t>(dst, SIZE_IN_WCHARS * sizeof(wchar_t), src, (size_t)-1);
}

template <size_t SIZE_IN_WCHARS>
inline bool cry_wstrcpy(wchar_t(&dst)[SIZE_IN_WCHARS], const wchar_t* const src, size_t const src_size_in_bytes)
{
    return CryStringUtils_Internal::strcpy_with_clamp<wchar_t>(dst, SIZE_IN_WCHARS * sizeof(wchar_t), src, src_size_in_bytes);
}


inline bool cry_strcat(char* const dst, size_t const dst_size_in_bytes, const char* const src)
{
    return CryStringUtils_Internal::strcat_with_clamp<char>(dst, dst_size_in_bytes, src, (size_t)-1);
}

inline bool cry_strcat(char* const dst, size_t const dst_size_in_bytes, const char* const src, size_t const src_size_in_bytes)
{
    return CryStringUtils_Internal::strcat_with_clamp<char>(dst, dst_size_in_bytes, src, src_size_in_bytes);
}

template <size_t SIZE_IN_CHARS>
inline bool cry_strcat(char(&dst)[SIZE_IN_CHARS], const char* const src)
{
    return CryStringUtils_Internal::strcat_with_clamp<char>(dst, SIZE_IN_CHARS, src, (size_t)-1);
}

template <size_t SIZE_IN_CHARS>
inline bool cry_strcat(char(&dst)[SIZE_IN_CHARS], const char* const src, size_t const src_size_in_bytes)
{
    return CryStringUtils_Internal::strcat_with_clamp<char>(dst, SIZE_IN_CHARS, src, src_size_in_bytes);
}


inline bool cry_wstrcat(wchar_t* const dst, size_t const dst_size_in_bytes, const wchar_t* const src)
{
    return CryStringUtils_Internal::strcat_with_clamp<wchar_t>(dst, dst_size_in_bytes, src, (size_t)-1);
}

inline bool cry_wstrcat(wchar_t* const dst, size_t const dst_size_in_bytes, const wchar_t* const src, size_t const src_size_in_bytes)
{
    return CryStringUtils_Internal::strcat_with_clamp<wchar_t>(dst, dst_size_in_bytes, src, src_size_in_bytes);
}

template <size_t SIZE_IN_WCHARS>
inline bool cry_wstrcat(wchar_t(&dst)[SIZE_IN_WCHARS], const wchar_t* const src)
{
    return CryStringUtils_Internal::strcat_with_clamp<wchar_t>(dst, SIZE_IN_WCHARS * sizeof(wchar_t), src, (size_t)-1);
}

template <size_t SIZE_IN_WCHARS>
inline bool cry_wstrcat(wchar_t(&dst)[SIZE_IN_WCHARS], const wchar_t* const src, size_t const src_size_in_bytes)
{
    return CryStringUtils_Internal::strcat_with_clamp<wchar_t>(dst, SIZE_IN_WCHARS * sizeof(wchar_t), src, src_size_in_bytes);
}


namespace CryStringUtils
{
    enum
    {
        CRY_DEFAULT_HASH_SEED = 40503, // This is a large 16 bit prime number (perfect for seeding)
        CRY_EMPTY_STR_HASH = 3350499166U, // HashString("")
    };

    // removes the extension from the file path
    // \param szFilePath the source path
    inline char* StripFileExtension(char* szFilePath)
    {
        for (char* p = szFilePath + (int)strlen(szFilePath) - 1; p >= szFilePath; --p)
        {
            switch (*p)
            {
            case ':':
            case '/':
            case '\\':
                // we've reached a path separator - it means there's no extension in this name
                return nullptr;
            case '.':
                // there's an extension in this file name
                *p = '\0';
                return p + 1;
            }
        }
        // it seems the file name is a pure name, without path or extension
        return nullptr;
    }


    // returns the parent directory of the given file or directory.
    // the returned path is WITHOUT the trailing slash
    // if the input path has a trailing slash, it's ignored
    // nGeneration - is the number of parents to scan up
    // Note: A drive specifier (if any) will always be kept (Windows-specific).
    template <class StringCls>
    StringCls GetParentDirectory(const StringCls& strFilePath, int nGeneration = 1)
    {
        for (const char* p = strFilePath.c_str() + strFilePath.length() - 2; // -2 is for the possible trailing slash: there always must be some trailing symbol which is the file/directory name for which we should get the parent
             p >= strFilePath.c_str();
             --p)
        {
            switch (*p)
            {
            case ':':
                return StringCls(strFilePath.c_str(), p);
                break;
            case '/':
            case '\\':
                // we've reached a path separator - return everything before it.
                if (!--nGeneration)
                {
                    return StringCls(strFilePath.c_str(), p);
                }
                break;
            }
        }
        ;
        // the file name is a pure name, without path or extension
        return StringCls();
    }

    /*!
    // converts all chars to lower case
    */
    inline string ToLower(const string& str)
    {
        string temp = str;

#ifndef NOT_USE_CRY_STRING
        temp.MakeLower();
#else
        std::transform(temp.begin(), temp.end(), temp.begin(), toLowerAscii); // STL MakeLower
#endif

        return temp;
    }

    /// Converts the single character to lower case.
    /*!
    \param c source character to convert to lower case if possible
    \return the lower case character equivalent if possible
    */
    inline char ToLower(char c)
    {
        return ((c <= 'Z') && (c >= 'A')) ? c + ('a' - 'A') : c;
    }

    /// Converts the specified character into lowercase.
    /*!
    \param c the character to convert to lowercase
    \return the lowercase character
    */
    // Converts all ASCII characters to upper case.
    // Note: Any non-ASCII characters are left unchanged.
    // This function is ASCII-only and locale agnostic.
    inline string toUpper (const string& str)
    {
        string temp = str;

#ifndef NOT_USE_CRY_STRING
        temp.MakeUpper();
#else
        std::transform(temp.begin(), temp.end(), temp.begin(), toUpperAscii); // STL MakeLower
#endif

        return temp;
    }

    // searches and returns the pointer to the extension of the given file
    /*!
    \param szFileName source filename to search
    // This function is Unicode agnostic and locale agnostic.
    */
    inline const char* FindExtension(const char* szFileName)
    {
        const char* szEnd = szFileName + (int)strlen(szFileName);
        for (const char* p = szEnd - 1; p >= szFileName; --p)
        {
            if (*p == '.')
            {
                return p + 1;
            }
        }

        return szEnd;
    }

    // searches and returns the pointer to the file name in the given file path
    /*!
    \param szFilePath source path to search
    */
    inline const char* FindFileNameInPath(const char* szFilePath)
    {
        for (const char* p = szFilePath + (int)strlen(szFilePath) - 1; p >= szFilePath; --p)
        {
            if (*p == '\\' || *p == '/')
            {
                return p + 1;
            }
        }
        return szFilePath;
    }

    // works like strstr, but is case-insensitive
    /*!
    \param szString the source string
    \param szSubstring the sub-string to look for
    */
    inline const char* stristr(const char* szString, const char* szSubstring)
    {
        int nSuperstringLength = (int)strlen(szString);
        int nSubstringLength = (int)strlen(szSubstring);

        for (int nSubstringPos = 0; nSubstringPos <= nSuperstringLength - nSubstringLength; ++nSubstringPos)
        {
            if (_strnicmp(szString + nSubstringPos, szSubstring, nSubstringLength) == 0)
            {
                return szString + nSubstringPos;
            }
        }
        return nullptr;
    }


#ifndef NOT_USE_CRY_STRING

    /*
    \param strPath the path to "unify"
    */
    inline void UnifyFilePath(stack_string& strPath)
    {
        strPath.replace('\\', '/');
        strPath.MakeLower();
    }

    template<size_t SIZE>
    inline void UnifyFilePath(CryStackStringT<char, SIZE>& strPath)
    {
        strPath.replace('\\', '/');
        strPath.MakeLower();
    }

    /// Replaces backslashes with forward slashes and transforms string to lowercase.
    /*!
    \param strPath the path to "unify"
    */
    inline void UnifyFilePath(string& strPath)
    {
        strPath.replace('\\', '/');
        strPath.MakeLower();
    }

#endif

    // converts the number to a string
    /*
    \param nNumber an unsigned number
    \return the unsigned number as a string
    */
    inline string ToString(unsigned nNumber)
    {
        char szNumber[16];
        sprintf_s(szNumber, "%u", nNumber);
        return szNumber;
    }

    /// Converts the number to a string.
    /*!
    \param nNumber an signed integer
    \return the signed integer as a string
    */
    inline string ToString(signed int nNumber)
    {
        char szNumber[16];
        sprintf_s(szNumber, "%d", nNumber);
        return szNumber;
    }

    /// Converts the floating point number to a string.
    /*!
    \param nNumber a floating point number
    \return the floating point number as a string
    */
    inline string ToString(float nNumber)
    {
        char szNumber[128];
        sprintf_s(szNumber, "%f", nNumber);
        return szNumber;
    }

    /// Converts the boolean value to a string ("0" or "1").
    /*!
    \param nNumber an unsigned number
    \return the bool as a string (either "1" or "0")
    */
    inline string ToString(bool nNumber)
    {
        char szNumber[4];
        sprintf_s(szNumber, "%i", (int)nNumber);
        return szNumber;
    }

#ifdef CRYINCLUDE_CRYCOMMON_CRY_MATRIX44_H
    /// Converts a Matrix44 to a string.
    /*!
    \param m A matrix of type Matrix44
    \return the matrix in the format {0,0,0,0}{0,0,0,0}{0,0,0,0}{0,0,0,0}
    */
    inline string ToString(const Matrix44& m)
    {
        char szBuf[0x200];
        sprintf_s(szBuf, "{%g,%g,%g,%g}{%g,%g,%g,%g}{%g,%g,%g,%g}{%g,%g,%g,%g}",
            m(0, 0), m(0, 1), m(0, 2), m(0, 3),
            m(1, 0), m(1, 1), m(1, 2), m(1, 3),
            m(2, 0), m(2, 1), m(2, 2), m(2, 3),
            m(3, 0), m(3, 1), m(3, 2), m(3, 3));
        return szBuf;
    }
#endif

#ifdef CRYINCLUDE_CRYCOMMON_CRY_QUAT_H
    /// Converts a CryQuat to a string.
    /*!
    \param m A quaternion of type CryQuat
    \return the quaternion in the format {0,{0,0,0,0}}
    */
    inline string ToString (const CryQuat& q)
    {
        char szBuf[0x100];
        sprintf_s(szBuf, "{%g,{%g,%g,%g}}", q.w, q.v.x, q.v.y, q.v.z);
        return szBuf;
    }
#endif

#ifdef CRYINCLUDE_CRYCOMMON_CRY_VECTOR3_H
    /// Converts a Vec3 to a string.
    /*!
    \param m A vector of type Vec3
    \return the vector in the format {0,0,0}
    */
    inline string ToString (const Vec3& v)
    {
        char szBuf[0x80];
        sprintf_s(szBuf, "{%g,%g,%g}", v.x, v.y, v.z);
        return szBuf;
    }
#endif

    /// This function only exists to allow ToString to compile if it is ever used with an unsupported type.
    /*!
    \param unknownType
    \return a string that reads "unknown"
    */
    template<class T>
    inline string ToString(T& unknownType)
    {
        char szValue[8];
        sprintf_s(szValue, "%s", "unknown");
        return szValue;
    }

    // does the same as strstr, but the szString is allowed to be no more than the specified size
    /*!
    \param szString the source string
    \param szSubstring the sub-string to look for
    \param nSuperstringLength the maximum size of szString
    */
    inline const char* strnstr(const char* szString, const char* szSubstring, int nSuperstringLength)
    {
        int nSubstringLength = (int)strlen(szSubstring);
        if (!nSubstringLength)
        {
            return szString;
        }

        for (int nSubstringPos = 0; szString[nSubstringPos] && nSubstringPos < nSuperstringLength - nSubstringLength; ++nSubstringPos)
        {
            if (strncmp(szString + nSubstringPos, szSubstring, nSubstringLength) == 0)
            {
                return szString + nSubstringPos;
            }
        }
        return nullptr;
    }


    // Finds the string in the array of strings.
    // Returns its 0-based index or -1 if not found.
    // Comparison is case-sensitive.
    // The string array end is demarked by the NULL value.
    // This function is Unicode agnostic (but no Unicode collation is performed for equality test) and locale agnostic.
    inline int findString(const char* szString, const char* arrStringList[])
    {
        for (const char** p = arrStringList; *p; ++p)
        {
            if (0 == strcmp(*p, szString))
            {
                return (int)(p - arrStringList);
            }
        }
        return -1; // string was not found
    }

    /// Finds the string in the array of strings.
    /*!
    \param szString the string to look for
    \param arrStringList array of strings
    \remark comparison is case-sensitive
    \remark The string array end is delimited by the nullptr value
    \return its 0-based index or -1 if not found
    */
    inline int FindString(const char* szString, const char* arrStringList[])
    {
        for (const char** p = arrStringList; *p; ++p)
        {
            if (0 == strcmp(*p, szString))
            {
                return (int)(p - arrStringList);
            }
        }
        return -1; // string was not found
    }

    /// Used for printing out sets of objects of string type.
    /*!
    \remark just forms the comma-delimited string where each string in the set is presented as a formatted substring
    \param setStrings set of strings to print
    \return a formatted string
    */
    inline string ToString(const std::set<string>& setStrings)
    {
        string strResult;
        if (!setStrings.empty())
        {
            strResult += "{";
            for (std::set<string>::const_iterator it = setStrings.begin(); it != setStrings.end(); ++it)
            {
                if (it != setStrings.begin())
                {
                    strResult += ", ";
                }
                strResult += "\"";
                strResult += *it;
                strResult += "\"";
            }
            strResult += "}";
        }
        return strResult;
    }

    // Cuts the string and adds leading ... if it's longer than specified maximum length.
    // This function is ASCII-only and locale agnostic.
    inline string cutString(const string& strPath, unsigned nMaxLength)
    {
        if (strPath.length() > nMaxLength && nMaxLength > 3)
        {
            return string("...") + string(strPath.c_str() + strPath.length() - (nMaxLength - 3));
        }
        else
        {
            return strPath;
        }
    }

    /// Cuts the string and adds leading ... if it's longer than specified maximum length.
    /*!
    \param str the string to cut
    \param nMaxLength the allowed length of the string before its cut.
    \return a shortened string starting in ellipses or the source string if it's smaller than nMaxLength
    */
    inline string CutString(const string& str, unsigned nMaxLength)
    {
        if (str.length() > nMaxLength && nMaxLength > 3)
        {
            return string("...") + string(str.c_str() + str.length() - (nMaxLength - 3));
        }
        else
        {
            return str;
        }
    }

    /// Converts the given set of NUMBERS into the string.
    /*!
    \param setMtlms A numeric set to convert into a string
    \param szFormat
    \param szPostfix
    */
    template <typename T>
    string ToString(const std::set<T>& setMtls, const char* szFormat, const char* szPostfix = "")
    {
        string strResult;
        char szBuffer[64];
        if (!setMtls.empty())
        {
            strResult += strResult.empty() ? "(" : " (";
            for (typename std::set<T>::const_iterator it = setMtls.begin(); it != setMtls.end(); )
            {
                if (it != setMtls.begin())
                {
                    strResult += ", ";
                }
                sprintf_s(szBuffer, szFormat, *it);
                strResult += szBuffer;
                T nStart = *it;

                ++it;

                if (it != setMtls.end() && *it == nStart + 1)
                {
                    T nPrev = *it;
                    // we've got a region
                    while (++it != setMtls.end() && *it == nPrev + 1)
                    {
                        nPrev = *it;
                    }
                    if (nPrev == nStart + 1)
                    {
                        // special case - range of length 1
                        strResult += ",";
                    }
                    else
                    {
                        strResult += "..";
                    }
                    sprintf_s(szBuffer, szFormat, nPrev);
                    strResult += szBuffer;
                }
            }
            strResult += ")";
        }
        return szPostfix[0] ? strResult + szPostfix : strResult;
    }


    // Attempts to find a matching wildcard in a string.
    /*!
    \param szString source string
    \param szWildcard the wildcard, supports * and ?
    // returns true if the string matches the wildcard
    // Note: ANSI input is not supported, ASCII is fine since it's a subset of UTF-8.
    */
    inline bool MatchWildcard(const char* szString, const char* szWildcard)
    {
        return CryStringUtils_Internal::MatchesWildcards_Tpl<CryStringUtils_Internal::SCharComparatorCaseSensitive>(szString, szWildcard);
    }

    // Returns true if the string matches the wildcard.
    // Supports wildcard ? (matches one code-point) and * (matches zero or more code-points).
    // This function is Unicode aware and uses the "C" locale for case comparison.
    // Note: ANSI input is not supported, ASCII is fine since it's a subset of UTF-8.
    inline bool MatchWildcardIgnoreCase(const char* szString, const char* szWildcard)
    {
        return CryStringUtils_Internal::MatchesWildcards_Tpl<CryStringUtils_Internal::SCharComparatorCaseInsensitive>(szString, szWildcard);
    }

#if !defined(RESOURCE_COMPILER)

    // calculates a hash value for a given string
    inline uint32 CalculateHash(const char* str)
    {
        return CCrc32::Compute(str);
    }

    // calculates a hash value for the lower case version of a given string
    inline uint32 CalculateHashLowerCase(const char* str)
    {
        return CCrc32::ComputeLowercase(str);
    }

    // This function is Unicode agnostic and locale agnostic.
    inline uint32 HashStringSeed(const char* string, const uint32 seed)
    {
        // A string hash taken from the FRD/Crysis2 (game) code with low probability of clashes
        // Recommend you use the CRY_DEFAULT_HASH_SEED (see HashString)
        const char*     p;
        uint32 hash = seed;
        for (p = string; *p != '\0'; p++)
        {
            hash += *p;
            hash += (hash << 10);
            hash ^= (hash >> 6);
        }
        hash += (hash << 3);
        hash ^= (hash >> 11);
        hash += (hash << 15);
        return hash;
    }

    // This function is ASCII-only and uses the standard "C" locale for case conversion.
    inline uint32 HashStringLowerSeed(const char* string, const uint32 seed)
    {
        // computes the hash of 'string' converted to lower case
        // also see the comment in HashStringSeed
        const char*     p;
        uint32 hash = seed;
        for (p = string; *p != '\0'; p++)
        {
            hash += toLowerAscii(*p);
            hash += (hash << 10);
            hash ^= (hash >> 6);
        }
        hash += (hash << 3);
        hash ^= (hash >> 11);
        hash += (hash << 15);
        return hash;
    }

    // This function is Unicode agnostic and locale agnostic.
    inline uint32 HashString(const char* string)
    {
        return HashStringSeed(string, CRY_DEFAULT_HASH_SEED);
    }

    // This function is ASCII-only and uses the standard "C" locale for case conversion.
    inline uint32 HashStringLower(const char* string)
    {
        return HashStringLowerSeed(string, CRY_DEFAULT_HASH_SEED);
    }
#endif

    // converts all chars to lower case - avoids memory allocation
    /*!
    \param str Reference to string to convert into lowercase.
    */
    inline void ToLowerInplace(string& str)
    {
#ifndef NOT_USE_CRY_STRING
        str.MakeLower();
#else
        std::transform(str.begin(), str.end(), str.begin(), toLowerAscii); // STL MakeLower
#endif
    }

    /// Converts all chars to lower case - avoids memory allocation
    /*!
    \param str Reference to string to convert into lowercase.
    */
    inline void ToLowerInplace(char* str)
    {
        if (str == nullptr)
        {
            return;
        }

        for (char* s = str; *s != '\0'; s++)
        {
            *s = toLowerAscii(*s);
        }
    }


#ifndef NOT_USE_CRY_STRING

    // Converts a wide string (can be UTF-16 or UTF-32 depending on platform) to UTF-8.
    // This function is Unicode aware and locale agnostic.
    /// Converts a wide string to a UTF8 string.
    /*!
    \param str The wide string to convert.
    \return dstr The wide string converted into the specified UTF8 type.
    */
    template <typename T>
    inline void WStrToUTF8(const wchar_t* str, T& dstr)
    {
        string utf8;
        Unicode::Convert(utf8, str);

        // Note: This function expects T to have assign(ptr, len) function
        dstr.assign(utf8.c_str(), utf8.length());
    }

    // Converts a wide string (can be UTF-16 or UTF-32 depending on platform) to UTF-8.
    // This function is Unicode aware and locale agnostic.
    /// Converts a wide string to UTF8.
    /*!
    \param str The wchar_t string to convert.
    \return The UTF8 string.
    */
    inline string WStrToUTF8(const wchar_t* str)
    {
        return Unicode::Convert<string>(str);
    }

    /// Converts a UTF8 string to a wide string.
    /*!
    \param str The UTF8 string to convert.
    \return dstr The UTF8 string converted into the specified type.
    */
    template <typename T>
    inline void UTF8ToWStr(const char* str, T& dstr)
    {
        wstring wide;
        Unicode::Convert(wide, str);

        // Note: This function expects T to have assign(ptr, len) function
        dstr.assign(wide.c_str(), wide.length());
    }

    // Converts an UTF-8 string to wide string (can be UTF-16 or UTF-32 depending on platform).
    // This function is Unicode aware and locale agnostic.
    /*!
    \param str The UTF8 string to convert.
    \return str as converted to wstring.
    */
    inline wstring UTF8ToWStr(const char* str)
    {
        return Unicode::Convert<wstring>(str);
    }

    /// Converts a string to a wide character string
    /*!
    \param str Source string.
    \param dstr Destination wstring.
    */
    inline void StrToWstr(const char* str, wstring& dstr)
    {
        CryStackStringT<wchar_t, 64> tmp;
        tmp.resize(strlen(str));
        tmp.clear();

        while (const wchar_t c = (wchar_t)(*str++))
        {
            tmp.append(1, c);
        }

        dstr.assign(tmp.data(), tmp.length());
    }

#endif // NOT_USE_CRY_STRING

    /// The type used to parse a yes/no string
#if defined(_DISALLOW_ENUM_CLASS)
    enum YesNoType
#else
    enum class YesNoType
#endif
    {
        Yes,
        No,
        Invalid
    };

    // parse the yes/no string
    /*!
    \param szString any of the following strings: yes, enable, true, 1, no, disable, false, 0
    \return YesNoType::Yes if szString is yes/enable/true/1, YesNoType::No if szString is no, disable, false, 0 and YesNoType::Invalid if the string is not one of the expected values.
    */
    inline YesNoType ToYesNoType(const char* szString)
    {
        if (!_stricmp(szString, "yes")
            || !_stricmp(szString, "enable")
            || !_stricmp(szString, "true")
            || !_stricmp(szString, "1"))
#if defined(_DISALLOW_ENUM_CLASS)
        {
            return Yes;
        }
#else
        {
            return YesNoType::Yes;
        }
#endif
        if (!_stricmp(szString, "no")
            || !_stricmp(szString, "disable")
            || !_stricmp(szString, "false")
            || !_stricmp(szString, "0"))
#if defined(_DISALLOW_ENUM_CLASS)
        {
            return No;
        }
#else
        {
            return YesNoType::No;
        }
#endif

#if defined(_DISALLOW_ENUM_CLASS)
        return Invalid;
#else
        return YesNoType::Invalid;
#endif
    }

    /// Verifies if the filename provided only contains the accepted characters.
    /*!
    \param fileName the filename to verify
    \return true if the filename only contains alphanumeric values and/or dot, dash and underscores.
    */
    inline bool IsValidFileName(const char* fileName)
    {
        size_t i = 0;
        for (;; )
        {
            const char c = fileName[i++];
            if (c == 0)
            {
                return true;
            }
            if (!((c >= '0' && c <= '9')
                  || (c >= 'A' && c <= 'Z')
                  || (c >= 'a' && c <= 'z')
                  || c == '.' || c == '-' || c == '_'))
            {
                return false;
            }
        }
    }


    /**************************************************************************
    *void _makepath() - build path name from components
    *
    *Purpose:
    *       create a path name from its individual components
    *
    *Entry:
    *       char *path  - pointer to buffer for constructed path
    *       char *drive - pointer to drive component, may or may not contain trailing ':'
    *       char *dir   - pointer to subdirectory component, may or may not include leading and/or trailing '/' or '\' characters
    *       char *fname - pointer to file base name component
    *       char *ext   - pointer to extension component, may or may not contain a leading '.'.
    *
    *Exit:
    *       path - pointer to constructed path name
    *
    *******************************************************************************/
    ILINE void portable_makepath(char path[_MAX_PATH], const char* drive, const char* dir, const char* fname, const char* ext)
    {
        const char* p;

        /* we assume that the arguments are in the following form (although we
        * do not diagnose invalid arguments or illegal filenames (such as
        * names longer than 8.3 or with illegal characters in them)
        *
        *  drive:
        *      A           ; or
        *      A:
        *  dir:
        *      \top\next\last\     ; or
        *      /top/next/last/     ; or
        *      either of the above forms with either/both the leading
        *      and trailing / or \ removed.  Mixed use of '/' and '\' is
        *      also tolerated
        *  fname:
        *      any valid file name
        *  ext:
        *      any valid extension (none if empty or null )
        */

        /* copy drive */

        if (drive && *drive)
        {
            *path++ = *drive;
            *path++ = (':');
        }

        /* copy dir */

        if ((p = dir) && *p)
        {
            do
            {
                *path++ = *p++;
            } while (*p);
            if (*(p - 1) != '/' && *(p - 1) != ('\\'))
            {
                *path++ = ('\\');
            }
        }

        /* copy fname */

        if (p = fname)
        {
            while (*p)
            {
                *path++ = *p++;
            }
        }

        /* copy ext, including 0-terminator - check to see if a '.' needs
        * to be inserted.
        */

        if (p = ext)
        {
            if (*p && *p != ('.'))
            {
                *path++ = ('.');
            }
            while (*path++ = *p++)
            {
                ;
            }
        }
        else
        {
            /* better add the 0-terminator */
            *path = ('\0');
        }
    }

    /// Create a path name from its individual components.
    /*!
    \param char *path  pointer to buffer for constructed path
    \param char *drive pointer to drive component, may or may not contain trailing ':'
    \param char *dir   pointer to subdirectory component, may or may not include leading and/or trailing '/' or '\' characters
    \param char *fname pointer to file base name component
    \param char *ext   pointer to extension component, may or may not contain a leading '.'.
    \return path pointer to constructed path name
    */
    inline void MakePath(char path[_MAX_PATH], const char* drive, const char* dir, const char* fname, const char* ext)
    {
        const char* p;

        // we assume that the arguments are in the following form (although we
        // do not diagnose invalid arguments or illegal filenames (such as
        // names longer than 8.3 or with illegal characters in them)
        //
        // drive:
        //     A           ; or
        //     A:
        // dir:
        //     \top\next\last\     ; or
        //     /top/next/last/     ; or
        //     either of the above forms with either/both the leading
        //     and trailing / or \ removed.  Mixed use of '/' and '\' is
        //     also tolerated
        // fname:
        //     any valid file name
        // ext:
        //     any valid extension (none if empty or null )
        //

        // copy drive
        if (drive && *drive)
        {
            *path++ = *drive;
            *path++ = (':');
        }

        // copy dir
        if ((p = dir) && *p)
        {
            do
            {
                *path++ = *p++;
            } while (*p);
            if (*(p - 1) != '/' && *(p - 1) != ('\\'))
            {
                *path++ = ('\\');
            }
        }

        // copy fname
        if (p = fname)
        {
            while (*p)
            {
                *path++ = *p++;
            }
        }

        // copy ext, including 0-terminator - check to see if a '.' needs
        // to be inserted.
        if (p = ext)
        {
            if (*p && *p != ('.'))
            {
                *path++ = ('.');
            }
            while (*path++ = *p++)
            {
                ;
            }
        }
        else
        {
            // better add the 0-terminator
            *path = ('\0');
        }
    }

    // Copies characters from a string.
    /*!
    \param destination Pointer to the destination array where the content is to be copied.
    \param source C string to be copied.
    \param num Maximum number of characters to be copied from source.
    \remark Parameter order is the same as strncpy; Copies only up to num characters from source to destination.
    \return true if entirety of source was copied into destination.
    */
    inline bool strncpy(char* destination, const char* source, size_t num)
    {
        bool reply = false;

#if CRY_STRING_ASSERTS && !defined(RESOURCE_COMPILER)
        CRY_ASSERT(destination);
        CRY_ASSERT(source);
#endif

        if (num)
        {
            size_t i;
            for (i = 0; source[i] && (i + 1) < num; ++i)
            {
                destination[i] = source[i];
            }
            destination[i] = '\0';
            reply = (source[i] == '\0');
        }

#if CRY_STRING_ASSERTS && !defined(RESOURCE_COMPILER)
        CRY_ASSERT_MESSAGE(reply, string().Format("String '%s' is too big to fit into a buffer of length %u", source, (unsigned int)num));
#endif
        return reply;
    }

    // Copies wide characters from a wide string.
    /*!
    \param destination Pointer to the destination wchar_t array where the content is to be copied.
    \param source C wide string to be copied.
    \param num Maximum number of characters to be copied from source.
    \remark Parameter order is the same as strncpy; Copies only up to num characters from source to destination.
    \return true if entirety of source was copied into destination.
    */
    inline bool wstrncpy(wchar_t* destination, const wchar_t* source, size_t bufferLength)
    {
        bool reply = false;

#if CRY_STRING_ASSERTS && !defined(RESOURCE_COMPILER)
        CRY_ASSERT(destination);
        CRY_ASSERT(source);
#endif

        if (bufferLength)
        {
            size_t i;
            for (i = 0; source[i] && (i + 1) < bufferLength; ++i)
            {
                destination[i] = source[i];
            }
            destination[i] = '\0';
            reply = (source[i] == '\0');
        }

#if CRY_STRING_ASSERTS && !defined(RESOURCE_COMPILER)
        CRY_ASSERT_MESSAGE(reply, string().Format("String '%ls' is too big to fit into a buffer of length %u", source, (unsigned int)bufferLength));
#endif
        return reply;
    }

    // Copies a C string into a destination buffer up to a specified delimiter or null terminator.
    /*!
    \param destination Pointer to a buffer where the resulting C-string is stored.
    \param source C string to be copied.
    \param num Maximum number of characters to be copied from source.
    \param delimiter Delimiter character up to which the string will be copied.
    \return Number of bytes written into destination (including null terminator) or 0 if delimiter is not found within the first num bytes of source.
    */
    inline size_t CopyStringUntilFindChar(char* destination, const char* source, size_t num, char delimiter)
    {
        size_t reply = 0;

#if CRY_STRING_ASSERTS && !defined(RESOURCE_COMPILER)
        CRY_ASSERT(destination);
        CRY_ASSERT(source);
#endif

        if (num)
        {
            size_t i;
            for (i = 0; source[i] && source[i] != delimiter && (i + 1) < num; ++i)
            {
                destination[i] = source[i];
            }
            destination[i] = '\0';
            reply = (source[i] == delimiter) ? (i + 1) : 0;
        }

        return reply;
    }
} // namespace CryStringUtils

#endif // CRYINCLUDE_CRYCOMMON_STRINGUTILS_H
