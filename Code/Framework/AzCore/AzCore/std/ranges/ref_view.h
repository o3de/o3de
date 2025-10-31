/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/ranges/ranges.h>
#include <AzCore/std/optional.h>

namespace AZStd::ranges
{
    template<class R, class = void>
    class ref_view;

    template<class R>
    class ref_view<R, enable_if_t<ranges::range<R> && is_object_v<R>>>
        : public ranges::view_interface<ref_view<R>>
    {
    private:
        static void bindable_to_range(R&);
        static void bindable_to_range(R&&) = delete;
    public:
        template<class T, class = enable_if_t<conjunction_v<
            bool_constant<Internal::different_from<T, ref_view>>,
            bool_constant<convertible_to<T, R&>>,
            Internal::sfinae_trigger<decltype(bindable_to_range(declval<T>()))>
            >>>
        constexpr ref_view(T&& t)
            : m_range{ addressof(static_cast<R&>(AZStd::forward<T>(t))) }
        {}

        constexpr R& base() const
        {
            return *m_range;
        }

        constexpr iterator_t<R> begin() const
        {
            return ranges::begin(*m_range);
        }
        constexpr sentinel_t<R> end() const
        {
            return ranges::end(*m_range);
        }

        template<class Rn = R>
        constexpr auto empty() const -> enable_if_t<Internal::sfinae_trigger_v<decltype(ranges::empty(*declval<Rn*>()))>, bool>
        {
            return ranges::empty(*m_range);
        }

        template<class Rn = R>
        constexpr auto size() const -> enable_if_t<sized_range<R>, decltype(ranges::size(*declval<Rn*>()))>
        {
            return ranges::size(*m_range);
        }

        template<class Rn = R>
        constexpr auto data() const -> enable_if_t<contiguous_range<R>, decltype(ranges::data(*declval<Rn*>()))>
        {
            return ranges::data(*m_range);
        }

    private:
        R* m_range{};
    };

    //! deduction guides
    template<class R>
    ref_view(R&) -> ref_view<R>;

    template<class T>
    inline constexpr bool enable_borrowed_range<ref_view<T>> = true;
}
