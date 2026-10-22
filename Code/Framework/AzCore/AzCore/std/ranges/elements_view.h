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
#include <AzCore/std/typetraits/is_reference.h>

namespace AZStd::ranges
{
    namespace Internal
    {
        template<class T, size_t N>
        concept has_tuple_element =
            requires
            {
                typename tuple_size<T>::type;
                typename tuple_element_t<N, T>;
            }
            && (N < tuple_size_v<T>)
            && convertible_to<decltype(AZStd::get<N>(declval<T>())), const tuple_element_t<N, T>&>;

        template<class T, size_t N>
        concept returnable_element =
            is_reference_v<T>
            || move_constructible<tuple_element_t<N, T>>;
    }

    template<input_range View, size_t N>
        requires view<View>
            && Internal::has_tuple_element<range_value_t<View>, N>
            && Internal::has_tuple_element<remove_reference_t<range_reference_t<View>>, N>
            && Internal::returnable_element<range_reference_t<View>, N>
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
                template <viewable_range View>
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

    template<input_range View, size_t N>
        requires view<View>
            && Internal::has_tuple_element<range_value_t<View>, N>
            && Internal::has_tuple_element<remove_reference_t<range_reference_t<View>>, N>
            && Internal::returnable_element<range_reference_t<View>, N>
    class elements_view
        : public view_interface<elements_view<View, N>>
    {
        template<bool>
        struct iterator;
        template<bool>
        struct sentinel;

    public:
        elements_view()
            requires default_initializable<View>
        {}

        explicit constexpr elements_view(View base)
            : m_base(AZStd::move(base))
        {
        }

        constexpr View base() const&
            requires copy_constructible<View>
        {
            return m_base;
        }
        constexpr View base()&&
        {
            return AZStd::move(m_base);
        }

        constexpr auto begin()
            requires (!Internal::simple_view<View>)
        {
            return iterator<false>{ ranges::begin(m_base) };
        }

        constexpr auto begin() const
            requires range<const View>
        {
            return iterator<true>{ ranges::begin(m_base) };
        }

        constexpr auto end()
            requires (!Internal::simple_view<View>)
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

        constexpr auto end() const
            requires range<const View>
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

        constexpr auto size()
            requires sized_range<View>
        {
            return ranges::size(m_base);
        }

        constexpr auto size() const
            requires sized_range<const View>
        {
            return ranges::size(m_base);
        }

    private:
        View m_base{};
    };

    template<class View, size_t N, bool Const>
    struct elements_view_iterator_category {};

    template<class View, size_t N, bool Const>
        requires forward_range<Internal::maybe_const<Const, View>>
    struct elements_view_iterator_category<View, N, Const>
    {
    private:
        // Use a "function" to check the type traits of the join view iterators
        // and return an instance of the correct tag type
        // The function will only be used in the unevaluated context of decltype
        // to determine the type.
        // It is a form of template metaprogramming which uses actual code
        // to create an instance of a type and then uses decltype to extract the type
        static constexpr auto get_iterator_category()
        {
            using Base = Internal::maybe_const<Const, View>;
            using IterCategory = typename iterator_traits<iterator_t<Base>>::iterator_category;

            if constexpr (!is_lvalue_reference_v<decltype(AZStd::get<N>(*declval<iterator_t<Base>>()))>)
            {
                return input_iterator_tag{};
            }
            else if constexpr (derived_from<IterCategory, random_access_iterator_tag>)
            {
                return random_access_iterator_tag{};
            }
            else
            {
                return IterCategory{};
            }
        }
    public:
        using iterator_category = decltype(get_iterator_category());
    };

