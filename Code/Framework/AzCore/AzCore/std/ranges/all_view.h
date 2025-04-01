/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/ranges/owning_view.h>
#include <AzCore/std/ranges/ref_view.h>

namespace AZStd::ranges::views
{
    namespace Internal
    {
        template<class T, class = void>
        constexpr bool convertible_to_ref_view = false;
        template<class T>
        constexpr bool convertible_to_ref_view<T, void_t<decltype(ranges::ref_view(declval<T>()))>> = true;

        struct all_fn
            : Internal::range_adaptor_closure<all_fn>
        {
            template<class View> 
            constexpr auto operator()(View&& t) const noexcept(noexcept(static_cast<decay_t<View>>(AZStd::forward<View>(t))))
                ->enable_if_t<ranges::view<decay_t<View>>, decltype(static_cast<decay_t<View>>(AZStd::forward<View>(t)))>
            {
                return static_cast<decay_t<View>>(AZStd::forward<View>(t));
            }
            template<class View>
            constexpr auto operator()(View&& t) const noexcept(noexcept(ranges::ref_view(AZStd::forward<View>(t))))
                ->enable_if_t<conjunction_v<
                bool_constant<!ranges::view<decay_t<View>>>,
                bool_constant<convertible_to_ref_view<View>>>, decltype(ranges::ref_view(AZStd::forward<View>(t)))>
            {
                return ranges::ref_view(AZStd::forward<View>(t));
            }
            template<class View>
            constexpr auto operator()(View&& t) const noexcept(noexcept(ranges::owning_view(AZStd::forward<View>(t))))
                ->enable_if_t<conjunction_v<
                bool_constant<!ranges::view<decay_t<View>>>,
                bool_constant<!convertible_to_ref_view<View>>>, decltype(ranges::owning_view(AZStd::forward<View>(t)))>
            {
                return ranges::owning_view(AZStd::forward<View>(t));
            }
        };
    }

    inline namespace customization_point_object
    {
        constexpr Internal::all_fn all{};
    }

    namespace Internal
    {
        template <class R, class = void>
        struct all_view {};
        template <class R>
        struct all_view<R, enable_if_t<ranges::viewable_range<R>>>
        {
            using type = decltype(views::all(declval<R>()));
        };
    }
    template<class R>
    using all_t = typename Internal::all_view<R>::type;
}
