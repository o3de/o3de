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
#include <AzCore/std/ranges/ranges_functional.h>
#include <AzCore/std/typetraits/is_reference.h>

namespace AZStd::ranges
{
    template<class View, class = enable_if_t<conjunction_v<
        bool_constant<input_range<View>>,
        bool_constant<view<View>>,
        bool_constant<input_range<range_reference_t<View>>>
        >>>
        class join_view;

    // views::join customization point
    namespace views
    {
        namespace Internal
        {
            struct join_view_fn
                : Internal::range_adaptor_closure<join_view_fn>
            {
                template <class View, class = enable_if_t<conjunction_v<
                    bool_constant<viewable_range<View>>
                    >>>
                constexpr auto operator()(View&& view) const
                {
                    return join_view(views::all(AZStd::forward<View>(view)));
                }
            };
        }
        inline namespace customization_point_object
        {
            constexpr Internal::join_view_fn join{};
        }
    }

    template<class View, class>
    class join_view
        : public view_interface<join_view<View>>
    {
        template<bool>
        struct iterator;
        template<bool>
        struct sentinel;

    public:
        template <bool Enable = default_initializable<View>,
            class = enable_if_t<Enable>>
        join_view() {}

        explicit constexpr join_view(View base)
            : m_base(AZStd::move(base))
        {}


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
            constexpr bool UseConst = Internal::simple_view<View> && is_reference_v<range_reference_t<View>>;
            return iterator<UseConst>{ *this, ranges::begin(m_base) };
        }

        template<class ConstView = const View,
            class = enable_if_t<input_range<ConstView> && is_reference_v<range_reference_t<ConstView>>>>
        constexpr auto begin() const
        {
            return iterator<true>{ *this, ranges::begin(m_base) };
        }

        constexpr auto end()
        {
            if constexpr (forward_range<View> && is_reference_v<InnerRange> &&
                forward_range<InnerRange> && common_range<View> && common_range<InnerRange>)
            {
                return iterator<Internal::simple_view<View>>{ *this, ranges::end(m_base) };
            }
            else
            {
                return sentinel<Internal::simple_view<View>>{ *this };
            }
        }
        template<class ConstView = const View,
            class = enable_if_t<input_range<ConstView> && is_reference_v<range_reference_t<ConstView>>>>
        constexpr auto end() const
        {
            if constexpr (forward_range<const View> && forward_range<range_reference_t<const View>> &&
                common_range<const View> && common_range<range_reference_t<const View>>)
            {
                return iterator<true>{ *this, ranges::end(m_base) };
            }
            else
            {
                return sentinel<true>{ *this };
            }
        }
    private:
        View m_base{};

