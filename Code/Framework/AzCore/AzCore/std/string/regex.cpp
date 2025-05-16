/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/string/regex_impl.h>

namespace AZStd
{
#define AZ_REGEX_CHAR_CLASS_NAME(n, c) { n, sizeof(n) / sizeof(n[0]) - 1, c }

    template<>
    const ClassNames<char> RegexTraits<char>::m_names[] = {
        // map class names to numeric constants
        AZ_REGEX_CHAR_CLASS_NAME("alnum", RegexTraitsBase::Ch_alnum),
        AZ_REGEX_CHAR_CLASS_NAME("alpha", RegexTraitsBase::Ch_alpha),
        // AZ_REGEX_CHAR_CLASS_NAME("blank", RegexTraits<char>::Ch_blank),
        AZ_REGEX_CHAR_CLASS_NAME("cntrl", RegexTraitsBase::Ch_cntrl),
        AZ_REGEX_CHAR_CLASS_NAME("d", RegexTraitsBase::Ch_digit),
        AZ_REGEX_CHAR_CLASS_NAME("digit", RegexTraitsBase::Ch_digit),
        AZ_REGEX_CHAR_CLASS_NAME("graph", RegexTraitsBase::Ch_graph),
        AZ_REGEX_CHAR_CLASS_NAME("lower", RegexTraitsBase::Ch_lower),
        AZ_REGEX_CHAR_CLASS_NAME("print", RegexTraitsBase::Ch_print),
        AZ_REGEX_CHAR_CLASS_NAME("punct", RegexTraitsBase::Ch_punct),
        AZ_REGEX_CHAR_CLASS_NAME("space", RegexTraitsBase::Ch_space),
        AZ_REGEX_CHAR_CLASS_NAME("s", RegexTraitsBase::Ch_space),
        AZ_REGEX_CHAR_CLASS_NAME("upper", RegexTraitsBase::Ch_upper),
        AZ_REGEX_CHAR_CLASS_NAME("w", RegexTraitsBase::Ch_invalid),
        AZ_REGEX_CHAR_CLASS_NAME("xdigit", RegexTraitsBase::Ch_xdigit),
        { nullptr, 0, 0 },
    };

    template<>
    const ClassNames<wchar_t> RegexTraits<wchar_t>::m_names[] = {
        // map class names to numeric constants
        AZ_REGEX_CHAR_CLASS_NAME(L"alnum", RegexTraitsBase::Ch_alnum),
        AZ_REGEX_CHAR_CLASS_NAME(L"alpha", RegexTraitsBase::Ch_alpha),
        // AZ_REGEX_CHAR_CLASS_NAME(L"blank", RegexTraits<wchar_t>::Ch_blank),
        AZ_REGEX_CHAR_CLASS_NAME(L"cntrl", RegexTraitsBase::Ch_cntrl),
        AZ_REGEX_CHAR_CLASS_NAME(L"d", RegexTraitsBase::Ch_digit),
        AZ_REGEX_CHAR_CLASS_NAME(L"digit", RegexTraitsBase::Ch_digit),
        AZ_REGEX_CHAR_CLASS_NAME(L"graph", RegexTraitsBase::Ch_graph),
        AZ_REGEX_CHAR_CLASS_NAME(L"lower", RegexTraitsBase::Ch_lower),
        AZ_REGEX_CHAR_CLASS_NAME(L"print", RegexTraitsBase::Ch_print),
        AZ_REGEX_CHAR_CLASS_NAME(L"punct", RegexTraitsBase::Ch_punct),
        AZ_REGEX_CHAR_CLASS_NAME(L"space", RegexTraitsBase::Ch_space),
        AZ_REGEX_CHAR_CLASS_NAME(L"s", RegexTraitsBase::Ch_space),
        AZ_REGEX_CHAR_CLASS_NAME(L"upper", RegexTraitsBase::Ch_upper),
        AZ_REGEX_CHAR_CLASS_NAME(L"w", RegexTraitsBase::Ch_invalid),
        AZ_REGEX_CHAR_CLASS_NAME(L"xdigit", RegexTraitsBase::Ch_xdigit),
        { nullptr, 0, 0 },
    };
#undef AZ_REGEX_CHAR_CLASS_NAME

    template struct AZCORE_API_EXPORT ClassNames<char>;
    template struct AZCORE_API_EXPORT ClassNames<wchar_t>;
    template class AZCORE_API_EXPORT RegexTraits<char>;
    template class AZCORE_API_EXPORT RegexTraits<wchar_t>;
} // namespace AZStd
