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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_MAXHELPERS_H
#define CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_MAXHELPERS_H
#pragma once


#include "CompileTimeAssert.h"
#include "PathHelpers.h"
#include "StringHelpers.h"


namespace MaxHelpers
{
    enum
    {
        kBadChar = '_'
    };

#if !defined(MAX_PRODUCT_VERSION_MAJOR)
    #error MAX_PRODUCT_VERSION_MAJOR is undefined
#elif (MAX_PRODUCT_VERSION_MAJOR >= 15)
    COMPILE_TIME_ASSERT(sizeof(MCHAR) == 2);
    #define MAX_MCHAR_SIZE 2
    typedef wstring MaxCompatibleString;
#elif (MAX_PRODUCT_VERSION_MAJOR >= 12)
    COMPILE_TIME_ASSERT(sizeof(MCHAR) == 1);
    #define MAX_MCHAR_SIZE 1
    typedef string MaxCompatibleString;
#else
    #error 3dsMax 2009 and older are not supported anymore
#endif

    inline string CreateAsciiString(const char* s_ansi)
    {
        return StringHelpers::ConvertAnsiToAscii(s_ansi, kBadChar);
    }


    inline string CreateAsciiString(const wchar_t* s_utf16)
    {
        const string s_ansi = StringHelpers::ConvertUtf16ToAnsi(s_utf16, kBadChar);
        return CreateAsciiString(s_ansi.c_str());
    }


    inline string CreateUtf8String(const char* s_ansi)
    {
        return StringHelpers::ConvertAnsiToUtf8(s_ansi);
    }


    inline string CreateUtf8String(const wchar_t* s_utf16)
    {
        return StringHelpers::ConvertUtf16ToUtf8(s_utf16);
    }


    inline string CreateTidyAsciiNodeName(const char* s_ansi)
    {
        const size_t len = strlen(s_ansi);

        string res;
        res.reserve(len);

        for (size_t i = 0; i < len; ++i)
        {
            char c = s_ansi[i];
            if (c < ' ' || c >= 127)
            {
                c = kBadChar;
            }
            res.append(1, c);
        }
        return res;
    }


    inline string CreateTidyAsciiNodeName(const wchar_t* s_utf16)
    {
        const string s_ansi = StringHelpers::ConvertUtf16ToAnsi(s_utf16, kBadChar);
        return CreateTidyAsciiNodeName(s_ansi.c_str());
        ;
    }


    inline MSTR CreateMaxStringFromAscii(const char* s_ascii)
    {
#if (MAX_MCHAR_SIZE == 2)
        return MSTR(StringHelpers::ConvertAsciiToUtf16(s_ascii).c_str());
#else
        return MSTR(s_ascii);
#endif
    }


    inline MaxCompatibleString CreateMaxCompatibleStringFromAscii(const char* s_ascii)
    {
#if (MAX_MCHAR_SIZE == 2)
        return StringHelpers::ConvertAsciiToUtf16(s_ascii);
#else
        return MaxCompatibleString(s_ascii);
#endif
    }


    inline string GetAbsoluteAsciiPath(const char* s_ansi)
    {
        if (!s_ansi || !s_ansi[0])
        {
            return string();
        }
        return PathHelpers::GetAbsoluteAsciiPath(StringHelpers::ConvertAnsiToUtf16(s_ansi).c_str());
    }


    inline string GetAbsoluteAsciiPath(const wchar_t* s_utf16)
    {
        if (!s_utf16 || !s_utf16[0])
        {
            return string();
        }
        return PathHelpers::GetAbsoluteAsciiPath(s_utf16);
    }
}

#endif // CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_MAXHELPERS_H
