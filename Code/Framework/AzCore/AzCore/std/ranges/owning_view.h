/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/ranges/ranges_adaptor.h>

namespace AZStd::ranges
{
    template<class R, class = void>
    class owning_view;

    template<class R>
    class owning_view<R, enable_if_t<ranges::range<R> && movable<R> &&
        !Internal::is_initializer_list<remove_cvref_t<R>> >>
        : public ranges::view_interface<owning_view<R>>
    {
    public:

        template<class T = R, enable_if_t<default_initializable<T>>>
        constexpr owning_view() {}

        constexpr owning_view(R&& t)
            : m_range{ AZStd::move(t) }
        {}

        constexpr owning_view(owning_view&&) = default;
        constexpr owning_view& operator=(owning_view&&) = default;

        constexpr R& base() & noexcept
        {
            return m_range;
        }
        constexpr const R& base() const& noexcept
        {
            return m_range;
        }
        constexpr R&& base() && noexcept
        {
            return AZStd::move(m_range);
        }
        constexpr const R&& base() const&& noexcept
        {
            return AZStd::move(m_range);
        }

        constexpr iterator_t<R> begin()
        {
            return ranges::begin(m_range);
        }
        constexpr sentinel_t<R> end()
        {
            return ranges::end(m_range);
        }

        template<class Rn = R>
        constexpr auto begin() const -> enable_if_t<range<const R>, decltype(ranges::begin(declval<Rn>()))>
        {
            return ranges::begin(m_range);
        }
        template<class Rn = R>
        constexpr auto end() const -> enable_if_t<range<const R>, decltype(ranges::end(declval<Rn>()))>
        {
            return ranges::end(m_range);
        }

        template<class Rn = R>
        constexpr auto empty() -> enable_if_t<Internal::sfinae_trigger_v<decltype(ranges::empty(declval<Rn>()))>, bool>
        {
            return ranges::empty(m_range);
        }
        template<class Rn = R>
        constexpr auto empty() const -> enable_if_t<Internal::sfinae_trigger_v<decltype(ranges::empty(declval<Rn>()))>, bool>
        {
            return ranges::empty(m_range);
        }

        template<class Rn = R>
        constexpr auto size() -> enable_if_t<sized_range<R>, decltype(ranges::size(declval<Rn>()))>
        {
            return ranges::size(m_range);
        }
        template<class Rn = R>
        constexpr auto size() const -> enable_if_t<sized_range<const R>, decltype(ranges::size(declval<Rn>()))>
        {
            return ranges::size(m_range);
        }

        template<class Rn = R>
        constexpr auto data() -> enable_if_t<contiguous_range<R>, decltype(ranges::data(declval<Rn>()))>
        {
            return ranges::data(m_range);
        }
        template<class Rn = R>
        constexpr auto data() const -> enable_if_t<contiguous_range<const R>, decltype(ranges::data(declval<Rn>()))>
        {
            return ranges::data(m_range);
        }

    private:
        R m_range;
    };

    //! deduction guides
    template<class R>
    owning_view(R&&) -> owning_view<R>;

    template<class T>
    inline constexpr bool enable_borrowed_range<owning_view<T>> = enable_borrowed_range<T>;
}
