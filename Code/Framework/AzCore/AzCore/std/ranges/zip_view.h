/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/ranges/all_view.h>
#include <AzCore/std/ranges/empty_view.h>
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
            auto TransformToResultTuple = [&](auto&&... elements)
            {
                using tuple_invoke_result = invoke_result_t<F&, decltype(elements)...>;
                return tuple_or_pair<tuple_invoke_result>(AZStd::invoke(f, AZStd::forward<decltype(elements)>(elements))...);
            };
            return AZStd::apply(AZStd::move(TransformToResultTuple), AZStd::forward<Tuple>(tuple));
        }

        template<class F, class Tuple>
        constexpr void tuple_for_each(F&& f, Tuple&& tuple)
        {
            auto InvokeTupleElement = [&](auto&&... elements)
            {
                (AZStd::invoke(f, AZStd::forward<decltype(elements)>(elements)), ...);
            };
            AZStd::apply(AZStd::move(InvokeTupleElement), AZStd::forward<Tuple>(tuple));
        }

        template<class F, class... Tuples, size_t... Indices>
        constexpr decltype(auto) tuple_zip(F&& f, AZStd::index_sequence<Indices...>, Tuples&&... tuples)
        {
            AZStd::invoke(AZStd::forward<F>(f), AZStd::get<Indices>(AZStd::forward<Tuples>(tuples))...);
        }
    }

    template<class... Views>
    class zip_view;

    // views::zip customization point
    namespace views
    {
        namespace Internal
        {
            struct zip_view_fn
            {
                template <class... Views>
                constexpr auto operator()(Views&&... views)
                    -> enable_if_t<ranges::view<Views...>,
                    decltype(create_view(AZStd::forward<Views>(views)...))>
                {
                    return create_view(AZStd::forward<Views>(views)...);
                }
            private:
                template <class... Views>
                constexpr decltype(auto) create_view(Views&&... views)
                {
                    if constexpr (sizeof...(Views))
                    {
                        return views::empty<tuple<>>;
                    }
                    else
                    {
                        return zip_view<views::all_t<Views...>>(AZStd::forward<Views>(views)...);
                    }
                }

            };
        }
        inline namespace customization_point_object
        {
            constexpr Internal::zip_view_fn zip{};
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
        // [range.zip.iterator], class template zip_view::iterator
        template<bool>
        class iterator;

        // [range.zip.sentinel], class template zip_view::sentinel
        template<bool>
        class sentinel; // exposition only

    public:
        zip_view() = default;
        constexpr explicit zip_view(Views... views);

        template<bool Enable = !conjunction_v<bool_constant<Internal::simple_view<Views>>...>, class = enable_if_t<Enable>>
        constexpr auto begin();
        template<bool Enable = conjunction_v<bool_constant<range<const Views>>...>, class = enable_if_t<Enable>>
        constexpr auto begin() const;

        template<bool Enable = !conjunction_v<bool_constant<Internal::simple_view<Views>>...>, class = enable_if_t<Enable>>
        constexpr auto end();
        template<bool Enable = conjunction_v<bool_constant<range<const Views>>...>, class = enable_if_t<Enable>>
        constexpr auto end() const;

        template<bool Enable = conjunction_v<bool_constant<sized_range<Views>>...>, class = enable_if_t<Enable>>
        constexpr auto size();
        template<bool Enable = conjunction_v<bool_constant<sized_range<const Views>>...>, class = enable_if_t<Enable>>
        constexpr auto size() const;

    private:
        tuple<Views...> m_views;
    };

    template<class... Rs>
    zip_view(Rs&&...) -> zip_view<views::all_t<Rs>...>;

    template<class... Views>
    inline constexpr bool enable_borrowed_range<zip_view<Views...>> = (enable_borrowed_range<Views> && ...);


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
        : public enable_if_t<(conjunction_v<
            bool_constant<(sizeof...(Views) > 0)>,
            bool_constant<input_range<Views>>...,
            bool_constant<view<Views>>...>),
        Internal::zip_view_iterator_requires>
    {
        friend class zip_view<Views...>;
        constexpr explicit iterator(Internal::tuple_or_pair<iterator_t<Internal::maybe_const<Const, Views>>...>);
    public:
        using iterator_category = input_iterator_tag; // not always present
        using iterator_concept = conditional_t<Internal::all_random_access<Const, Views...>, random_access_iterator_tag,
            conditional_t<Internal::all_bidirectional<Const, Views...>, bidirectional_iterator_tag,
            conditional_t<Internal::all_forward<Const, Views...>, forward_iterator_tag, input_iterator_tag>>>;
        using value_type = Internal::tuple_or_pair<range_value_t<Internal::maybe_const<Const, Views>>...>;
        using difference_type = common_type_t<range_difference_t<Internal::maybe_const<Const, Views>>...>;

    private:
        constexpr auto view_iterator_to_value_tuple(difference_type n) const;
        template<size_t... Indices>
        static constexpr auto any_iterator_equal(const iterator& x, const iterator& y, AZStd::index_sequence<Indices...>);
        template<size_t... Indices>
        static constexpr auto min_distance_in_views(const iterator& x, const iterator& y, AZStd::index_sequence<Indices...>);
        template<size_t... Indices>
        static constexpr auto iter_swap_index(const iterator& l, const iterator& r, AZStd::index_sequence<Indices...>);
    public:

        iterator() = default;
        template<bool Enable = Const && conjunction_v<
            bool_constant<convertible_to<iterator_t<Views>, iterator_t<Internal::maybe_const<Const, Views>>>>...>,
            class = enable_if_t<Enable>>
        constexpr iterator(iterator<!Const> other);

        constexpr auto operator*() const;
        constexpr iterator& operator++();
        constexpr decltype(auto) operator++(int);

        template<bool Enable = Internal::all_bidirectional<Const, Views...>, class = enable_if_t<Enable>>
        constexpr auto operator--() -> iterator&;
        template<bool Enable = Internal::all_bidirectional<Const, Views...>, class = enable_if_t<Enable>>
        constexpr auto operator--(int) -> iterator;

        template<bool Enable = Internal::all_random_access<Const, Views...>, class = enable_if_t<Enable>>
        constexpr auto operator+=(difference_type x) -> iterator&;
        template<bool Enable = Internal::all_random_access<Const, Views...>, class = enable_if_t<Enable>>
        constexpr auto operator-=(difference_type x) -> iterator&;

        template<bool Enable = Internal::all_random_access<Const, Views...>, class = enable_if_t<Enable>>
        constexpr auto operator[](difference_type n) const;

        // Out-of-line defintions with these friend functions are possible, but quite a mess signature wise
        // that is why the defintions are inline, instead of
        // template<bool Const, class... Views>
        // constexpr auto operator==(const typename zip_views<Views...>::template iterator<Const>& x,
        //     const typename zip_views<Views...>::template iterator<Const>& y){}
        template<bool Enable = conjunction_v<bool_constant<equality_comparable<iterator_t<Internal::maybe_const<Const, Views>>>>...>,
            class = enable_if_t<Enable>>
        friend constexpr auto operator==(const iterator& x, const iterator& y) -> bool
        {
            if constexpr (Internal::all_bidirectional<Const, Views...>)
            {
                return x.m_current == y.m_current;
            }
            else
            {
                // Returns true if it at least one of the iterator views are equal
                return any_iterator_equal(x, y, AZStd::index_sequence_for<Views...>{});
            }
        }

        template<bool Enable = Internal::all_random_access<Const, Views...>, class = enable_if_t<Enable>>
        friend constexpr auto operator<(const iterator& x, const iterator& y) -> bool
        {
            return x.m_current < y.m_current;
        }
        template<bool Enable = Internal::all_random_access<Const, Views...>, class = enable_if_t<Enable>>
        friend constexpr auto operator>(const iterator& x, const iterator& y) -> bool
        {
            return y.m_current < x.m_current;
        }
        template<bool Enable = Internal::all_random_access<Const, Views...>, class = enable_if_t<Enable>>
        friend constexpr auto operator<=(const iterator& x, const iterator& y) -> bool
        {
            return !(y.m_current < x.m_current);
        }
        template<bool Enable = Internal::all_random_access<Const, Views...>, class = enable_if_t<Enable>>
        friend constexpr auto operator>=(const iterator& x, const iterator& y) -> bool
        {
            return !(x.m_current < y.m_current);
        }
        /* Requires C++20 compiler support
        friend constexpr auto operator<=>(const iterator& x, const iterator& y) requires Internal::all_random_access<Const, Views...> &&
            (three_way_comparable<iterator_t<Internal::maybe_const<Const, Views>>> && ...);
        */

        template<bool Enable = Internal::all_random_access<Const, Views...>, class = enable_if_t<Enable>>
        friend constexpr auto operator+(const iterator& i, difference_type n) -> iterator
        {
            auto r = i;
            r += n;
            return r;
        }
        template<bool Enable = Internal::all_random_access<Const, Views...>, class = enable_if_t<Enable>>
        friend constexpr auto operator+(difference_type n, const iterator& i) -> iterator
        {
            auto r = i;
            r += n;
            return r;
        }
        template<bool Enable = Internal::all_random_access<Const, Views...>, class = enable_if_t<Enable>>
        friend constexpr auto operator-(const iterator& i, difference_type n) -> iterator
        {
            auto r = i;
            r -= n;
            return r;
        }
        template<bool Enable = conjunction_v<
            bool_constant<sized_sentinel_for<
            iterator_t<Internal::maybe_const<Const, Views>>,
            iterator_t<Internal::maybe_const<Const, Views>>>>...
            >, class = enable_if_t<Enable>>
        friend constexpr auto operator-(const iterator& x, const iterator& y) -> difference_type
        {
            return min_distance_in_views(x, y, AZStd::index_sequence_for<Views...>{});
        }

        friend constexpr auto iter_move(const iterator& i) noexcept(
            noexcept((ranges::iter_move(declval<const iterator_t<Internal::maybe_const<Const, Views>>&>()) && ...)) &&
            (is_nothrow_move_constructible_v<range_rvalue_reference_t<Internal::maybe_const<Const, Views>>> && ...))
        {
            return tuple_transform(ranges::iter_move, i.m_current);
        }

        template<bool Enable =
            conjunction_v<bool_constant<indirectly_swappable<iterator_t<Internal::maybe_const<Const, Views>>>>...>,
            class = enable_if_t<Enable>>
        friend constexpr void iter_swap(const iterator& l, const iterator& r) noexcept(
            noexcept(iter_swap_index(l.m_current, r.m_current, AZStd::index_sequence_for<Views...>{})))
        {
            iter_swap_with_index(l, r, AZStd::index_sequence_for<Views...>{});
        }

    private:
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
        friend class zip_view<Views...>;
        constexpr explicit sentinel(Internal::tuple_or_pair<sentinel_t<Internal::maybe_const<Const, Views>>...> end);

        template<bool OtherConst, size_t... Indices>
        static constexpr auto min_distance_between_view_iterators(const iterator<OtherConst>& x,
            const sentinel& y, AZStd::index_sequence<Indices...>) ->
            common_type_t<range_difference_t<Internal::maybe_const<OtherConst, Views>>...>;
    public:
        sentinel() = default;
        template<bool Enable = Const && conjunction_v<
            bool_constant<convertible_to<sentinel_t<Views>, sentinel_t<Internal::maybe_const<Const, Views>>>>...>,
            class = enable_if_t<Enable>>
        constexpr sentinel(sentinel<!Const> i);

        template<bool OtherConst, class = enable_if_t<conjunction_v<
            bool_constant<sentinel_for<
            sentinel_t<Internal::maybe_const<Const, Views>>,
            iterator_t<Internal::maybe_const<OtherConst, Views>>>>...>
            >>
        friend constexpr auto operator==(const iterator<OtherConst>& x, const sentinel& y) -> bool
        {
            // Tracks if any iterator of the Views is equal to a sentinel of the views
            bool anyIteratorEqual = false;
            auto CompareIterator = [&anyIteratorEqual](auto&& lhs, auto&& rhs)
            {
                anyIteratorEqual = anyIteratorEqual || AZStd::forward<decltype(lhs)>(lhs) == AZStd::forward<decltype(rhs)>(rhs);
            };
            Internal::tuple_zip(AZStd::move(CompareIterator), x.m_current, y.m_end);

            return anyIteratorEqual;
        }

        template<bool OtherConst, class = enable_if_t<conjunction_v<
            bool_constant<sentinel_for<
            sentinel_t<Internal::maybe_const<Const, Views>>,
            iterator_t<Internal::maybe_const<OtherConst, Views>>>>...>
            >>
        friend constexpr auto operator-(const iterator<OtherConst>& x, const sentinel& y) ->
            common_type_t<range_difference_t<Internal::maybe_const<OtherConst, Views>>...>
        {
            return min_distance_between_view_iterators(x, y, AZStd::index_sequence_for<Views...>{});
        }

        template<bool OtherConst, class = enable_if_t<conjunction_v<
            bool_constant<sentinel_for<
            sentinel_t<Internal::maybe_const<Const, Views>>,
            iterator_t<Internal::maybe_const<OtherConst, Views>>>>...>
            >>
        friend constexpr auto operator-(const sentinel& y, const iterator<OtherConst>& x) ->
            common_type_t<range_difference_t<Internal::maybe_const<OtherConst, Views>>...>
        {
            return -(x - y);
        }

    private:
        Internal::tuple_or_pair<sentinel_t<Internal::maybe_const<Const, Views>>...> m_end;
    };
} // namespace AZStd::ranges

#include <AzCore/std/ranges/zip_view.inl>
