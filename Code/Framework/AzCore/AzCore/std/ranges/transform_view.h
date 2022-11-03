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
    template<class View, class Func, class = enable_if_t<conjunction_v<
        bool_constant<input_range<View>>,
        bool_constant<view<View>>,
        bool_constant<move_constructible<Func>>,
        is_object<Func>,
        bool_constant<regular_invocable<Func&, range_reference_t<View>>>,
        bool_constant<AZStd::Internal::can_reference<invoke_result_t<Func&, range_reference_t<View>>>>
        >
        >>
    class transform_view;


    // views::transform customization point
    namespace views
    {
       namespace Internal
        {
            struct transform_fn
                : Internal::range_adaptor_closure<transform_fn>
            {
                template <class View, class Func, class = enable_if_t<conjunction_v<
                    bool_constant<viewable_range<View>>
                    >>>
                constexpr auto operator()(View&& view, Func&& func) const
                {
                    return transform_view(AZStd::forward<View>(view), AZStd::forward<Func>(func));
                }

                // Create a range_adaptor forwarder which binds the pattern for later
                template <class Func, class = enable_if_t<constructible_from<decay_t<Func>, Func>>>
                constexpr auto operator()(Func&& func) const
                {
                    return range_adaptor_closure_forwarder(
                        [*this, func = AZStd::forward<Func>(func)](auto&& view) mutable
                        {
                            // Pattern needs to be decayed in case it's type is array
                            return (*this)(AZStd::forward<decltype(view)>(view), AZStd::forward<Func>(func));
                        }
                    );
                }
            };
        }
        inline namespace customization_point_object
        {
            constexpr Internal::transform_fn transform{};
        }
    }

    template<class View, class Func, class>
    class transform_view
        : public view_interface<transform_view<View, Func>>
    {
        template<bool>
        struct iterator;
        template<bool>
        struct sentinel;

    public:
        template <bool Enable = default_initializable<View> && default_initializable<Func>,
            class = enable_if_t<Enable>>
        transform_view() {}

         constexpr transform_view(View base, Func func)
            : m_base(AZStd::move(base))
            , m_func(AZStd::move(func))
        {
        }

        template <bool Enable = copy_constructible<View>, class = enable_if_t<Enable>>
        constexpr View base() const&
        {
            return m_base;
        }
        constexpr View base() &&
        {
            return AZStd::move(m_base);
        }

        constexpr auto begin()
        {
            return iterator<false>{ *this, ranges::begin(m_base) };
        }

        template<bool Enable = range<const View> && regular_invocable<const Func&, range_reference_t<const View>>,
            class = enable_if_t<Enable>>
        constexpr auto begin() const
        {
            return iterator<true>{ *this, ranges::begin(m_base) };
        }

        constexpr auto end()
        {
            if constexpr (!common_range<View>)
            {
                return sentinel<false>{ ranges::end(m_base) };
            }
            else
            {
                return iterator<false>{ *this, ranges::end(m_base) };
            }
        }

        template<bool Enable = range<const View> && regular_invocable<const Func&, range_reference_t<const View>>,
            class = enable_if_t<Enable>>
        constexpr auto end() const
        {
            if constexpr (!common_range<const View>)
            {
                return sentinel<true>{ ranges::end(m_base) };
            }
            else
            {
                return iterator<true>{ *this, ranges::end(m_base) };
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
        Internal::movable_box<Func> m_func{};
    };

    // deduction guides
    template<class R, class F>
    transform_view(R&&, F) -> transform_view<views::all_t<R>, F>;

    template<class View, class Func, bool Const, class = void>
    struct transform_view_iterator_category {};

    template<class View, class Func, bool Const>
    struct transform_view_iterator_category<View, Func, Const,
        enable_if_t<forward_range<Internal::maybe_const<Const, View>> >>
    {
    private:
        // Use a "function" to check the type traits of the join view iterators
        // and return an instance of the correct tag type
        // The function will only be used in the unevaluated context of decltype
        // to determine the type.
        // It is a form of template metaprogramming which uses actual code
        // to create an instance of a type and then uses decltype to extract the type
        static constexpr decltype(auto) get_iterator_category()
        {
            using Base = Internal::maybe_const<Const, View>;
            using IterCategory = typename iterator_traits<iterator_t<Base>>::iterator_category;

            if constexpr (is_lvalue_reference_v<invoke_result_t<Func&, range_reference_t<Base>>>)
            {
                if constexpr (derived_from<IterCategory, random_access_iterator_tag>)
                {
                    return random_access_iterator_tag{};
                }
                else
                {
                    return IterCategory{};
                }
            }
            else
            {
                return input_iterator_tag{};
            }
        }
    public:
        using iterator_category = decltype(get_iterator_category());
    };

    template<class View, class Func, class ViewEnable>
    template<bool Const>
    struct transform_view<View, Func, ViewEnable>::iterator
        : enable_if_t<conjunction_v<
        bool_constant<input_range<View>>,
        bool_constant<view<View>>,
        bool_constant<copy_constructible<Func>>,
        is_object<Func>,
        bool_constant<regular_invocable<Func&, range_reference_t<View>>>,
        bool_constant<AZStd::Internal::can_reference<invoke_result_t<Func&, range_reference_t<View>>>>
        >, transform_view_iterator_category<View, Func, Const>
       >
    {
    private:
        template <bool>
        friend struct sentinel;

        using Parent = Internal::maybe_const<Const, transform_view>;
        using Base = Internal::maybe_const<Const, View>;

    public:

        using iterator_concept = conditional_t<random_access_range<Base>,
            random_access_iterator_tag,
            conditional_t<bidirectional_range<Base>,
                bidirectional_iterator_tag,
                conditional_t<forward_range<Base>,
                    forward_iterator_tag,
                    input_iterator_tag>>>;

        using value_type = remove_cvref_t<invoke_result_t<Func&, range_reference_t<Base>>>;
        using difference_type = range_difference_t<Base>;

    // libstdc++ std::reverse_iterator use pre C++ concept when the concept feature is off
    // which requires that the iterator type has the aliases
    // of difference_type, value_type, pointer, reference, iterator_category,
    // With C++20, the iterator concept support is used to deduce the traits
    // needed, therefore alleviating the need to specialize std::iterator_traits
    // The following code allows std::reverse_iterator(which is aliased into AZStd namespace)
    // to work with the AZStd range views
    #if !__cpp_lib_concepts
        using pointer = void;
        using reference = invoke_result_t<Func&, range_reference_t<Base>>;
    #endif

        template<bool Enable = default_initializable<iterator_t<Base>>, class = enable_if_t<Enable>>
        iterator() {}

        constexpr iterator(Parent& parent, iterator_t<Base> current)
            : m_current(AZStd::move(current))
            , m_parent(AZStd::addressof(parent))
        {
        }
        template<bool Enable = convertible_to<iterator_t<View>, iterator_t<Base>>,
            class = enable_if_t<Const && Enable>>
        iterator(iterator<!Const> i)
            : m_current(i.m_current)
            , m_parent(i.m_parent)
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
            noexcept(noexcept(AZStd::invoke(*declval<Parent>().m_func, *declval<iterator_t<Base>>())))
        {
            return AZStd::invoke(*m_parent->m_func, *m_current);
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
        constexpr iterator& operator--()
        {
            --m_current;
            return *this;
        }

        template<bool Enable = bidirectional_range<Base>, class = enable_if_t<Enable>>
        constexpr iterator operator--(int)
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
            return AZStd::invoke(*m_parent->m_func, m_current[n]);
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
            return { x.m_parent, x.m_current + n};
        }

        template<bool Enable = random_access_range<Base>, class = enable_if_t<Enable>>
        friend constexpr iterator operator+(difference_type n, const iterator& x)
        {
            return { x.m_parent, x.m_current + n};
        }

        template<bool Enable = random_access_range<Base>, class = enable_if_t<Enable>>
        friend constexpr iterator operator-(const iterator& x, difference_type n)
        {
            return { x.m_parent, x.m_current - n};
        }

        template<class BaseIter = iterator_t<Base>, class = enable_if_t<sized_sentinel_for<BaseIter, BaseIter>>>
        friend constexpr difference_type operator-(const iterator& x, const iterator& y)
        {
            return x.m_current - y.m_current;
        }

    private:
        //! iterator to range being viewed
        iterator_t<Base> m_current{};
        //! reference to parent transform
        Parent* m_parent{};
    };

    namespace TransformViewInternal
    {
        struct requirements_fulfilled {};
    }

    template<class View, class Func, class ViewEnable>
    template<bool Const>
    struct transform_view<View, Func, ViewEnable>::sentinel
        : enable_if_t<conjunction_v<
        bool_constant<input_range<View>>,
        bool_constant<view<View>>,
        bool_constant<copy_constructible<Func>>,
        is_object<Func>,
        bool_constant<regular_invocable<Func&, range_reference_t<View>>>,
        bool_constant<AZStd::Internal::can_reference<invoke_result_t<Func&, range_reference_t<View>>>>
        >, TransformViewInternal::requirements_fulfilled>
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
