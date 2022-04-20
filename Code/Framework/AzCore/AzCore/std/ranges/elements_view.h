/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/ranges/all_view.h>
#include <AzCore/std/ranges/ranges_adaptor.h>

namespace AZStd::ranges
{
    namespace Internal
    {
        template<class T, size_t N, class = void>
        /*concept*/ constexpr bool has_tuple_element = false;

        template<class T, size_t N>
        /*concept*/ constexpr bool has_tuple_element<T, N, enable_if_t<conjunction_v<
            sfinae_trigger<typename tuple_size<T>::type>,
            bool_constant<(N < tuple_size_v<T>)>,
            sfinae_trigger<tuple_element_t<N, T>>,
            bool_constant<convertible_to<decltype(AZStd::get<N>(declval<T>())), const tuple_element_t<N, T>&>> >
        >> = true;

        template<class T, size_t N, class = void>
        /*concept*/ constexpr bool returnable_element = false;

        template<class T, size_t N>
        /*concept*/ constexpr bool returnable_element<T, N, enable_if_t<
            is_reference_v<T>> > = true;

        template<class T, size_t N>
        /*concept*/ constexpr bool returnable_element<T, N, enable_if_t<
            move_constructible<tuple_element_t<N, T>>> > = true;
    }

    template<class View, size_t N, class = enable_if_t<conjunction_v<
        bool_constant<input_range<View>>,
        bool_constant<view<View>>,
        bool_constant<Internal::has_tuple_element<range_value_t<View>, N>>,
        bool_constant<Internal::has_tuple_element<remove_reference_t<range_reference_t<View>>, N>>,
        bool_constant<Internal::returnable_element<range_reference_t<View>, N>> >
        >>
    class elements_view;

    // Alias for elements_view which is useful for extracting keys from associative containers
    template<class View>
    using keys_view = elements_view<View, 0>;
    // Alias for elements_view which is useful for extracting values from associative containers
    template<class View>
    using values_view = elements_view<View, 1>;

    template<class T, size_t N>
    inline constexpr bool enable_borrowed_range<elements_view<T, N>> = enable_borrowed_range<T>;

    // views::elements customization point
    namespace views
    {
        namespace Internal
        {
            template<size_t N>
            struct elements_fn
                : Internal::range_adaptor_closure<elements_fn<N>>
            {
                template <class View, class = enable_if_t<conjunction_v<
                    bool_constant<viewable_range<View>>
                    >>>
                constexpr auto operator()(View&& view) const
                {
                    return elements_view<views::all_t<View>, N>(AZStd::forward<View>(view));
                }
            };
        }
        inline namespace customization_point_object
        {
            template<size_t N>
            constexpr Internal::elements_fn<N> elements{};

            constexpr auto keys = elements<0>;
            constexpr auto values = elements<1>;
        }
    }

    template<class View, size_t N, class>
    class elements_view
        : public view_interface<elements_view<View, N>>
    {
        template<bool>
        struct iterator;
        template<bool>
        struct sentinel;

    public:
        template <bool Enable = default_initializable<View>,
            class = enable_if_t<Enable>>
        elements_view() {}

        explicit constexpr elements_view(View base)
            : m_base(AZStd::move(base))
        {
        }

        template <bool Enable = copy_constructible<View>, class = enable_if_t<Enable>>
        constexpr View base() const&
        {
            return m_base;
        }
        constexpr View base()&&
        {
            return AZStd::move(m_base);
        }

        template<bool Enable = !Internal::simple_view<View>, class = enable_if_t<Enable>>
        constexpr auto begin()
        {
            return iterator<false>{ ranges::begin(m_base) };
        }

        template<bool Enable = range<const View>, class = enable_if_t<Enable>>
        constexpr auto begin() const
        {
            return iterator<true>{ ranges::begin(m_base) };
        }

        template<bool Enable = !Internal::simple_view<View>, class = enable_if_t<Enable>>
        constexpr auto end()
        {
            if constexpr (!common_range<View>)
            {
                return sentinel<false>{ ranges::end(m_base) };
            }
            else
            {
                return iterator<false>{ ranges::end(m_base) };
            }
        }

        template<bool Enable = range<const View>>
        constexpr auto end() const
        {
            if constexpr (!common_range<const View>)
            {
                return sentinel<true>{ ranges::end(m_base) };
            }
            else
            {
                return iterator<true>{ ranges::end(m_base) };
            }
        }

        template<bool Enable = sized_range<View>, class = enable_if_t<Enable>>
        constexpr auto size()
        {
            return ranges::size(m_base);
        }

        template<bool Enable = sized_range<const View>, class = enable_if_t<Enable>>
        constexpr auto size() const
        {
            return ranges::size(m_base);
        }

    private:
        View m_base{};
    };

    template<class View, size_t N, bool Const, class = void>
    struct elements_view_iterator_category {};

