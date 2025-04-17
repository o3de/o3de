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
#include <AzCore/std/ranges/ranges_algorithm.h>

namespace AZStd::ranges
{
    template<class View, class Pred, class = enable_if_t<conjunction_v<
        bool_constant<input_range<View>>,
        bool_constant<view<View>>,
        is_object<Pred>,
        bool_constant<indirect_unary_predicate<Pred, iterator_t<View>>>
        >
        >>
    class filter_view;


    // views::filter customization point
    namespace views
    {
        namespace Internal
        {
            struct filter_fn
                : Internal::range_adaptor_closure<filter_fn>
            {
                template <class View, class Pred, class = enable_if_t<conjunction_v<
                    bool_constant<viewable_range<View>>
                    >>>
                constexpr auto operator()(View&& view, Pred&& func) const
                {
                    return filter_view(AZStd::forward<View>(view), AZStd::forward<Pred>(func));
                }

                // Create a range_adaptor forwarder which binds the pattern for later
                template <class Pred, class = enable_if_t<constructible_from<decay_t<Pred>, Pred>>>
                constexpr auto operator()(Pred&& func) const
                {
                    return range_adaptor_closure_forwarder(
                        [*this, func = AZStd::forward<Pred>(func)](auto&& view) mutable
                        {
                            // Pattern needs to be decayed in case it's type is array
                            return (*this)(AZStd::forward<decltype(view)>(view), AZStd::forward<Pred>(func));
                        }
                    );
                }
            };
        }
        inline namespace customization_point_object
        {
            constexpr Internal::filter_fn filter{};
        }
    }

    template<class View, class Pred, class>
    class filter_view
        : public view_interface<filter_view<View, Pred>>
    {
        struct iterator;
        struct sentinel;

    public:
        template <bool Enable = default_initializable<View> && default_initializable<Pred>,
            class = enable_if_t<Enable>>
        filter_view() {}

         constexpr filter_view(View base, Pred func)
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

        constexpr const Pred& pred() const
        {
            return *m_func;
        }

        constexpr iterator begin()
        {
            return iterator{ *this, ranges::find_if(m_base, AZStd::ref(*m_func)) };
        }

        constexpr auto end()
        {
            if constexpr (common_range<View>)
            {
                return iterator{ *this, ranges::end(m_base) };
            }
            else
            {
                return sentinel{ *this };
            }
        }

    private:
        View m_base{};
        Internal::movable_box<Pred> m_func{};
    };

    // deduction guides
    template<class R, class Pred>
    filter_view(R&&, Pred) -> filter_view<views::all_t<R>, Pred>;

    template<class View, class Pred, class = void>
    struct filter_view_iterator_category {};

    template<class View, class Pred>
    struct filter_view_iterator_category<View, Pred,
        enable_if_t<forward_range<View> >>
    {
    private:
        // Use a "function" to check the type traits of the View iterator type
        // and return an instance of the correct tag type
        // The function will only be used in the unevaluated context of decltype
        // to determine the type.
        // It is a form of template metaprogramming which uses actual code
        // to create an instance of a type and then uses decltype to extract the type
        static constexpr decltype(auto) get_iterator_category()
        {
            using IterCategory = typename iterator_traits<iterator_t<View>>::iterator_category;

            if constexpr (derived_from<IterCategory, bidirectional_iterator_tag>)
            {
                return bidirectional_iterator_tag{};
            }
            else if constexpr (derived_from<IterCategory, forward_iterator_tag>)
            {
                return forward_iterator_tag{};
            }
            else
            {
                return IterCategory{};
            }
        }
    public:
        using iterator_category = decltype(get_iterator_category());
    };

