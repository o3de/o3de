/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/ranges/all_view.h>
#include <AzCore/std/ranges/ranges_algorithm.h>
#include <AzCore/std/tuple.h>

namespace AZStd::ranges
{
    namespace Internal
    {
        template<class... Rs>
        /*concept*/ constexpr bool zip_is_common = (sizeof...(Rs) == 1 && (common_range<Rs> && ...)) ||
            (!(bidirectional_range<Rs> && ...) && (common_range<Rs> && ...)) ||
            ((random_access_range<Rs> && ...) && (sized_range<Rs> && ...));

        template<class... Ts>
        using tuple_or_pair = tuple<Ts...>;

        template<class F, class Tuple>
        constexpr auto tuple_transform(F&& f, Tuple&& tuple)
        {
            auto transform_to_result_tuple = [&](auto&&... elements)
            {
                using tuple_invoke_result = invoke_result_t<F&, decltype(elements)...>;
                return tuple_or_pair<tuple_invoke_result>(AZStd::invoke(f, AZStd::forward<decltype(elements)>(elements))...);
            };
            return apply(AZStd::move(transform_to_result_tuple), AZStd::forward<Tuple>(tuple));
        }

        template<class F, class Tuple>
        constexpr void tuple_for_each(F&& f, Tuple&& tuple)
        {
            auto invoke_tuple_element = [&](auto&&... elements)
            {
                (AZStd::invoke(f, AZStd::forward<decltype(elements)>(elements)), ...);
            };
            apply(AZStd::move(invoke_tuple_element), AZStd::forward<Tuple>(tuple));
        }
    }

    template<class... Views>
    class zip_view
        : public enable_if_t<conjunction_v<
            bool_constant<(sizeof...(Views) > 0)>,
            bool_constant<input_range<Views>>...,
            bool_constant<view<Views>>...
        >,view_interface<zip_view<Views...>>>
    {
        tuple<Views...> m_views; // exposition only

        // [range.zip.iterator], class template zip_view::iterator
        template<bool>
        class iterator;

        // [range.zip.sentinel], class template zip_view::sentinel
        template<bool>
        class sentinel; // exposition only

    public:
        zip_view() = default;
        constexpr explicit zip_view(Views... views)
            : m_views{ AZStd::move(views)... }
        {
        }

        constexpr auto begin() -> enable_if_t<negation_v<
            conjunction<bool_constant<Internal::simple_view<Views>>...>
            >, decltype(iterator<false>(Internal::tuple_transform(ranges::begin, m_views)))>
        {
            return iterator<false>(Internal::tuple_transform(ranges::begin, m_views));
        }
        constexpr auto begin() const -> enable_if_t<conjunction_v<
            bool_constant<range<const Views>>...
            >, decltype(iterator<true>(Internal::tuple_transform(ranges::begin, m_views)))>
        {
            return iterator<true>(Internal::tuple_transform(ranges::begin, m_views));
        }

        constexpr auto end(enable_if_t<negation_v<conjunction<bool_constant<Internal::simple_view<Views>>...>>>* = nullptr)
        {
            if constexpr (!Internal::zip_is_common<Views...>)
            {
                return sentinel<false>(Internal::tuple_transform(ranges::end, m_views));
            }
            else if constexpr ((random_access_range<Views> && ...))
            {
                return begin() + iter_difference_t<iterator<false>>(size());
            }
            else
            {
                return iterator<false>(Internal::tuple_transform(ranges::end, m_views));
            }
        }

        constexpr auto end(enable_if_t<conjunction_v<bool_constant<range<const Views>>...>>* = nullptr) const
        {
            if constexpr (!Internal::zip_is_common<const Views...>)
            {
                return sentinel<true>(Internal::tuple_transform(ranges::end, m_views));
            }
            else if constexpr ((random_access_range<const Views> && ...))
            {
                return begin() + iter_difference_t<iterator<true>>(size());
            }
            else
            {
                return iterator<true>(Internal::tuple_transform(ranges::end, m_views));
            }
        }

        constexpr auto size(enable_if_t<conjunction_v<bool_constant<sized_range<Views>>...>>* = nullptr)
        {
            return apply(
                [](auto... sizes)
                {
                    using CommonType = make_unsigned_t<common_type_t<decltype(sizes)...>>;
                    return ranges::min({ CommonType(sizes)... });
                },
                Internal::tuple_transform(ranges::size, m_views));
        }
        constexpr auto size(enable_if_t<conjunction_v<bool_constant<sized_range<const Views>>...>>* = nullptr) const
        {
            return apply(
                [](auto... sizes)
                {
                    using CommonType = make_unsigned_t<common_type_t<decltype(sizes)...>>;
                    return ranges::min({ CommonType(sizes)... });
                },
                Internal::tuple_transform(ranges::size, m_views));
        }
    };

