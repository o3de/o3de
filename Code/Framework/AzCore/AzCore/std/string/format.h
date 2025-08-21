/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <format>
#include <utility>

#include <AzCore/std/string/string.h>

namespace AZStd
{
    using std::format_string;
    using std::format_to;
    using std::format_to_n;
    using std::wformat_string;

    template<class... _Types>
    _NODISCARD string format(const format_string<_Types...> _Fmt, _Types&&... _Args)
    {
        string result;
        format_to(std::back_inserter(result), _Fmt, std::forward<_Types>(_Args)...);
        return result;
    }

    template<class... _Types>
    _NODISCARD wstring format(const wformat_string<_Types...> _Fmt, _Types&&... _Args)
    {
        wstring result;
        format_to(std::back_inserter(result), _Fmt, std::forward<_Types>(_Args)...);
        return result;
    }

    template<int n, class... _Types>
    _NODISCARD fixed_string<n> format_fixed_string(const format_string<_Types...> _Fmt, _Types&&... _Args)
    {
        fixed_string<n> result;
        format_to_n(std::back_inserter(result), n, _Fmt, std::forward<_Types>(_Args)...);
        return result;
    }

    template<int n, class... _Types>
    _NODISCARD fixed_wstring<n> format_fixed_string(const wformat_string<_Types...> _Fmt, _Types&&... _Args)
    {
        fixed_wstring<n> result;
        format_to_n(std::back_inserter(result), n, _Fmt, std::forward<_Types>(_Args)...);
        return result;
    }
} // namespace AZStd
