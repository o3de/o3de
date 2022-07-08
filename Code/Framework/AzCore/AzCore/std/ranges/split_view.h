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
#include <AzCore/std/ranges/ranges_functional.h>
#include <AzCore/std/ranges/single_view.h>

namespace AZStd::ranges
{
    template<class View, class Pattern, class = enable_if_t<conjunction_v<
        bool_constant<forward_range<View>>,
        bool_constant<forward_range<Pattern>>,
        bool_constant<view<View>>,
        bool_constant<view<Pattern>>,
        bool_constant<indirectly_comparable<iterator_t<View>, iterator_t<Pattern>, ranges::equal_to>>
        >>>
        class split_view;

    // views::split customization point
    namespace views
    {
        namespace Internal
        {
            struct split_view_fn
                : Internal::range_adaptor_closure<split_view_fn>
            {
                template <class View, class Pattern, class = enable_if_t<conjunction_v<
                    bool_constant<viewable_range<View>>
                    >>>
                constexpr auto operator()(View&& view, Pattern&& pattern) const
                {
                    return split_view(AZStd::forward<View>(view), AZStd::forward<Pattern>(pattern));
                }

                // Create a range_adaptor arugment forwarder which binds the pattern for later
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
            constexpr Internal::split_view_fn split{};
        }
    }

    template<class View, class Pattern, class>
    class split_view
        : public view_interface<split_view<View, Pattern>>
    {
        struct iterator;
        struct sentinel;
    public:
        template <bool Enable = default_initializable<View> && default_initializable<Pattern>,
            class = enable_if_t<Enable>>
        split_view() {}

        constexpr split_view(View base, Pattern pattern)
            : m_base(AZStd::move(base))
            , m_pattern(AZStd::move(pattern))
        {}

        template<class R, class = enable_if_t<conjunction_v<
            bool_constant<forward_range<R>>,
            bool_constant<constructible_from<View, views::all_t<R>>>,
            bool_constant<constructible_from<Pattern, single_view<range_value_t<R>>>>
            >>>
        constexpr split_view(R&& r, range_value_t<R> e)
            : m_base{ views::all(AZStd::forward<R>(r)) }
            , m_pattern{ views::single(AZStd::move(e)) }
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

        constexpr iterator begin()
        {
            return { *this, ranges::begin(m_base), find_next(ranges::begin(m_base)) };
        }

        constexpr auto end()
        {
            if constexpr (common_range<View>)
            {
                return iterator{ *this, ranges::end(m_base), {} };
            }
            else
            {
                return sentinel{ *this };
            }
        }

        constexpr subrange<iterator_t<View>> find_next(iterator_t<View> it)
        {
            auto [first, last] = ranges::search(subrange(it, ranges::end(m_base)), m_pattern);
            if (first != ranges::end(m_base) && ranges::empty(m_pattern))
            {
                ++first;
                ++last;
            }

            return { first, last };
        }
    private:
        View m_base;
        Pattern m_pattern;
    };

    template<class R, class P>
    split_view(R&&, P&&)->split_view<views::all_t<R>, views::all_t<P>>;

    template<class R, class = enable_if_t<forward_range<R>>>
    split_view(R&&, range_value_t<R>)
        -> split_view<views::all_t<R>, single_view<range_value_t<R>>>;

    template<class View, class Pattern, class Enable>
    struct split_view<View, Pattern, Enable>::iterator
    {
        using iterator_concept = forward_iterator_tag;
        using iterator_category = input_iterator_tag;
        using value_type = subrange<iterator_t<View>>;
        using difference_type = range_difference_t<View>;

        iterator() = default;

        constexpr iterator(split_view& parent, iterator_t<View> current, subrange<iterator_t<View>> patternSubrange)
            : m_parent(&parent)
            , m_current(AZStd::move(current))
            , m_next(AZStd::move(patternSubrange))
        {
        }
        constexpr iterator_t<View> base() const
        {
            return m_current;
        }

        constexpr value_type operator*() const
        {
            return { m_current, m_next.begin() };
        }

        constexpr iterator& operator++()
        {
            m_current = m_next.begin();
            if (m_current != ranges::end(m_parent->m_base))
            {
                m_current = m_next.end();
                if (m_current == ranges::end(m_parent->m_base))
                {
                    m_trailing_empty = true;
                    m_next = { m_current, m_current };
                }
                else
                {
                    m_next = m_parent->find_next(m_current);
                }
            }
            else
            {
                m_trailing_empty = false;
            }

            return *this;
        }

        constexpr iterator operator++(int)
        {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }

        friend constexpr bool operator==(const iterator& x, const iterator& y)
        {
            return x.m_current == y.m_current && x.m_trailing_empty == y.m_trailing_empty;
        }
        friend constexpr bool operator!=(const iterator& y, const iterator& x)
        {
            return !operator==(x, y);
        }
    private:
        //! reference to parent split_view
        split_view<View, Pattern>* m_parent{};
        //! iterator of the current split content of the view
        iterator_t<View> m_current{};
        //! The next subrange within the view that matches the pattern
        subrange<iterator_t<View>> m_next{};
        //! Set to true to indicate an iterator
        //! pointing to the empty after the last pattern if the pattern is at the end of the view
        //! For a View="a,b,c," and Pattern=",", the last iterator will point to the empty element
        //! after the last comma. This iteator will not match the sentinel iterator to allow users
        //! to know between the end of the View and an empty element after the last pattern
        bool m_trailing_empty{};
    };

    template<class View, class Pattern, class Enable>
    struct split_view<View, Pattern, Enable>::sentinel
    {
        sentinel() = default;
        explicit constexpr sentinel(split_view& parent)
            : m_end{ ranges::end(parent.m_base) }
        {}

        friend constexpr bool operator==(const iterator& x, const sentinel& y)
        {
            return x.m_current == y.m_end && !x.m_trailing_empty;
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
        sentinel_t<View> m_end;
    };
}