    template<class... Rs>
    zip_view(Rs&&...) -> zip_view<views::all_t<Rs>...>;

    namespace Internal
    {
        template<bool Const, class... Views>
        /*concept*/ constexpr bool all_random_access = (random_access_range<Internal::maybe_const<Const, Views>>&&...);
        template<bool Const, class... Views>
        /*concept*/ constexpr bool all_bidirectional = (bidirectional_range<Internal::maybe_const<Const, Views>>&&...);
        template<bool Const, class... Views>
        /*concept*/ constexpr bool all_forward = (forward_range<Internal::maybe_const<Const, Views>>&&...);

        struct zip_view_iterator_requires {};
    }

    template<class... Views>
    template<bool Const>
    class zip_view<Views...>::iterator
            : public enable_if_t<conjunction_v<
            bool_constant<(sizeof...(Views) > 0)>,
            bool_constant<input_range<Views>>...,
            bool_constant<view<Views>>...
            >, Internal::zip_view_iterator_requires>
    {
        constexpr explicit iterator(Internal::tuple_or_pair<iterator_t<Internal::maybe_const<Const, Views>>...>);
        // exposition only
    public:
        using iterator_category = input_iterator_tag; // not always present
        using iterator_concept = conditional_t<Internal::all_random_access<Const, Views...>, random_access_iterator_tag,
            conditional_t<Internal::all_bidirectional<Const, Views...>, bidirectional_iterator_tag,
            conditional_t<Internal::all_forward<Const, Views...>, forward_iterator_tag, input_iterator_tag>>>;
        using value_type = Internal::tuple_or_pair<range_value_t<Internal::maybe_const<Const, Views>>...>;
        using difference_type = common_type_t<range_difference_t<Internal::maybe_const<Const, Views>>...>;

        iterator() = default;
        constexpr iterator(iterator<!Const> i, enable_if_t<Const && conjunction_v<
            bool_constant<convertible_to<iterator_t<Views>, iterator_t<Internal::maybe_const<Const, Views>>>>...>>* = nullptr);

        constexpr auto operator*() const;
        constexpr iterator& operator++();
        constexpr void operator++(int);
        constexpr auto operator++(int) -> enable_if_t<Internal::all_forward<Const, Views...>, iterator>;

        constexpr auto operator--() -> enable_if_t<Internal::all_bidirectional<Const, Views...>, iterator&>;
        constexpr auto operator--(int) -> enable_if_t<Internal::all_bidirectional<Const, Views...>, iterator>;

        constexpr auto operator+=(difference_type x) -> enable_if_t<Internal::all_random_access<Const, Views...>, iterator&>;
        constexpr auto operator-=(difference_type x) -> enable_if_t<Internal::all_random_access<Const, Views...>, iterator&>;

        constexpr auto operator[](difference_type n) const ->
            enable_if_t<Internal::all_random_access<Const, Views...>,
            decltype(view_iterator_to_value_tuple(n))>;

        friend constexpr auto operator==(const iterator& x, const iterator& y) ->
            enable_if_t<conjunction_v<bool_constant<equality_comparable<iterator_t<Internal::maybe_const<Const, Views>>>>...>, bool>;

        friend constexpr auto operator<(const iterator& x, const iterator& y) ->
            enable_if_t<Internal::all_random_access<Const, Views...>, bool>;
        friend constexpr auto operator>(const iterator& x, const iterator& y) ->
            enable_if_t<Internal::all_random_access<Const, Views...>, bool>;
        friend constexpr auto operator<=(const iterator& x, const iterator& y) ->
            enable_if_t<Internal::all_random_access<Const, Views...>, bool>;
        friend constexpr auto operator>=(const iterator& x, const iterator& y) ->
            enable_if_t<Internal::all_random_access<Const, Views...>, bool>;
        /* Requires C++20 compiler support
        friend constexpr auto operator<=>(const iterator& x, const iterator& y) requires Internal::all_random_access<Const, Views...> &&
            (three_way_comparable<iterator_t<Internal::maybe_const<Const, Views>>> && ...);
        */

        friend constexpr auto operator+(const iterator& i, difference_type n) ->
            enable_if_t<Internal::all_random_access<Const, Views...>, iterator>;
        friend constexpr auto operator+(difference_type n, const iterator& i) ->
            enable_if_t<Internal::all_random_access<Const, Views...>, iterator>;
        friend constexpr auto operator-(const iterator& i, difference_type n) ->
            enable_if_t<Internal::all_random_access<Const, Views...>, iterator>;
        friend constexpr auto operator-(const iterator& x, const iterator& y) ->
            enable_if_t<conjunction_v<
                bool_constant<sized_sentinel_for<
                    iterator_t<Internal::maybe_const<Const, Views>>,
                    iterator_t<Internal::maybe_const<Const, Views>>>>...
            >, difference_type>;

        friend constexpr auto iter_move(const iterator& i) noexcept(
            noexcept(ranges::iter_move(declval<const iterator_t<Internal::maybe_const<Const, Views>>&>()) && ...) &&
            (is_nothrow_move_constructible_v<range_rvalue_reference_t<Internal::maybe_const<Const, Views>>> && ...));

        friend constexpr auto iter_swap(const iterator& l, const iterator& r) noexcept(
            noexcept(ranges::iter_swap(AZStd::get<0>(l.m_current), AZStd::get<0>(r.m_current))))
            -> enable_if_t<conjunction_v<
            bool_constant<indirectly_swappable<iterator_t<Internal::maybe_const<Const, Views>>>>...>>;

    private:
        auto view_iterator_to_value_tuple(difference_type n)
        {
            auto tranform_iterator_to_value = [&](auto& i)
            {
                using I = decltype(i);
                return i[iter_difference_t<I>(n)];
            };
            Internal::tuple_transform(AZStd::move(tranform_iterator_to_value), m_current);
        };

        Internal::tuple_or_pair<iterator_t<Internal::maybe_const<Const, Views>>...> m_current;
    };

