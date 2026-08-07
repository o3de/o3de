/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/span.h>
#include <AzCore/std/ranges/all_view.h>
#include <AzCore/std/ranges/ranges_adaptor.h>

namespace AZStd::ranges
{
    //! presents a view of the underlying sequence as a constant.
    //! Elements of const_view cannot be modified
    template<input_range View>
        requires view<View>
    class as_const_view;
}

// views::as_const customization point
namespace AZStd::ranges::views
{
    namespace Internal
    {
        template<class V>
        concept is_all_t_constant_range = requires { requires constant_range<all_t<V>>; };

        template<class V>
        constexpr bool is_view_span = false;
        template<class X, size_t Extent>
        constexpr bool is_view_span<span<X, Extent>> = true;

        template<class V>
        concept is_lvalue_constant_range_not_view =
            is_lvalue_reference_v<V>
            && constant_range<const remove_cvref_t<V>>
            && (!view<remove_cvref_t<V>>);

        template<class V>
        struct get_const_span_type {};
        template<class X, size_t Extent>
        struct get_const_span_type<span<X, Extent>>
        {
            using type = span<const X, Extent>;
        };

        struct as_const_fn
            : Internal::range_adaptor_closure<as_const_fn>
        {
            template<class View>
            constexpr decltype(auto) operator()(View&& view) const
            {
                if constexpr (is_all_t_constant_range<View>)
                {
                    return views::all(AZStd::forward<View>(view));
                }
                else if constexpr (is_view_span<remove_cvref_t<View>>)
                {
                    return typename get_const_span_type<View>::type(AZStd::forward<View>(view));
                }
                else if constexpr (is_lvalue_constant_range_not_view<View>)
                {
                    return ref_view(static_cast<const remove_cvref_t<View>&>(AZStd::forward<View>(view)));
                }
                else
                {
                    return as_const_view(AZStd::forward<View>(view));
                }
            }
        };
    }
    inline namespace customization_point_object
    {
        constexpr Internal::as_const_fn as_const{};
    }
}

namespace AZStd::ranges::Internal
{
    struct as_const_view_requirements_fulfilled {};
}

namespace AZStd::ranges
{
    template<input_range View>
        requires view<View>
    class as_const_view
        : public view_interface<as_const_view<View>>
    {
    public:
        constexpr as_const_view()
            requires default_initializable<View>
        {}

        explicit constexpr as_const_view(View base)
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
            requires (!Internal::simple_view<View>)
        {
            return ranges::cbegin(m_base);
        }
        constexpr auto begin() const
            requires range<const View>
        {
            return ranges::cbegin(m_base);
        }

        constexpr auto end()
            requires (!Internal::simple_view<View>)
        {
            return ranges::cend(m_base);
        }
        constexpr auto end() const
            requires range<const View>
        {
            return ranges::cend(m_base);
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
    as_const_view(R&&) -> as_const_view<views::all_t<R>>;

    template<class T>
    inline constexpr bool enable_borrowed_range<as_const_view<T>> = enable_borrowed_range<T>;
}
