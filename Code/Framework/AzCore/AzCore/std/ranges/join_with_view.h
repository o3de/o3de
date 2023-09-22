/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/variant.h>
#include <AzCore/std/ranges/all_view.h>
#include <AzCore/std/ranges/ranges_adaptor.h>
#include <AzCore/std/ranges/ranges_functional.h>
#include <AzCore/std/ranges/single_view.h>

namespace AZStd::ranges
{
    namespace Internal
    {
        template<class Range, class Pattern, class = void>
        /*concept*/ constexpr bool compatible_joinable_ranges = false;
        template<class Range, class Pattern>
        /*concept*/ constexpr bool compatible_joinable_ranges<Range, Pattern, enable_if_t<conjunction_v<
            bool_constant<common_with<range_value_t<Range>, range_value_t<Pattern>>>,
            bool_constant<common_reference_with<range_reference_t<Range>, range_reference_t<Pattern>>>,
            bool_constant<common_reference_with<range_rvalue_reference_t<Range>, range_rvalue_reference_t<Pattern>>>
            >
        >> = true;

        template<class R>
        /*concept*/ constexpr bool bidirectional_common = bidirectional_range<R> && common_range<R>;
    }

    template<class View, class Pattern, class = enable_if_t<conjunction_v<
        bool_constant<input_range<View>>,
        bool_constant<view<View>>,
        bool_constant<input_range<range_reference_t<View>>>,
        bool_constant<forward_range<Pattern>>,
        bool_constant<view<Pattern>>,
        bool_constant<Internal::compatible_joinable_ranges<range_reference_t<View>, Pattern>>
        >>>
        class join_with_view;

    // views::join_with customization point
    namespace views
    {
        namespace Internal
        {
            struct join_with_view_fn
                : Internal::range_adaptor_closure<join_with_view_fn>
            {
                template <class View, class Pattern, class = enable_if_t<conjunction_v<
                    bool_constant<viewable_range<View>>
                    >>>
                constexpr auto operator()(View&& view, Pattern&& pattern) const
                {
                    return join_with_view(AZStd::forward<View>(view), AZStd::forward<Pattern>(pattern));
                }

                // Create a range_adaptor argument forwarder which binds the pattern for later
                template <class Pattern, class = enable_if_t<constructible_from<decay_t<Pattern>, Pattern>>>
                constexpr auto operator()(Pattern&& pattern) const
                {
                    return range_adaptor_argument_forwarder(
                        *this, AZStd::forward<Pattern>(pattern));
                }
            };
        }
        inline namespace customization_point_object
        {
            constexpr Internal::join_with_view_fn join_with{};
        }
    }

    template<class View, class Pattern, class>
    class join_with_view
        : public view_interface<join_with_view<View, Pattern>>
    {
        template<bool>
        struct iterator;
        template<bool>
        struct sentinel;

        using InnerRange = range_reference_t<View>;

    public:
        template <bool Enable = default_initializable<View> && default_initializable<Pattern>,
            class = enable_if_t<Enable>>
        join_with_view() {}

        constexpr join_with_view(View base, Pattern pattern)
            : m_base(AZStd::move(base))
            , m_pattern(AZStd::move(pattern))
        {}

        template<class R, class = enable_if_t<conjunction_v<
            bool_constant<forward_range<R>>,
            bool_constant<constructible_from<View, views::all_t<R>>>,
            bool_constant<constructible_from<Pattern, single_view<range_value_t<InnerRange>>>
            >>>>
        constexpr join_with_view(R&& range, range_value_t<InnerRange> elem)
            : m_base{ views::all(AZStd::forward<R>(range)) }
            , m_pattern{ views::single(AZStd::move(elem)) }
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
            constexpr bool UseConst = Internal::simple_view<View> && is_reference_v<InnerRange>
                && Internal::simple_view<Pattern>;
            return iterator<UseConst>{ *this, ranges::begin(m_base) };
        }

        template<class ConstView = const View,
            class = enable_if_t<input_range<ConstView> && is_reference_v<range_reference_t<ConstView>>
            && forward_range<const Pattern>>>
        constexpr auto begin() const
        {
            return iterator<true>{ *this, ranges::begin(m_base) };
        }

