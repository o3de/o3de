/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

namespace LyShine
{
    //! \brief Returns the number of UTF8 characters in a string.
    //!
    //! AZStd::string::length() counts individual bytes in the string buffer
    //! whereas this function will consider multi-byte chars as one element/
    //! character in the string.
    inline int GetUtf8StringLength(const AZStd::string& utf8String)
    {
        int utf8StrLen = 0;
        Unicode::CIterator<const char*, false> pChar(utf8String.c_str());
        while (uint32_t ch = *pChar)
        {
            ++pChar;
            ++utf8StrLen;
        }

        return utf8StrLen;
    }

    //! \brief Returns the number of bytes of the size of the given multi-byte char.
    inline int GetMultiByteCharSize(const uint32_t multiByteChar)
    {
        // determine the number of bytes that this Unicode code point uses in utf-8 format
        // NOTE: this assumes the uint32_t can be interpreted as a wchar_t, it seems to
        // work for cases tested but may not in general.
        // In the long run it would be better to eliminate
        // this function and use Unicode::CIterator<>::Position instead.
        wchar_t wcharString[2] = { static_cast<wchar_t>(multiByteChar), 0 };
        AZStd::string utf8String(CryStringUtils::WStrToUTF8(wcharString));
        int utf8Length = utf8String.length();
        return utf8Length;
    }

    inline int GetByteLengthOfUtf8Chars(const char* utf8String, int numUtf8Chars)
    {
        int byteStrlen = 0;
        Unicode::CIterator<const char*, false> pChar(utf8String);
        for (int i = 0; i < numUtf8Chars; i++)
        {
            uint32_t ch = *pChar;
            if (!ch)
            {
                break;
            }
            byteStrlen += GetMultiByteCharSize(ch);
            pChar++;
        }

        return byteStrlen;
    }
}
