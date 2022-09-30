/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/iterator/move_sentinel.h>
#include <AzCore/std/ranges/all_view.h>
#include <AzCore/std/ranges/ranges_adaptor.h>

namespace AZStd::ranges
{
    //! view which provides the same behavior of the underlying sequenence, except
    //! that its elements are rvalues
    template<class View>
    class as_rvalue_view;
}

// views::as_rvalue customization point
namespace AZStd::ranges::views
{
    namespace Internal
    {
        struct as_rvalue_fn
            : Internal::range_adaptor_closure<as_rvalue_fn>
        {
            template<class View>
            constexpr auto operator()(View&& view) const
            {
                // If the rvalue refrence is the same as the reference type, then an rvalue view
                // will not have an effect, so use use all to return a suitable view
                if constexpr(same_as<range_rvalue_reference_t<View>, range_reference_t<View>>)
                {
                    return views::all(AZStd::forward<View>(view));
                }
                else
                {
                    return as_rvalue_view(AZStd::forward<View>(view));
                }
            }
        };
    }
    inline namespace customization_point_object
    {
        constexpr Internal::as_rvalue_fn as_rvalue{};
    }
}

namespace AZStd::ranges::Internal
{
    struct as_rvalue_view_requirements_fulfilled {};
}

namespace AZStd::ranges
{
    template<class View>
    class as_rvalue_view
        : public view_interface<as_rvalue_view<View>>
        , private enable_if_t<input_range<View>, Internal::as_rvalue_view_requirements_fulfilled>
    {
    public:
        template <bool Enable = default_initializable<View>,
            class = enable_if_t<Enable>>
        constexpr as_rvalue_view() {}

        explicit constexpr as_rvalue_view(View base)
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

        template<bool Enable = !Internal::simple_view<View>, class = enable_if_t<Enable>>
        constexpr auto begin()
        {
            return move_iterator{ ranges::begin(m_base) };
        }

        template<bool Enable = range<const View>, class = enable_if_t<Enable>>
        constexpr auto begin() const
        {
            return move_iterator{ ranges::begin(m_base) };
        }

        template<bool Enable = !Internal::simple_view<View>, class = enable_if_t<Enable>>
        constexpr auto end()
        {
            if constexpr (common_range<View>)
            {
                return move_iterator{ ranges::end(m_base) };
            }
            else
            {
                return move_sentinel{ ranges::end(m_base) };
            }
        }

        template<bool Enable = range<const View>, class = enable_if_t<Enable>>
        constexpr auto end() const
        {
            if constexpr (common_range<const View>)
            {
                return move_iterator{ ranges::end(m_base) };
            }
            else
            {
                return move_sentinel{ ranges::end(m_base) };
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

    // deduction guides
    template<class R>
    as_rvalue_view(R&&) -> as_rvalue_view<views::all_t<R>>;
}