        constexpr auto end()
        {
            if constexpr (forward_range<View> && is_reference_v<InnerRange> &&
                forward_range<InnerRange> && common_range<View> && common_range<InnerRange>)
            {
                return iterator<Internal::simple_view<View> && Internal::simple_view<Pattern>>{ *this, ranges::end(m_base) };
            }
            else
            {
                return sentinel<Internal::simple_view<View> && Internal::simple_view<Pattern>>{ *this };
            }
        }
        template<class ConstView = const View,
            class = enable_if_t<input_range<ConstView> && is_reference_v<range_reference_t<ConstView>>
            && forward_range<const Pattern>>>
        constexpr auto end() const
        {
            using InnerRangeConst = range_reference_t<const View>;
            if constexpr (forward_range<const View> && forward_range<InnerRangeConst> &&
                common_range<const View> && common_range<InnerRangeConst>)
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
        Pattern m_pattern{};

        // When the inner range for the view is a reference it doesn't need to be stored
        struct InnerRangeIsReference {};
        using InnerRange_t = conditional_t<is_reference_v<InnerRange>, InnerRangeIsReference,
            Internal::non_propagating_cache<remove_cv_t<InnerRange>>>;
        AZ_NO_UNIQUE_ADDRESS InnerRange_t m_innerRef{};
    };

    // Deduction guides
    template<class R, class P>
    join_with_view(R&&, P&&) -> join_with_view<views::all_t<R>, views::all_t<P>>;

    template<class R, class = enable_if_t<input_range<R>>>
    join_with_view(R&&, range_value_t<range_reference_t<R>>)
        -> join_with_view<views::all_t<R>, single_view<range_value_t<range_reference_t<R>>>>;

    template<class View, class Pattern, bool Const, class = void>
    struct join_with_view_iterator_category {};
    template<class View, class Pattern, bool Const>
    struct join_with_view_iterator_category<View, Pattern, Const, enable_if_t<conjunction_v<
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
            using PatternBase = Internal::maybe_const<Const, Pattern>;

            using OuterIter = iterator_t<Base>;
            using InnerIter = iterator_t<InnerBase>;
            using PatternIter = iterator_t<PatternBase>;

            using OuterC = typename iterator_traits<OuterIter>::iterator_category;
            using InnerC = typename iterator_traits<InnerIter>::iterator_category;
            using PatternC = typename iterator_traits<PatternIter>::iterator_category;

            if constexpr (!is_lvalue_reference_v<
                common_reference_t<iter_reference_t<InnerIter>, iter_reference_t<PatternIter>>>)
            {
                return input_iterator_tag{};
            }
            else if constexpr (derived_from<OuterC, bidirectional_iterator_tag>
                && derived_from<InnerC, bidirectional_iterator_tag>
                && derived_from<PatternC, bidirectional_iterator_tag>
                && common_range<InnerBase>
                && common_range<PatternBase>)
            {
                return bidirectional_iterator_tag{};
            }
            else if constexpr (derived_from<OuterC, forward_iterator_tag>
                && derived_from<InnerC, forward_iterator_tag>
                && derived_from<PatternC, forward_iterator_tag>)
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