    template<class View, class Pred, class ViewEnable>
    struct filter_view<View, Pred, ViewEnable>::iterator
        : enable_if_t<conjunction_v<
        bool_constant<input_range<View>>,
        bool_constant<view<View>>,
        is_object<Pred>,
        bool_constant<indirect_unary_predicate<Pred, iterator_t<View>>>
        >, filter_view_iterator_category<View, Pred>
       >
    {
    private:
        friend struct sentinel;

        using Parent = filter_view;

    public:

        using iterator_concept = conditional_t<random_access_range<View>,
            random_access_iterator_tag,
            conditional_t<bidirectional_range<View>,
                bidirectional_iterator_tag,
                conditional_t<forward_range<View>,
                    forward_iterator_tag,
                    input_iterator_tag>>>;

        using value_type = range_value_t<View>;
        using difference_type = range_difference_t<View>;

    // libstdc++ std::reverse_iterator use pre C++ concept when the concept feature is off
    // which requires that the iterator type has the aliases
    // of difference_type, value_type, pointer, reference, iterator_category,
    // With C++20, the iterator concept support is used to deduce the traits
    // needed, therefore alleviating the need to special std::iterator_traits
    // The following code allows std::reverse_iterator(which is aliased into AZStd namespace)
    // to work with the AZStd range views
    #if !__cpp_lib_concepts
        using pointer = void;
        using reference = range_reference_t<View>;
    #endif

        template<bool Enable = default_initializable<iterator_t<View>>, class = enable_if_t<Enable>>
        iterator() {}

        constexpr iterator(Parent& parent, iterator_t<View> current)
            : m_current(AZStd::move(current))
            , m_parent(AZStd::addressof(parent))
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

        constexpr range_reference_t<View> operator*() const
        {
            return *m_current;
        }

        template<bool Enable = Internal::has_arrow<iterator_t<View>> && copyable<iterator_t<View>>, class = enable_if_t<Enable>>
        constexpr iterator_t<View> operator->() const
        {
            return m_current;
        }

        constexpr iterator& operator++()
        {
            m_current = ranges::find_if(AZStd::move(++m_current), ranges::end(m_parent->m_base), AZStd::ref(*m_parent->m_func));
            return *this;
        }

        constexpr decltype(auto) operator++(int)
        {
            if constexpr (!forward_range<View>)
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

        template<bool Enable = bidirectional_range<View>, class = enable_if_t<Enable>>
        constexpr iterator& operator--()
        {
            do
            {
                --m_current;
            } while (!AZStd::invoke(*m_parent->m_func, *m_current));
            return *this;
        }

        template<bool Enable = bidirectional_range<View>, class = enable_if_t<Enable>>
        constexpr iterator operator--(int)
        {
            auto tmp = *this;
            --(*this);
            return tmp;
        }

        // equality_comparable
        template<class ViewIter = iterator_t<View>, class = enable_if_t<equality_comparable<ViewIter>>>
        friend constexpr bool operator==(const iterator& x, const iterator& y)
        {
            return x.m_current == y.m_current;
        }
        friend constexpr bool operator!=(const iterator& y, const iterator& x)
        {
            return !operator==(x, y);
        }

        // customization of iter_move and iter_swap
        friend constexpr decltype(auto) iter_move(
            const iterator& i)
            noexcept(noexcept(ranges::iter_move(i.m_current)))
        {
            return ranges::iter_move(i.m_current);
        }

        friend constexpr void iter_swap(
            const iterator& x,
            const iterator& y)
            noexcept(noexcept(ranges::iter_swap(x.m_current, y.m_current)))
        {
            static_assert(indirectly_swappable<iterator_t<View>>, "iter_swap can only be invoked on iterators that are indirectly swappable");
            ranges::iter_swap(x.m_current, y.m_current);
        }

    private:
        //! iterator to range being viewed
        iterator_t<View> m_current{};
        //! reference to filter_view class
        Parent* m_parent{};
    };

    namespace FilterViewInternal
    {
        struct requirements_fulfilled {};
    }

    template<class View, class Pred, class ViewEnable>
    struct filter_view<View, Pred, ViewEnable>::sentinel
        : enable_if_t<conjunction_v<
        bool_constant<input_range<View>>,
        bool_constant<view<View>>,
        is_object<Pred>,
        bool_constant<indirect_unary_predicate<Pred, iterator_t<View>>>
        >, FilterViewInternal::requirements_fulfilled>
    {
    public:
        sentinel() = default;

        explicit constexpr sentinel(sentinel_t<View> end)
            : m_end(end)
        {
        }

        constexpr sentinel_t<View> base() const
        {
            return m_end;
        }

        // comparison operators>
        friend constexpr bool operator==(const iterator& x, const sentinel& y)
        {
            return iterator_accessor(x) == y.m_end;
        }
        friend constexpr bool operator==(const sentinel& y, const iterator& x)
        {
            return operator==(x, y);
        }

        friend constexpr bool operator!=(const iterator& x, const sentinel& y)
        {
            return !operator==(x, y);
        }
        friend constexpr bool operator!=(const sentinel& y, const iterator& x)
        {
            return !operator==(x, y);
        }

    private:
        // On MSVC The friend functions can only access the sentinel struct members
        // The iterator struct which is a friend of the sentinel struct is NOT a friend
        // of the friend functions
        // So a shim is added to provide access to the iterator m_current member
        static constexpr auto iterator_accessor(const iterator& it)
        {
            return it.m_current;
        }

        sentinel_t<View> m_end{};
    };
}
