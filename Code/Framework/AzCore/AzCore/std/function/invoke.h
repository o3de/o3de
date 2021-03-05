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

#pragma once

#include <AzCore/std/typetraits/invoke_traits.h>

namespace AZStd
{
    template <class Fn, class... ArgTypes>
    struct is_invocable : Internal::invocable<Fn, ArgTypes...>::type {};

    template <class R, class Fn, class... ArgTypes>
    struct is_invocable_r : Internal::invocable_r<R, Fn, ArgTypes...>::type {};

    template <class Fn, class... ArgTypes>
    constexpr bool is_invocable_v = is_invocable<Fn, ArgTypes...>::value;

    template <class R, class Fn, class ...ArgTypes>
    constexpr bool is_invocable_r_v = is_invocable_r<R, Fn, ArgTypes...>::value;

    template<class Fn, class... ArgTypes>
    struct invoke_result
        : AZStd::enable_if<Internal::invocable<Fn, ArgTypes...>::value, typename Internal::invocable<Fn, ArgTypes...>::result_type>
    {};

    template<class Fn, class... ArgTypes>
    using invoke_result_t = typename invoke_result<Fn, ArgTypes...>::type;

    template <class F, class... Args>
    inline constexpr invoke_result_t<F, Args...> invoke(F&& f, Args&&... args)
    {
        return Internal::INVOKE(Internal::InvokeTraits::forward<F>(f), Internal::InvokeTraits::forward<Args>(args)...);
    }
}
