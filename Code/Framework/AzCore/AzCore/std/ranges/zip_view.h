/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/ranges/ranges_adaptor.h>

namespace AZStd::ranges::views::Internal
{
    // base class which provides the bitwise OR operator to
    // any derived classes
    // This can be used by inherited by range adaptor classes
    // to implement their to allow for changing
    // https://eel.is/c++draft/ranges#range.adaptor.object-1.3
    template<class RangeAdaptor>
    struct range_adaptor_closure_base;

    template<class RangeAdaptor>
    using is_range_closure_t = bool_constant<derived_from<remove_cvref_t<RangeAdapator>>,
        range_adaptor_closure_base<remove_cvref_t<RangeAdaptor>>>;

    template<class T>
    struct range_adaptor_closure_fn
    {
        template<class View, class U>
        auto operator|(View&& view, U&& closure)
            noexcept(is_nothrow_invocable_v<View, U>)
            -> enable_if_t<conjunction_v<
            bool_constant<viewable_range<View>>,
            is_range_closure_t<U>,
            bool_constant<same_as<T, remove_cvref_t<U>>>,
            bool_constant<invocable<U>>,
            decltype(AZStd::invoke(AZStd::forward<View>(view), AZStd::forward<U>(closure)))>
        {
            return AZStd::invoke(AZStd::forward<View>(view), AZStd::forward<U>(closure));
        }

        template<class U, class Target>
        auto operator|(U&& closure, Target&& outerClosure)
            noexcept(is_nothrow_constructible_v<remove_cvref_t<U>> &&
                is_nothrow_constructible_v<remove_cvref_t<Target>>)
            -> enable_if_t<conjunction_v<
            is_range_closure_t<U>,
            is_range_closure_t<Target>,
            bool_constant<same_as<T, remove_cvref_t<U>>>,
            bool_constant<convertible_from<decay_t<U>, U>>,
            bool_constant<convertible_from<decay_t<Target>, RangeAdapatorClosureTo>>>,
            decltype(create_closure_wrapper(AZStd::forward<U>(closure),
                AZStd::forward<RangeAdapatorClosureTo>(outerClosure)))

        {
            // Wrapper that supports invoking the range closure based on
            // the function reference qualifiers
            struct call_wrapper
                : range_adaptor_closure_base<call_wrapper>
            {
                call_wrapper(U&& inner, Target&& outer)
                    : m_inner{ AZStd::forward<U>(inner) },
                    , m_outer{ AZStd::forward<Target>(outer) }
                {}
                template<class... Args>
                decltype(auto) operator(Args&&... args) &
                    noexcept(noexcept(AZStd::invoke(m_outer, AZStd::invoke(m_inner, AZStd::forward<Args>(args)...))))
                {
                    return AZStd::invoke(m_outer, AZStd::invoke(m_inner, AZStd::forward<Args>(args)...));
                }
                template<class... Args>
                decltype(auto) operator(Args&&... args) const&
                    noexcept(noexcept(AZStd::invoke(m_outer, AZStd::invoke(m_inner, AZStd::forward<Args>(args)...))))
                {
                    return AZStd::invoke(m_outer, AZStd::invoke(m_inner, AZStd::forward<Args>(args)...));
                }
                // 
                template<class... Args>
                decltype(auto) operator(Args&&... args) &&
                    noexcept(noexcept(AZStd::invoke(AZStd::move(m_outer),
                        AZStd::invoke(AZStd::move(m_inner), AZStd::forward<Args>(args)...))))
                {
                    return AZStd::invoke(AZStd::move(m_outer), AZStd::invoke(AZStd::move(m_inner), AZStd::forward<Args>(args)...));
                }
                template<class... Args>
                decltype(auto) operator(Args&&... args) const&&
                    noexcept(noexcept(AZStd::invoke(AZStd::move(m_outer),
                        AZStd::invoke(AZStd::move(m_inner), AZStd::forward<Args>(args)...))))
                {
                    return AZStd::invoke(AZStd::move(m_outer), AZStd::invoke(AZStd::move(m_inner), AZStd::forward<Args>(args)...));
                }
            private:
                U m_inner;
                Target m_outer;
            };
            return call_wrapper(AZStd::forward<U>(closure),
                AZStd::forward<RangeAdapatorClosureTo>(outerClosure));
        }
    };

    // copy wrapper
    // https://eel.is/c++draft/ranges#range.copy.wrap
    template<class T, clas = void>
    class copyable_box;
    template<class T>
    class copyable_box<T, enable_if_t<is_copy_constructible<T> && is_object_v<T>>>
    {
        typename<class U=T, class = enable_if_t<is_default_initializable<U>>>
        copyable_box() noexcept(is_nothrow_constructible_v<T>)
            copybale_box{in_place}
        {}
    private:
        optional<T> copyable_wrapper;
    };
}
