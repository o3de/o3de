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
    template<ranges::range R>
        requires movable<R>
            && (!Internal::is_initializer_list<remove_cvref_t<R>>)
    class owning_view
        : public ranges::view_interface<owning_view<R>>
    {
    public:

        template<default_initializable T = R>
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
            requires range<const R>
        constexpr auto begin() const -> decltype(ranges::begin(declval<Rn>()))
        {
            return ranges::begin(m_range);
        }
        template<class Rn = R>
            requires range<const R>
        constexpr auto end() const -> decltype(ranges::end(declval<Rn>()))
        {
            return ranges::end(m_range);
        }

        template<class Rn = R>
            requires requires { ranges::empty(declval<Rn>()); }
        constexpr bool empty()
        {
            return ranges::empty(m_range);
        }
        template<class Rn = R>
            requires requires { ranges::empty(declval<Rn>()); }
        constexpr bool empty() const
        {
            return ranges::empty(m_range);
        }

        template<class Rn = R>
            requires sized_range<R>
        constexpr auto size() -> decltype(ranges::size(declval<Rn>()))
        {
            return ranges::size(m_range);
        }
        template<class Rn = R>
            requires sized_range<const R>
        constexpr auto size() const -> decltype(ranges::size(declval<Rn>()))
        {
            return ranges::size(m_range);
        }

        template<class Rn = R>
            requires contiguous_range<R>
        constexpr auto data() -> decltype(ranges::data(declval<Rn>()))
        {
            return ranges::data(m_range);
        }
        template<class Rn = R>
            requires contiguous_range<const R>
        constexpr auto data() const -> decltype(ranges::data(declval<Rn>()))
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