    template<class View, size_t N, bool Const>
    struct elements_view_iterator_category<View, N, Const, enable_if_t<forward_range<Internal::maybe_const<Const, View>> >>
    {
    private:
        using Base = Internal::maybe_const<Const, View>;
        using IterCategory = typename iterator_traits<iterator_t<Base>>::iterator_category;
    public:
        using iterator_category = conditional_t<
            !is_lvalue_reference_v<decltype(AZStd::get<N>(*declval<iterator_t<Base>>()))>,
            input_iterator_tag,
            conditional_t<derived_from<IterCategory, random_access_iterator_tag>,
                random_access_iterator_tag,
                IterCategory>>;
    };

    template<class View, size_t N, class ViewEnable>
    template<bool Const>
    struct elements_view<View, N, ViewEnable>::iterator
        : enable_if_t<conjunction_v<
        bool_constant<input_range<View>>,
        bool_constant<view<View>>,
        bool_constant<Internal::has_tuple_element<range_value_t<View>, N>>,
        bool_constant<Internal::has_tuple_element<remove_reference_t<range_reference_t<View>>, N>>,
        bool_constant<Internal::returnable_element<range_reference_t<View>, N>> >
        , elements_view_iterator_category<View, N, Const>
       >
    {
    private:
        template <bool>
        friend struct sentinel;

        using Base = Internal::maybe_const<Const, View>;
    public:

        using iterator_concept = conditional_t<random_access_range<Base>,
            random_access_iterator_tag,
            conditional_t<bidirectional_range<Base>,
                bidirectional_iterator_tag,
                conditional_t<forward_range<Base>,
                    forward_iterator_tag,
                    input_iterator_tag>>>;

        using value_type = remove_cvref_t<tuple_element_t<N, range_value_t<Base>>>;
        using difference_type = range_difference_t<Base>;

        template<class BaseIter = iterator_t<Base>, class = enable_if_t<default_initializable<BaseIter>>>
        iterator() {}

        constexpr iterator(iterator_t<Base> current)
            : m_current(AZStd::move(current))
        {
        }
        template<class ViewIter = iterator_t<View>, class BaseIter = iterator_t<Base>,
            class = enable_if_t<Const && convertible_to<ViewIter, BaseIter>>>
        iterator(iterator<!Const> i)
            : m_current(i.m_current)
        {
        }

        constexpr iterator_t<View> base() const& noexcept
        {
            return m_current;
        }

        constexpr iterator_t<View> base() &&
        {
            return AZStd::move(m_current);
        }

        constexpr decltype(auto) operator*() const
        {
            return get_element(m_current);
        }

        constexpr iterator& operator++()
        {
            ++m_current;
            return *this;
        }

        constexpr decltype(auto) operator++(int)
        {
            if constexpr (!forward_range<Base>)
            {
                ++m_current;
            }
            else
            {
                auto tmp = *this;
                ++(*this);
                return tmp;
            }
        }

        template<bool Enable = bidirectional_range<Base>, class = enable_if_t<Enable>>
        constexpr iterator& operator--() const
        {
            --m_current;
            return *this;
        }

        template<bool Enable = bidirectional_range<Base>, class = enable_if_t<Enable>>
        constexpr iterator operator--(int) const
        {
            auto tmp = *this;
            --(*this);
            return tmp;
        }

        template<bool Enable = random_access_range<Base>, class = enable_if_t<Enable>>
        constexpr iterator& operator+=(difference_type n)
        {
            m_current += n;
            return *this;
        }

        template<bool Enable = random_access_range<Base>, class = enable_if_t<Enable>>
        constexpr iterator& operator-=(difference_type n)
        {
            m_current -= n;
            return *this;
        }

        template<bool Enable = random_access_range<Base>, class = enable_if_t<Enable>>
        constexpr decltype(auto) operator[](difference_type n) const
        {
            return get_element(m_current + n);
        }

        // equality_comparable
        template<class BaseIter = iterator_t<Base>, class = enable_if_t<equality_comparable<BaseIter>>>
        friend constexpr bool operator==(const iterator& x, const iterator& y)
        {
            return x.m_current == y.m_current;
        }
        friend constexpr bool operator!=(const iterator& y, const iterator& x)
        {
            return !operator==(x, y);
        }

        // strict_weak_order
        template<bool Enable = random_access_range<Base>, class = enable_if_t<Enable>>
        friend constexpr bool operator<(const iterator& x, const iterator& y)
        {
            return x.m_current < y.m_current;
        }
        template<bool Enable = random_access_range<Base>, class = enable_if_t<Enable>>
        friend constexpr bool operator>(const iterator& x, const iterator& y)
        {
            return y < x;
        }
        template<bool Enable = random_access_range<Base>, class = enable_if_t<Enable>>
        friend constexpr bool operator<=(const iterator& x, const iterator& y)
        {
            return !(y < x);
        }
        template<bool Enable = random_access_range<Base>, class = enable_if_t<Enable>>
        friend constexpr bool operator>=(const iterator& x, const iterator& y)
        {
            return !(x < y);
        }

        template<bool Enable = random_access_range<Base>, class = enable_if_t<Enable>>
        friend constexpr iterator operator+(const iterator& x, difference_type n)
        {
            iterator iterCopy(x);
            iterCopy += n;
            return iterCopy;
        }

        template<bool Enable = random_access_range<Base>, class = enable_if_t<Enable>>
        friend constexpr iterator operator+(difference_type n, const iterator& x)
        {
            return n + x;
        }

        template<bool Enable = random_access_range<Base>, class = enable_if_t<Enable>>
        friend constexpr iterator operator-(const iterator& x, difference_type n)
        {
            iterator iterCopy(x);
            iterCopy -= n;
            return iterCopy;
        }

        template<class BaseIter = iterator_t<Base>, class = enable_if_t<sized_sentinel_for<BaseIter, BaseIter>>>
        friend constexpr difference_type operator-(const iterator& x, const iterator& y)
        {
            return x.m_current - y.m_current;
        }
    private:
        static constexpr decltype(auto) get_element(const iterator_t<Base>& i)
        {
            if constexpr (is_reference_v<range_reference_t<Base>>)
            {
                // Return a reference to the element of the tuple like type
                return AZStd::get<N>(*i);
            }
            else
            {
                // Cast the result of calling AZStd::get on value type reference
                using E = remove_cv_t<tuple_element_t<N, range_reference_t<Base>>>;
                return static_cast<E>(AZStd::get<N>(*i));
            }
        }

        //! iterator to range being viewed
        iterator_t<Base> m_current{};
    };

