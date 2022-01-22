/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/typetraits/typetraits.h>
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

    // models the invocable concept
    template <class F, class... Args>
    /*concept*/ constexpr bool invocable = is_invocable_v<F, Args...>;

    // models the regular_invocable concept
    template <class F, class... Args>
    /*concept*/ constexpr bool regular_invocable = invocable<F, Args...>;
}