    template<class View, class Pattern, class ViewEnable>
    template<bool Const>
    struct join_with_view<View, Pattern, ViewEnable>::iterator
        : enable_if_t<conjunction_v<
        bool_constant<input_range<View>>,
        bool_constant<view<View>>,
        bool_constant<input_range<range_reference_t<View>>>,
        bool_constant<forward_range<Pattern>>,
        bool_constant<view<Pattern>>,
        bool_constant<Internal::compatible_joinable_ranges<range_reference_t<View>, Pattern>>
        >, join_with_view_iterator_category<View, Pattern, Const>
       >
    {
    private:
        friend class join_with_view;

        template<bool>
        friend struct sentinel;

        using Parent = Internal::maybe_const<Const, join_with_view>;
        using Base = Internal::maybe_const<Const, View>;
        using InnerBase = range_reference_t<Base>;
        using PatternBase = Internal::maybe_const<Const, Pattern>;

        using OuterIter = iterator_t<Base>;
        using InnerIter = iterator_t<InnerBase>;
        using PatternIter = iterator_t<PatternBase>;
        static constexpr bool ref_is_glvalue = is_reference_v<InnerBase>;

    public:

        using iterator_concept = conditional_t<ref_is_glvalue && bidirectional_range<Base>
            && Internal::bidirectional_common<InnerBase> && Internal::bidirectional_common<PatternBase>, bidirectional_iterator_tag,
            conditional_t<ref_is_glvalue && forward_range<Base> && forward_range<InnerBase>,
            forward_iterator_tag,
            input_iterator_tag>>;

        using value_type = common_type_t<iter_value_t<InnerIter>, iter_value_t<PatternIter>>;
        using difference_type = common_type_t<iter_difference_t<OuterIter>,
            iter_difference_t<InnerIter>, iter_difference_t<PatternIter>>;

    // libstdc++ std::reverse_iterator use pre C++ concept when the concept feature is off
    // which requires that the iterator type has the aliases
    // of difference_type, value_type, pointer, reference, iterator_category,
    // With C++20, the iterator concept support is used to deduce the traits
    // needed, therefore alleviating the need to special std::iterator_traits
    // The following code allows std::reverse_iterator(which is aliased into AZStd namespace)
    // to work with the AZStd range views
    #if !__cpp_lib_concepts
        using pointer = void;
        using reference = common_reference_t<iter_reference_t<InnerIter>, iter_reference_t<PatternIter>>;
    #endif

        template<bool Enable = default_initializable<OuterIter> && default_initializable<InnerIter>,
            class = enable_if_t<Enable>>
        iterator() {}

        template<bool Enable = Const && convertible_to<iterator_t<View>, OuterIter> &&
            convertible_to<iterator_t<InnerRange>, InnerIter> && convertible_to<iterator_t<Pattern>, PatternIter>,
            class = enable_if_t<Enable>>
        iterator(iterator<!Const> i)
            : m_parent(i.m_parent)
            , m_outerIter(i.m_outerIter)
        {
            if (i.m_innerIter.index() == 0)
            {
                m_innerIter.template emplace<0>(get<0>(AZStd::move(i.m_innerIter)));
            }
            else
            {
                m_innerIter.template emplace<1>(get<1>(AZStd::move(i.m_innerIter)));
            }
        }

        constexpr decltype(auto) operator*() const
        {
            using reference = common_reference_t<iter_reference_t<InnerIter>, iter_reference_t<PatternIter>>;
            auto get_reference = [](auto& it) -> reference { return *it; };
            return visit(AZStd::move(get_reference), m_innerIter);
        }

        constexpr iterator& operator++()
        {
            auto increment = [](auto& it) { ++it; };
            visit(increment, m_innerIter);
            satisfy();
            return *this;
        }

        constexpr auto operator++(int)
        {
            if constexpr (ref_is_glvalue && forward_iterator<OuterIter> && forward_iterator<InnerIter>)
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

        template<bool Enable = ref_is_glvalue && bidirectional_range<Base> && Internal::bidirectional_common<InnerBase>
            && Internal::bidirectional_common<PatternBase>,
            class = enable_if_t<Enable>>
        constexpr iterator& operator--()
        {
            if (m_outerIter == ranges::end(m_parent->m_base))
            {
                auto&& inner = *--m_outerIter;
                m_innerIter.template emplace<1>(ranges::end(inner));
            }
            // If at the beginning an inner value, move to the before the last element in the pattern
            // OR if at the beginning of the pattern, move to before the end of the last element o previous inner value
            while (true)
            {
                if (m_innerIter.index() == 0)
                {
                    // The current iterator is pointing within the pattern
                    auto& it = get<0>(m_innerIter);
                    if (it == ranges::begin(m_parent->m_pattern))
                    {
                        // swap iterator to point at the end of the input value
                        auto&& inner = *--m_outerIter;
                        m_innerIter.template emplace<1>(ranges::end(inner));
                    }
                    else
                    {
                        break;
                    }
                }
                else
                {
                    // The current iterator is pointing within an input value
                    auto& it = get<1>(m_innerIter);
                    auto&& inner = *m_outerIter;
                    if (it == ranges::begin(inner))
                    {
                        // swap iterator to point at the end of the pattern
                        m_innerIter.template emplace<0>(ranges::end(m_parent->m_pattern));
                    }
                    else
                    {
                        break;
                    }
                }
            }

            // Move inner iterator backwards to the previous element of the value being viewed
            auto decrement = [](auto& it) { --it; };
            visit(decrement, m_innerIter);
            return *this;
        }

        template<bool Enable = ref_is_glvalue && bidirectional_range<Base> && Internal::bidirectional_common<InnerBase>
            && Internal::bidirectional_common<PatternBase>,
            class = enable_if_t<Enable>>
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
            return x.m_outerIter == y.m_outerIter && x.m_innerIter == y.m_innerIter;
        }
        template<class OtherBase = Base,
            class = enable_if_t<ref_is_glvalue && equality_comparable<iterator_t<OtherBase>>&&
            equality_comparable<iterator_t<range_reference_t<OtherBase>>>>>
        friend constexpr bool operator!=(const iterator& x, const iterator& y)
        {
            return !operator==(x, y);
        }

        // customization of iter_move and iter_swap

        friend constexpr decltype(auto) iter_move(const iterator& x)
        {
            using rvalue_reference = common_reference_t<
                iter_rvalue_reference_t<InnerIter>,
                iter_rvalue_reference_t<PatternIter>>;
            return visit<rvalue_reference>(ranges::iter_move, x.m_innerIter);
        }


        friend constexpr void iter_swap(const iterator& x, const iterator& y)
        {
            visit(ranges::iter_swap, x.m_innerIter, y.m_innerIter);
        }

    private:
        constexpr iterator(Parent& parent, OuterIter outer)
            : m_parent(addressof(parent))
            , m_outerIter(AZStd::move(outer))
        {
            if (m_outerIter != ranges::end(m_parent->m_base))
            {
                // Initialize the iterator to start of the beginning of the inner range
                auto&& inner = update_inner(m_outerIter);
                m_innerIter.template emplace<1>(ranges::begin(inner));
                satisfy();
            }
        }

        // dereference the join_with_view inner iterator if it is is a reference
        // or update the parents copy of the inner iterator
        // by making a copy of the dereference of the outer iter
        constexpr auto&& update_inner(const OuterIter& x)
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
                return m_parent->m_innerRef.emplace_deref(x);
            }
        }

        constexpr auto&& get_inner(const OuterIter& x)
        {
            if constexpr (ref_is_glvalue)     // *x is a reference
            {
                (void)this;
                return *x;
            }
            else
            {
                return *m_parent->m_innerRef;
            }
        }

        //! Use to advanced the inner iterator variant to the next valid state
        //! States are advanced as follows
        //! <inner element1 iter begin>
        //! -> <inner element1 iter end>
        //! -> <pattern element iter begin>
        //! -> <pattern element iter end>
        //! -> <inner element2 iter begin>
        //! -> <inner element2 iter end>
        //! -> <pattern element iter begin>
        //! -> <pattern element iter end>
        //! ...
        //! -> <inner elementN iter begin>
        //! -> <inner elementN iter end>
        constexpr void satisfy()
        {
            for (; m_outerIter != ranges::end(m_parent->m_base);)
            {
                if (m_innerIter.index() == 0)
                {
                    // If Currently iterating over the pattern break out the for loop
                    if (get<0>(m_innerIter) != ranges::end(m_parent->m_pattern))
                    {
                        break;
                    }

                    // The end of the pattern has been reached, so update the inner iterator
                    // to point to the next element of the outer iterator
                    auto&& inner = update_inner(m_outerIter);
                    m_innerIter.template emplace<1>(ranges::begin(inner));
                }
                else
                {
                    auto&& inner = get_inner(m_outerIter);
                    if (get<1>(m_innerIter) != ranges::end(inner))
                    {
                        break;
                    }
                    // The m_innerIter iterator is end then the inner range so advanced
                    // the outer iterator
                    if (++m_outerIter == ranges::end(m_parent->m_base))
                    {
                        // The end of the outer range has been reached
                        // so set the m_innerIter iterator to the default pattern iterator
                        if constexpr (ref_is_glvalue)
                        {
                            m_innerIter.template emplace<0>();
                            break;
                        }
                    }

                    // ... and now swap over to iterating over the pattern
                    m_innerIter.template emplace<0>(ranges::begin(m_parent->m_pattern));
                }
            }
        }

        //! iterator to the outer view element of the view wrapped by the join_with_view
        OuterIter m_outerIter{};
        //! iterator to the range element which is wrapped by the view or the pattern
        //! placement new and destructor calls aren't constexpr until C++20
        //! Because AZStd::variant under the hood invokes AZStd::construct_at, which uses placement new
        //! this makes the emplace calls in this class non-constexpr
        //! fallback to use a struct in C++17 with a bool, since it is simple to deal with than a union
