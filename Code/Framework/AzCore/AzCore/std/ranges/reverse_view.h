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
#include <AzCore/std/ranges/subrange.h>

namespace AZStd::ranges
{
    template<class View, class = enable_if_t<conjunction_v<
        bool_constant<bidirectional_range<View>>,
        bool_constant<view<View>>
        >
        >>
    class reverse_view;


    // views::reverse customization point
    namespace views
    {
        namespace Internal
        {
            template<class T>
            constexpr bool is_reverse_view = false;
            template<class View>
            constexpr bool is_reverse_view<reverse_view<View>> = true;

            template<class T>
            constexpr bool is_sized_reverse_subrange = false;
            template<class I>
            constexpr bool is_sized_reverse_subrange<subrange<reverse_iterator<I>, reverse_iterator<I>, subrange_kind::sized>> = true;

            template<class T>
            constexpr bool is_unsized_reverse_subrange = false;
            template<class I>
            constexpr bool is_unsized_reverse_subrange<subrange<reverse_iterator<I>, reverse_iterator<I>, subrange_kind::unsized>> = true;

            struct reverse_fn
                : Internal::range_adaptor_closure<reverse_fn>
            {
                template <class View, class = enable_if_t<viewable_range<View>>>
                constexpr auto operator()(View&& view) const
                {
                    if constexpr (is_reverse_view<AZStd::remove_cvref_t<View>>)
                    {
                        // A reverse of a reverse view is the original view
                        return view.base();
                    }
                    else if constexpr (is_sized_reverse_subrange<AZStd::remove_cvref_t<View>>)
                    {
                        return subrange(view.end().base(), view.begin().base(), view.size());
                    }
                    else if constexpr (is_unsized_reverse_subrange<AZStd::remove_cvref_t<View>>)
                    {
                        return subrange(view.end().base(), view.begin().base());
                    }
                    else
                    {
                        return reverse_view(AZStd::forward<View>(view));
                    }
                }
            };
        }
        inline namespace customization_point_object
        {
            constexpr Internal::reverse_fn reverse{};
        }
    }

    template<class View, class>
    class reverse_view
        : public view_interface<reverse_view<View>>
    {

    public:
        template <bool Enable = default_initializable<View>,
            class = enable_if_t<Enable>>
        reverse_view() {}

         constexpr explicit reverse_view(View base)
            : m_base(AZStd::move(base))
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

        constexpr reverse_iterator<iterator_t<View>> begin()
        {
            if constexpr (common_range<View>)
            {
                return make_reverse_iterator(ranges::end(m_base));
            }
            else
            {
                if (!m_cachedIter.has_value())
                {
                    m_cachedIter = make_reverse_iterator(ranges::next(ranges::begin(m_base), ranges::end(m_base)));
                }
                return *m_cachedIter;
            }
        }
        constexpr auto begin() const
        {
            return make_reverse_iterator(ranges::end(m_base));
        }

        constexpr reverse_iterator<iterator_t<View>> end()
        {
            return make_reverse_iterator(ranges::begin(m_base));
        }
        constexpr auto end() const
        {
            return make_reverse_iterator(ranges::begin(m_base));
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

        // If the View is a common range, the sentinel type(View::end()) is the same as the iterator type(View::begin())
        // so there is no need to cache it off, otherwise to provide amoritized constant time a reverse iterator
        // that of the iterator type equal to the sentinel value is stored
        struct Empty {};
        using CachedIter = conditional_t<common_range<View>, reverse_iterator<iterator_t<View>>, Empty>;
        AZ_NO_UNIQUE_ADDRESS CachedIter m_cachedIter{};
    };

    // deduction guides
    template<class R>
    reverse_view(R&&) -> reverse_view<views::all_t<R>>;
}