    template<input_range View, size_t N>
        requires view<View>
            && Internal::has_tuple_element<range_value_t<View>, N>
            && Internal::has_tuple_element<remove_reference_t<range_reference_t<View>>, N>
            && Internal::returnable_element<range_reference_t<View>, N>
    template<bool Const>
    struct elements_view<View, N>::iterator
        : elements_view_iterator_category<View, N, Const>
    {
    private:
        template <bool>
        friend struct sentinel;

        using Base = Internal::maybe_const<Const, View>;

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

    // libstdc++ std::reverse_iterator use pre C++ concept when the concept feature is off
    // which requires that the iterator type has the aliases
    // of difference_type, value_type, pointer, reference, iterator_category,
    // With C++20, the iterator concept support is used to deduce the traits
    // needed, therefore alleviating the need to special std::iterator_traits
    // The following code allows std::reverse_iterator(which is aliased into AZStd namespace)
    // to work with the AZStd range views
    #if !__cpp_lib_concepts
        using pointer = void;
        using reference = decltype(get_element(declval<iterator_t<Base>>()));
    #endif

        template<default_initializable BaseIter = iterator_t<Base>>
        iterator() {}

        constexpr iterator(iterator_t<Base> current)
            : m_current(AZStd::move(current))
        {
        }
        template<class ViewIter = iterator_t<View>, class BaseIter = iterator_t<Base>>
            requires Const
                && convertible_to<ViewIter, BaseIter>
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

        constexpr auto operator++(int)
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

        constexpr iterator& operator--()
            requires bidirectional_range<Base>
        {
            --m_current;
            return *this;
        }

        constexpr iterator operator--(int)
            requires bidirectional_range<Base>
        {
            auto tmp = *this;
            --(*this);
            return tmp;
        }

        constexpr iterator& operator+=(difference_type n)
            requires random_access_range<Base>
        {
            m_current += n;
            return *this;
        }

        constexpr iterator& operator-=(difference_type n)
            requires random_access_range<Base>
        {
            m_current -= n;
            return *this;
        }

        constexpr decltype(auto) operator[](difference_type n) const
            requires random_access_range<Base>
        {
            return get_element(m_current + n);
        }

        // equality_comparable
        template<equality_comparable BaseIter = iterator_t<Base>>
        friend constexpr bool operator==(const iterator& x, const iterator& y)
        {
            return x.m_current == y.m_current;
        }
        friend constexpr bool operator!=(const iterator& y, const iterator& x)
        {
            return !operator==(x, y);
        }

        // strict_weak_order
        friend constexpr bool operator<(const iterator& x, const iterator& y)
            requires random_access_range<Base>
        {
            return x.m_current < y.m_current;
        }
        friend constexpr bool operator>(const iterator& x, const iterator& y)
            requires random_access_range<Base>
        {
            return y < x;
        }
        friend constexpr bool operator<=(const iterator& x, const iterator& y)
            requires random_access_range<Base>
        {
            return !(y < x);
        }
        friend constexpr bool operator>=(const iterator& x, const iterator& y)
            requires random_access_range<Base>
        {
            return !(x < y);
        }

        friend constexpr iterator operator+(const iterator& x, difference_type n)
            requires random_access_range<Base>
        {
            iterator iterCopy(x);
            iterCopy += n;
            return iterCopy;
        }

        friend constexpr iterator operator+(difference_type n, const iterator& x)
            requires random_access_range<Base>
        {
            return n + x;
        }

        friend constexpr iterator operator-(const iterator& x, difference_type n)
            requires random_access_range<Base>
        {
            iterator iterCopy(x);
            iterCopy -= n;
            return iterCopy;
        }

        template<class BaseIter = iterator_t<Base>>
            requires sized_sentinel_for<BaseIter, BaseIter>
        friend constexpr difference_type operator-(const iterator& x, const iterator& y)
        {
            return x.m_current - y.m_current;
        }

    private:
        //! iterator to range being viewed
        iterator_t<Base> m_current{};
    };

    namespace ElementsViewInternal
    {
        struct requirements_fulfilled {};
    }

    template<input_range View, size_t N>
        requires view<View>
            && Internal::has_tuple_element<range_value_t<View>, N>
            && Internal::has_tuple_element<remove_reference_t<range_reference_t<View>>, N>
            && Internal::returnable_element<range_reference_t<View>, N>
    template<bool Const>
    struct elements_view<View, N>::sentinel
    {
    private:
        using Base = Internal::maybe_const<Const, View>;

    public:
        sentinel() = default;

        explicit constexpr sentinel(sentinel_t<View> end)
            : m_end(end)
        {
        }
        template<class SentinelView = sentinel_t<View>, class SentinelBase = sentinel_t<Base>>
            requires Const
                && convertible_to<SentinelView, SentinelBase>
        constexpr sentinel(sentinel<!Const> s)
            : m_end(AZStd::move(s.m_end))
        {
        }

        constexpr sentinel_t<View> base() const
        {
            return m_end;
        }

        // comparison operators
        template<bool OtherConst>
            requires sentinel_for<sentinel_t<Base>, iterator_t<Internal::maybe_const<OtherConst, Base>>>
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
        template<bool OtherConst>
            requires sized_sentinel_for<sentinel_t<Base>, iterator_t<Internal::maybe_const<OtherConst, Base>>>
        friend constexpr range_difference_t<Internal::maybe_const<OtherConst, Base >>
            operator-(const iterator<OtherConst>& x, const sentinel& y)
        {
            return iterator_accessor(x) - y.m_end;
        }

        template<bool OtherConst>
            requires sized_sentinel_for<sentinel_t<Base>, iterator_t<Internal::maybe_const<OtherConst, Base>>>
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