    namespace Internal
    {
        struct zip_view_sentinel_requires {};
    }

    template<class... Views>
    template<bool Const>
    class zip_view<Views...>::sentinel
        : public enable_if_t<conjunction_v<
        bool_constant<(sizeof...(Views) > 0)>,
        bool_constant<input_range<Views>>...,
        bool_constant<view<Views>>...
            >, Internal::zip_view_sentinel_requires>
    {
        constexpr explicit sentinel(Internal::tuple_or_pair<sentinel_t<Internal::maybe_const<Const, Views>>...> end);
    public:
        sentinel() = default;
        constexpr sentinel(sentinel<!Const> i, enable_if_t<Const && conjunction_v<
            bool_constant<convertible_to<sentinel_t<Views>, sentinel_t<Internal::maybe_const<Const, Views>>>>...>>* = nullptr);

        template<bool OtherConst>
        friend constexpr auto operator==(const iterator<OtherConst>& x, const sentinel& y) ->
            enable_if_t<conjunction_v<
            bool_constant<sentinel_for<
                sentinel_t<Internal::maybe_const<Const, Views>>,
                iterator_t<Internal::maybe_const<OtherConst, Views>>>>...
            >,bool>;

        template<bool OtherConst>
        friend constexpr auto operator-(const iterator<OtherConst>& x, const sentinel& y) ->
            enable_if_t<conjunction_v<
            bool_constant<sentinel_for<
            sentinel_t<Internal::maybe_const<Const, Views>>,
            iterator_t<Internal::maybe_const<OtherConst, Views>>>>...
            >, common_type_t<range_difference_t<Internal::maybe_const<OtherConst, Views>>...>>;

        template<bool OtherConst>
        friend constexpr auto operator-(const sentinel& y, const iterator<OtherConst>& x) ->
            enable_if_t<conjunction_v<
            bool_constant<sentinel_for<
            sentinel_t<Internal::maybe_const<Const, Views>>,
            iterator_t<Internal::maybe_const<OtherConst, Views>>>>...
            >, common_type_t<range_difference_t<Internal::maybe_const<OtherConst, Views>>...>>;

    private:
        Internal::tuple_or_pair<sentinel_t<Internal::maybe_const<Const, Views>>...> m_end;
    };
} // namespace AZStd::ranges