        using InnerRange = range_reference_t<View>;
        // When the inner range for the view is a reference it doesn't need to be stored
        struct InnerRangeIsReference {};
        using InnerRange_t = conditional_t<is_reference_v<InnerRange>, InnerRangeIsReference,
            Internal::non_propagating_cache<remove_cv_t<InnerRange>>>;
        AZ_NO_UNIQUE_ADDRESS InnerRange_t m_inner{};
    };

    template<class View, bool Const, class = void>
    struct join_view_iterator_category {};
    template<class View, bool Const>
    struct join_view_iterator_category<View, Const, enable_if_t<conjunction_v<
        /*ref-is-glvalue*/ is_reference<range_reference_t<Internal::maybe_const<Const, View>>>,
        bool_constant<forward_range<Internal::maybe_const<Const, View>>>,
        bool_constant<forward_range<range_reference_t<Internal::maybe_const<Const, View>>>>
        >>>
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
            using InnerBase = range_reference_t<Base>;

            using OuterIter = iterator_t<Base>;
            using InnerIter = iterator_t<InnerBase>;

            using OuterC = typename iterator_traits<OuterIter>::iterator_category;
            using InnerC = typename iterator_traits<InnerIter>::iterator_category;

            if constexpr (derived_from<OuterC, bidirectional_iterator_tag>
                && derived_from<InnerC, bidirectional_iterator_tag>
                && common_range<InnerBase>)
            {
                return bidirectional_iterator_tag{};
            }
            else if constexpr (derived_from<OuterC, forward_iterator_tag>
                && derived_from<InnerC, forward_iterator_tag>)
            {
                return forward_iterator_tag{};
            }
            else
            {
                return input_iterator_tag{};
            }
        }
    public:
        using iterator_category = decltype(get_iterator_category());
    };


    template<class R>
    join_view(R&&) -> join_view<views::all_t<R>>;

    template<class View, class ViewEnable>
    template<bool Const>
    struct join_view<View, ViewEnable>::iterator
        : enable_if_t<conjunction_v<
        bool_constant<input_range<View>>,
        bool_constant<view<View>>,
        bool_constant<input_range<range_reference_t<View>>>
        >, join_view_iterator_category<View, Const>
       >
    {
    private:
        template<bool>
        friend struct sentinel;

        using Parent = Internal::maybe_const<Const, join_view>;
        using Base = Internal::maybe_const<Const, View>;
        using OuterIter = iterator_t<Base>;
        using InnerIter = iterator_t<range_reference_t<Base>>;
        static constexpr bool ref_is_glvalue = is_reference_v<range_reference_t<Base>>;
    public:

        using iterator_concept = conditional_t<ref_is_glvalue && bidirectional_range<Base>
            && common_range<Base>, bidirectional_iterator_tag,
            conditional_t<ref_is_glvalue && forward_range<Base> && forward_range<range_reference_t<Base>>,
            forward_iterator_tag,
            input_iterator_tag>>;

        using value_type = range_value_t<range_reference_t<Base>>;
        using difference_type = common_type_t<range_difference_t<Base>,
            range_difference_t<range_reference_t<Base>>>;

    // libstdc++ std::reverse_iterator use pre C++ concept when the concept feature is off
    // which requires that the iterator type has the aliases
    // of difference_type, value_type, pointer, reference, iterator_category,
    // With C++20, the iterator concept support is used to deduce the traits
    // needed, therefore alleviating the need to special std::iterator_traits
    // The following code allows std::reverse_iterator(which is aliased into AZStd namespace)
    // to work with the AZStd range views
    #if !__cpp_lib_concepts
        using pointer = void;
        using reference = range_reference_t<range_reference_t<Base>>;
    #endif

        template<bool Enable = default_initializable<OuterIter> && default_initializable<InnerIter>,
            class = enable_if_t<Enable>>
        iterator() {}

        constexpr iterator(Parent& parent, OuterIter outer)
            : m_parent(addressof(parent))
            , m_outer(AZStd::move(outer))
        {
            satisfy();
        }
        template<bool Enable = convertible_to<iterator_t<View>, OuterIter> &&
            convertible_to<iterator_t<InnerRange>, InnerIter>,
            class = enable_if_t<Enable>>
        iterator(iterator<!Const> i)
            : m_parent(i.m_parent)
            , m_outer(i.m_outer)
            , m_inner(i.m_inner)
        {}

        constexpr iterator_t<View> base() const
        {
            return m_inner;
        }

        constexpr decltype(auto) operator*() const
        {
            return *m_inner;
        }

        template<bool Enable = Internal::has_arrow<InnerIter> && copyable<InnerIter>,
            class = enable_if_t<Enable>>
        constexpr InnerIter operator->() const
        {
            return m_inner;
        }

        constexpr iterator& operator++()
        {
            if constexpr (ref_is_glvalue)
            {
                auto&& innerRange = *m_outer;
                if (++m_inner == ranges::end(innerRange))
                {
                    ++m_outer;
                    satisfy();
                }
            }
            else
            {
                auto&& innerRange = *m_parent->m_inner;
                if (++m_inner == ranges::end(innerRange))
                {
                    ++m_outer;
                    satisfy();

                }
            }
            return *this;
        }

        constexpr auto operator++(int)
        {
            if constexpr (ref_is_glvalue && forward_range<Base> && forward_range<range_reference_t<Base>>)
            {
                auto tmp = *this;
                ++(*this);
                return tmp;
            }
            else
            {
                ++(*this);
            }
        }

        template<bool Enable = ref_is_glvalue && bidirectional_range<Base> && bidirectional_range<range_reference_t<Base>>,
            class = enable_if_t<Enable && common_range<range_reference_t<Base>>>>
        constexpr iterator& operator--()
        {
            if (m_outer == ranges::end(m_parent->m_base))
            {
                m_inner = ranges::end(*--m_outer);
            }
            // Move inner iterator backwards to the last element of the first non-empty outer iterator
            // by iterating backwards
            while (m_inner == ranges::begin(*m_outer))
            {
                m_inner = ranges::end(*--m_outer);
            }
            --m_inner;
            return *this;
        }

        template<bool Enable = ref_is_glvalue && bidirectional_range<Base> && bidirectional_range<range_reference_t<Base>>,
            class = enable_if_t<Enable && common_range<range_reference_t<Base>>>>
        constexpr iterator operator--(int)
        {
            auto tmp = *this;
            --(*this);
            return tmp;
        }

        template<class OtherBase = Base,
            class = enable_if_t<ref_is_glvalue && equality_comparable<iterator_t<OtherBase>> &&
            equality_comparable<iterator_t<range_reference_t<OtherBase>>>>>
        friend constexpr bool operator==(const iterator& x, const iterator& y)
        {
            return x.m_outer == y.m_outer && x.m_inner == y.m_inner;
        }
        template<class OtherBase = Base,
            class = enable_if_t<ref_is_glvalue && equality_comparable<iterator_t<OtherBase>>&&
            equality_comparable<iterator_t<range_reference_t<OtherBase>>>>>
        friend constexpr bool operator!=(const iterator& x, const iterator& y)
        {
            return !operator==(x, y);
        }

        // customization of iter_move and iter_swap

        friend constexpr decltype(auto) iter_move(
            const iterator& i)
            noexcept(noexcept(ranges::iter_move(i.m_inner)))
        {
            return ranges::iter_move(i.m_inner);
        }


        friend constexpr void iter_swap(
            const iterator& x,
            const iterator& y)
            noexcept(noexcept(ranges::iter_swap(x.m_inner, y.m_inner)))
        {
            return ranges::iter_swap(x.m_inner, y.m_inner);
        }

    private:
        constexpr void satisfy()
        {
            // dereference the outer iterator if the inner iterator is a reference
            // or make a copy of the dereference of outer iterator
            auto update_inner = [this](const OuterIter& x) constexpr -> auto&&
            {
                if constexpr (ref_is_glvalue)     // *x is a reference
                {
                    // workaround clang 9.0.0 bug where this is unused because
                    // of not being used in one block of the if constexpr
                    (void)this;
                    return *x;
                }
                else
                {
                    return m_parent->m_inner.emplace_deref(x);
                }
            };

            for (; m_outer != ranges::end(m_parent->m_base); ++m_outer)
            {
                auto&& inner = update_inner(m_outer);
                m_inner = ranges::begin(inner);
                // m_inner is end then the inner range is empty
                // and the next outer element is iterated over
                if (m_inner != ranges::end(inner))
                {
                    return;
                }
            }
            if constexpr (ref_is_glvalue)
            {
                m_inner = InnerIter();
            }
        }

        //! iterator to the outer view element of the view wrapped by the join view
        OuterIter m_outer{};
        //! iterator to the actually range element which is wrapped by the view
        InnerIter m_inner{};
        //! reference to parent join_view
        Parent* m_parent{};
    };


    // sentinel type for iterator
    namespace JoinViewInternal
    {
        struct requirements_fulfilled {};
    }

    template<class View, class ViewEnable>
    template<bool Const>
    struct join_view<View, ViewEnable>::sentinel
        : enable_if_t<conjunction_v<
        bool_constant<input_range<View>>,
        bool_constant<view<View>>,
        bool_constant<input_range<range_reference_t<View>>>
        >, JoinViewInternal::requirements_fulfilled>
    {
    private:
        using Parent = Internal::maybe_const<Const, join_view>;
        using Base = Internal::maybe_const<Const, View>;

    public:
        sentinel() = default;
        explicit constexpr sentinel(Parent& parent)
            : m_end(ranges::end(parent.m_base))
        {}
        template<bool Enable = Const,
            class = enable_if_t<Enable && convertible_to<sentinel_t<View>, sentinel_t<Base>>>>
        constexpr sentinel(sentinel<!Const> s)
            : m_end(AZStd::move(s.m_end))
        {
        }

        // comparison operators
        template<bool OtherConst, class = enable_if_t<
            sentinel_for<sentinel_t<Base>, iterator_t<Internal::maybe_const<OtherConst, Base>>>
        >>
        friend constexpr bool operator==(const iterator<OtherConst>& x, const sentinel& y)
        {
            return iterator_accessor(x) == y.m_end;
        }
        template<bool OtherConst, class = enable_if_t<
            sentinel_for<sentinel_t<Base>, iterator_t<Internal::maybe_const<OtherConst, Base>>>
        >>
        friend constexpr bool operator==(const sentinel& y, const iterator<OtherConst>& x)
        {
            return operator==(x, y);
        }
        template<bool OtherConst, class = enable_if_t<
            sentinel_for<sentinel_t<Base>, iterator_t<Internal::maybe_const<OtherConst, Base>>>
        >>
        friend constexpr bool operator!=(const iterator<OtherConst>& x, const sentinel& y)
        {
            return !operator==(x, y);
        }
        template<bool OtherConst, class = enable_if_t<
            sentinel_for<sentinel_t<Base>, iterator_t<Internal::maybe_const<OtherConst, Base>>>
        >>
        friend constexpr bool operator!=(const sentinel& y, const iterator<OtherConst>& x)
        {
            return !operator==(x, y);
        }
    private:
        // On MSVC The friend functions are can only access the sentinel struct members
        // The iterator struct which is a friend of the sentinel struct is NOT a friend
        // of the friend functions
        // So a shim is added to provide access to the iterator m_current member
        template<bool OtherConst>
        static constexpr auto iterator_accessor(const iterator<OtherConst>& it)
        {
            return it.m_outer;
        }
        sentinel_t<Base> m_end{};
    };
}
