/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/iterator/common_iterator.h>
#include <AzCore/std/ranges/all_view.h>
#include <AzCore/std/ranges/ranges_adaptor.h>

namespace AZStd::ranges
{
    template<class View>
        requires (!common_range<View>)
            && copyable<iterator_t<View>>
    class common_view;


    // views::common customization point
    namespace views
    {
       namespace Internal
        {
            template<class View>
            concept is_common_range_with_all_t =
                common_range<View>
                && requires { typename views::all_t<View>; };

            struct common_fn
                : Internal::range_adaptor_closure<common_fn>
            {
                template<class View>
                constexpr auto operator()(View&& view) const
                {
                    if constexpr(is_common_range_with_all_t<View>)
                    {
                        return views::all(AZStd::forward<View>(view));
                    }
                    else
                    {
                        return common_view(AZStd::forward<View>(view));
                    }
                }
            };
        }
        inline namespace customization_point_object
        {
            constexpr Internal::common_fn common{};
        }
    }

    template<class View>
        requires (!common_range<View>)
            && copyable<iterator_t<View>>
    class common_view
        : public view_interface<common_view<View>>
    {
    public:
        common_view()
            requires default_initializable<View>
        {}

         explicit constexpr common_view(View base)
            : m_base(AZStd::move(base))
        {
        }

        constexpr View base() const&
            requires copy_constructible<View>
        {
            return m_base;
        }
        constexpr View base() &&
        {
            return AZStd::move(m_base);
        }

        constexpr auto begin()
        {
            if constexpr (random_access_range<View> && sized_range<View>)
            {
                return ranges::begin(m_base);
            }
            else
            {
                return common_iterator<iterator_t<View>, sentinel_t<View>>(ranges::begin(m_base));
            }
        }

        constexpr auto begin() const
            requires range<const View>
        {
            if constexpr (random_access_range<const View> && sized_range<const View>)
            {
                return ranges::begin(m_base);
            }
            else
            {
                return common_iterator<iterator_t<const View>, sentinel_t<const View>>(ranges::begin(m_base));
            }
        }

        constexpr auto end()
        {
            if constexpr (random_access_range<View> && sized_range<View>)
            {
                return ranges::begin(m_base) + ranges::size(m_base);
            }
            else
            {
                return common_iterator<iterator_t<View>, sentinel_t<View>>(ranges::end(m_base));
            }
        }

        constexpr auto end() const
            requires range<const View>
        {
            if constexpr (random_access_range<const View> && sized_range<const View>)
            {
                return ranges::begin(m_base) + ranges::size(m_base);
            }
            else
            {
                return common_iterator<iterator_t<const View>, sentinel_t<const View>>(ranges::end(m_base));
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

    // deduction guides
    template<class R>
    common_view(R&&) -> common_view<views::all_t<R>>;
}