    namespace ElementsViewInternal
    {
        struct requirements_fulfilled {};
    }

    template<class View, size_t N, class ViewEnable>
    template<bool Const>
    struct elements_view<View, N, ViewEnable>::sentinel
        : enable_if_t<conjunction_v<
        bool_constant<input_range<View>>,
        bool_constant<view<View>>,
        bool_constant<Internal::has_tuple_element<range_value_t<View>, N>>,
        bool_constant<Internal::has_tuple_element<remove_reference_t<range_reference_t<View>>, N>>,
        bool_constant<Internal::returnable_element<range_reference_t<View>, N>> >
        , ElementsViewInternal::requirements_fulfilled>
    {
    private:
        using Base = Internal::maybe_const<Const, View>;

    public:
        sentinel() = default;

        explicit constexpr sentinel(sentinel_t<View> end)
            : m_end(end)
        {
        }
        template<class SentinelView = sentinel_t<View>, class SentinelBase = sentinel_t<Base>,
            class = enable_if_t<Const && convertible_to<SentinelView, SentinelBase>>>
        constexpr sentinel(sentinel<!Const> s)
            : m_end(AZStd::move(s.m_end))
        {
        }

        constexpr sentinel_t<View> base() const
        {
            return m_end;
        }

        // comparison operators
        template<bool OtherConst, class = enable_if_t<
            sentinel_for<sentinel_t<Base>, iterator_t<Internal::maybe_const<OtherConst, Base>>>
        >>
        friend constexpr bool operator==(const iterator<OtherConst>& x, const sentinel& y)
        {
            return iterator_accessor(x) == y.m_end;
        }
        template<bool OtherConst>
        friend constexpr bool operator==(const sentinel& y, const iterator<OtherConst>& x)
        {
            return operator==(x, y);
        }
        template<bool OtherConst>
        friend constexpr bool operator!=(const iterator<OtherConst>& x, const sentinel& y)
        {
            return !operator==(x, y);
        }
        template<bool OtherConst>
        friend constexpr bool operator!=(const sentinel& y, const iterator<OtherConst>& x)
        {
            return !operator==(x, y);
        }

        // difference operator
        template<bool OtherConst, class = enable_if_t<
            sized_sentinel_for<sentinel_t<Base>, iterator_t<Internal::maybe_const<OtherConst, Base>>>
            >>
        friend constexpr range_difference_t<Internal::maybe_const<OtherConst, Base >>
            operator-(const iterator<OtherConst>& x, const sentinel& y)
        {
            return iterator_accessor(x) - y.m_end;
        }

        template<bool OtherConst, class = enable_if_t<
            sized_sentinel_for<sentinel_t<Base>, iterator_t<Internal::maybe_const<OtherConst, Base>>>
            >>
        friend constexpr range_difference_t<Internal::maybe_const<OtherConst, Base>>
            operator-(const sentinel& x, const iterator<OtherConst>& y)
        {
            return x.m_end - iterator_accessor(y);
        }
    private:
        // On MSVC The friend functions are can only access the sentinel struct members
        // The iterator struct which is a friend of the sentinel struct is NOT a friend
        // of the friend functions
        // So a shim is added to provide access to the iterator m_current member
        template<bool OtherConst>
        static constexpr auto iterator_accessor(const iterator<OtherConst>& it)
        {
            return it.m_current;
        }

        sentinel_t<Base> m_end{};
    };
}
