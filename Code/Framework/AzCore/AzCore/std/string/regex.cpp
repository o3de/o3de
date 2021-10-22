/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/string/regex.h>

namespace AZStd
{
    // CHARACTER CLASS NAMES
    #define AZ_REGEX_CHAR_CLASS_NAME(n, c)          { n, sizeof (n) / sizeof (n[0]) - 1, c }

    template<>
    const ClassNames<char> RegexTraits<char>::m_names[] =
    {   // map class names to numeric constants
        AZ_REGEX_CHAR_CLASS_NAME("alnum", RegexTraits<char>::Ch_alnum),
        AZ_REGEX_CHAR_CLASS_NAME("alpha", RegexTraits<char>::Ch_alpha),
        //AZ_REGEX_CHAR_CLASS_NAME("blank", RegexTraits<char>::Ch_blank),
        AZ_REGEX_CHAR_CLASS_NAME("cntrl", RegexTraits<char>::Ch_cntrl),
        AZ_REGEX_CHAR_CLASS_NAME("d", RegexTraits<char>::Ch_digit),
        AZ_REGEX_CHAR_CLASS_NAME("digit", RegexTraits<char>::Ch_digit),
        AZ_REGEX_CHAR_CLASS_NAME("graph", RegexTraits<char>::Ch_graph),
        AZ_REGEX_CHAR_CLASS_NAME("lower", RegexTraits<char>::Ch_lower),
        AZ_REGEX_CHAR_CLASS_NAME("print", RegexTraits<char>::Ch_print),
        AZ_REGEX_CHAR_CLASS_NAME("punct", RegexTraits<char>::Ch_punct),
        AZ_REGEX_CHAR_CLASS_NAME("space", RegexTraits<char>::Ch_space),
        AZ_REGEX_CHAR_CLASS_NAME("s", RegexTraits<char>::Ch_space),
        AZ_REGEX_CHAR_CLASS_NAME("upper", RegexTraits<char>::Ch_upper),
        AZ_REGEX_CHAR_CLASS_NAME("w", RegexTraits<char>::Ch_invalid),
        AZ_REGEX_CHAR_CLASS_NAME("xdigit", RegexTraits<char>::Ch_xdigit),
        {nullptr, 0, 0},
    };

    template<>
    const ClassNames<wchar_t> RegexTraits<wchar_t>::m_names[] =
    {   // map class names to numeric constants
        AZ_REGEX_CHAR_CLASS_NAME(L"alnum", RegexTraits<wchar_t>::Ch_alnum),
        AZ_REGEX_CHAR_CLASS_NAME(L"alpha", RegexTraits<wchar_t>::Ch_alpha),
        //AZ_REGEX_CHAR_CLASS_NAME(L"blank", RegexTraits<wchar_t>::Ch_blank),
        AZ_REGEX_CHAR_CLASS_NAME(L"cntrl", RegexTraits<wchar_t>::Ch_cntrl),
        AZ_REGEX_CHAR_CLASS_NAME(L"d", RegexTraits<wchar_t>::Ch_digit),
        AZ_REGEX_CHAR_CLASS_NAME(L"digit", RegexTraits<wchar_t>::Ch_digit),
        AZ_REGEX_CHAR_CLASS_NAME(L"graph", RegexTraits<wchar_t>::Ch_graph),
        AZ_REGEX_CHAR_CLASS_NAME(L"lower", RegexTraits<wchar_t>::Ch_lower),
        AZ_REGEX_CHAR_CLASS_NAME(L"print", RegexTraits<wchar_t>::Ch_print),
        AZ_REGEX_CHAR_CLASS_NAME(L"punct", RegexTraits<wchar_t>::Ch_punct),
        AZ_REGEX_CHAR_CLASS_NAME(L"space", RegexTraits<wchar_t>::Ch_space),
        AZ_REGEX_CHAR_CLASS_NAME(L"s", RegexTraits<wchar_t>::Ch_space),
        AZ_REGEX_CHAR_CLASS_NAME(L"upper", RegexTraits<wchar_t>::Ch_upper),
        AZ_REGEX_CHAR_CLASS_NAME(L"w", RegexTraits<char>::Ch_invalid),
        AZ_REGEX_CHAR_CLASS_NAME(L"xdigit", RegexTraits<wchar_t>::Ch_xdigit),
        {nullptr, 0, 0},
    };
    #undef AZ_REGEX_CHAR_CLASS_NAME
} // namespace AZStd