#if __cpp_constexpr_dynamic_alloc >= 201907L
        variant<PatternIter, InnerIter> m_innerIter{};
#else
        struct ElementIter
        {
            constexpr bool operator==(const ElementIter& other) const
            {
                return m_elementIndex != other.m_elementIndex ? false
                    : m_elementIndex == 0
                    ? m_pattern == other.m_pattern
                    : m_inner == other.m_inner;
            }
            constexpr bool operator!=(const ElementIter& other) const
            {
                return !operator==(other);
            }
            template<size_t Index, class... Args>
            constexpr auto& emplace(Args&&... args)
            {
                m_elementIndex = Index;
                if constexpr (Index == 0)
                {
                    m_pattern = PatternIter(AZStd::forward<Args>(args)...);
                    return m_pattern;
                }
                else
                {
                    m_inner = InnerIter(AZStd::forward<Args>(args)...);
                    return m_inner;
                }
            }

            constexpr size_t index() const
            {
                return m_elementIndex;
            }

            PatternIter m_pattern{};
            InnerIter m_inner{};
            uint8_t m_elementIndex{};
        } m_innerIter;

        template<size_t Index>
        static constexpr auto& get(ElementIter& elementIter)
        {
            if constexpr (Index == 0)
            {
                return elementIter.m_pattern;
            }
            else
            {
                return elementIter.m_inner;
            }
        }
        template<size_t Index>
        static constexpr const auto& get(const ElementIter& elementIter)
        {
            if constexpr (Index == 0)
            {
                return elementIter.m_pattern;
            }
            else
            {
                return elementIter.m_inner;
            }
        }
        template<size_t Index>
        static constexpr auto&& get(ElementIter&& elementIter)
        {
            if constexpr (Index == 0)
            {
                return AZStd::move(elementIter.m_pattern);
            }
            else
            {
                return AZStd::move(elementIter.m_inner);
            }
        }
        template<size_t Index>
        static constexpr const auto&& get(const ElementIter&& elementIter)
        {
            if constexpr (Index == 0)
            {
                return AZStd::move(elementIter.m_pattern);
            }
            else
            {
                return AZStd::move(elementIter.m_inner);
            }
        }

        template<class Fn, class ElementIter1>
        static constexpr decltype(auto) visit(Fn&& fn, ElementIter1&& elementIter1)
        {
            if (elementIter1.m_elementIndex == 0)
            {
                return AZStd::forward<Fn>(fn)(AZStd::forward<ElementIter1>(elementIter1).m_pattern);
            }
            else
            {
                return AZStd::forward<Fn>(fn)(AZStd::forward<ElementIter1>(elementIter1).m_inner);
            }
        }

        template<class R, class Fn, class ElementIter1>
        static constexpr R visit(Fn&& fn, ElementIter1&& elementIter1)
        {
            if (elementIter1.m_elementIndex == 0)
            {
                return R(AZStd::forward<Fn>(fn)(AZStd::forward<ElementIter1>(elementIter1).m_pattern));
            }
            else
            {
                return R(AZStd::forward<Fn>(fn)(AZStd::forward<ElementIter1>(elementIter1).m_inner));
            }
        }

        template<class Fn, class ElementIter1, class ElementIter2>
        constexpr decltype(auto) visit(Fn&& fn, ElementIter1&& elementIter1, ElementIter2&& elementIter2)
        {
            if (elementIter1.m_elementIndex == 0)
            {
                if (elementIter2.m_elementIndex == 0)
                {
                    return AZStd::forward<Fn>(fn)(AZStd::forward<ElementIter1>(elementIter1).m_pattern,
                        AZStd::forward<ElementIter2>(elementIter2).m_pattern);
                }
                else
                {
                    return AZStd::forward<Fn>(fn)(AZStd::forward<ElementIter1>(elementIter1).m_pattern,
                        AZStd::forward<ElementIter2>(elementIter2).m_inner);
                }
            }
            else
            {
                if (elementIter2.m_elementIndex == 0)
                {
                    return AZStd::forward<Fn>(fn)(AZStd::forward<ElementIter1>(elementIter1).m_inner,
                        AZStd::forward<ElementIter2>(elementIter2).m_pattern);
                }
                else
                {
                    return AZStd::forward<Fn>(fn)(AZStd::forward<ElementIter1>(elementIter1).m_inner,
                        AZStd::forward<ElementIter2>(elementIter2).m_inner);
                }
            }
        }
#endif
        //! reference to parent join_with_view
        Parent* m_parent{};
    };


    // sentinel type for iterator
    namespace JoinWithViewInternal
    {
        struct requirements_fulfilled {};
    }

    template<class View, class Pattern, class ViewEnable>
    template<bool Const>
    struct join_with_view<View, Pattern, ViewEnable>::sentinel
        : enable_if_t<conjunction_v<
        bool_constant<input_range<View>>,
        bool_constant<view<View>>,
        bool_constant<input_range<range_reference_t<View>>>,
        bool_constant<forward_range<Pattern>>,
        bool_constant<view<Pattern>>,
        bool_constant<Internal::compatible_joinable_ranges<range_reference_t<View>, Pattern>>
        >, JoinWithViewInternal::requirements_fulfilled>
    {
    private:
        using Parent = Internal::maybe_const<Const, join_with_view>;
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
            return it.m_outerIter;
        }
        sentinel_t<Base> m_end{};
    };
}
