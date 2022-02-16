/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/ranges/ranges.h>
#include <AzCore/std/optional.h>

namespace AZStd::ranges::views::Internal
{
    // Call wrapping that performs perfecting forwarding of arguments
    // and take const and ref function qualifiers into account(rvalue functions will move)
    template<class Outer, class Inner>
    struct perfect_forwarding_call_wrapper
    {
        perfect_forwarding_call_wrapper(Outer&& outer, Inner&& inner)
            : m_outer{ AZStd::forward<Outer>(outer) }
            , m_inner{ AZStd::forward<Inner>(inner) }
        {}
        template<class... Args>
        constexpr decltype(auto) operator()(Args&&... args) &
            noexcept(noexcept(AZStd::invoke(declval<Outer>(), AZStd::invoke(declval<Inner>(), AZStd::forward<Args>(args)...))))
        {
            return AZStd::invoke(m_outer, AZStd::invoke(m_inner, AZStd::forward<Args>(args)...));
        }
        template<class... Args>
        constexpr decltype(auto) operator()(Args&&... args) const&
            noexcept(noexcept(AZStd::invoke(declval<Outer>(), AZStd::invoke(declval<Inner>(), AZStd::forward<Args>(args)...))))
        {
            return AZStd::invoke(m_outer, AZStd::invoke(m_inner, AZStd::forward<Args>(args)...));
        }
        //
        template<class... Args>
        constexpr decltype(auto) operator()(Args&&... args) &&
            noexcept(noexcept(AZStd::invoke(declval<Outer>(),
                AZStd::invoke(declval<Inner>(), AZStd::forward<Args>(args)...))))
        {
            return AZStd::invoke(AZStd::move(m_outer), AZStd::invoke(AZStd::move(m_inner), AZStd::forward<Args>(args)...));
        }
        template<class... Args>
        constexpr decltype(auto) operator()(Args&&... args) const&&
            noexcept(noexcept(AZStd::invoke(declval<Outer>(),
                AZStd::invoke(declval<Inner>(), AZStd::forward<Args>(args)...))))
        {
            return AZStd::invoke(AZStd::move(m_outer), AZStd::invoke(AZStd::move(m_inner), AZStd::forward<Args>(args)...));
        }
    private:
        Outer m_outer;
        Inner m_inner;
    };

    // base class which provides the bitwise OR operator to
    // any derived classes
    // This can be used by inherited by range adaptor classes
    // to implement their to allow for changing
    // https://eel.is/c++draft/ranges#range.adaptor.object-1.3
    template<class RangeAdaptor>
    struct range_adaptor_closure;

    // Return a closure new in which the outer range adaptor invokes the inner range adaptor
    template<class RangeAdaptorFunctor>
    struct range_adaptor_closure_forwarder
        : RangeAdaptorFunctor
        , range_adaptor_closure_forwarder<range_adaptor_closure<RangeAdaptorFunctor>>
    {
        constexpr explicit range_adaptor_closure_forwarder(RangeAdaptorFunctor&& closure)
            : RangeAdaptorFunctor{ AZStd::forward<RangeAdaptorFunctor>(closure) }
        {}
    };

    template<class RangeAdaptor>
    using is_range_closure_t = bool_constant<derived_from<remove_cvref_t<RangeAdaptor>,
        range_adaptor_closure<remove_cvref_t<RangeAdaptor>>>>;

    template<class T>
    struct range_adaptor_closure
    {
        template<class View, class U>
        friend constexpr auto operator|(View&& view, U&& closure) noexcept(is_nothrow_invocable_v<View, U>)
            -> enable_if_t<conjunction_v<
            bool_constant<viewable_range<View>>,
            is_range_closure_t<U>,
            bool_constant<same_as<T, remove_cvref_t<U>>>,
            bool_constant<invocable<U, View>>>,
            decltype(AZStd::invoke(AZStd::forward<U>(closure), AZStd::forward<View>(view)))>
        {
            return AZStd::invoke(AZStd::forward<U>(closure), AZStd::forward<View>(view));
        }

        template<class U, class Target>
        friend constexpr auto operator|(U&& closure, Target&& outerClosure)
            noexcept(is_nothrow_constructible_v<remove_cvref_t<U>> && is_nothrow_constructible_v<remove_cvref_t<Target>>)
            ->enable_if_t<conjunction_v<
            is_range_closure_t<U>,
            is_range_closure_t<Target>,
            bool_constant<same_as<T, remove_cvref_t<U>>>,
            bool_constant<constructible_from<decay_t<U>, U>>,
            bool_constant<constructible_from<decay_t<Target>, Target>>>,
            decltype(range_adaptor_closure_forwarder{
                perfect_forwarding_call_wrapper{AZStd::forward<Target>(outerClosure), AZStd::forward<U>(closure) }
                })>
        {
            // Create a perfect_forwarding_wrapper that wraps the outer adaptor around the inner adaptor
            // and then pass that to the range_adaptor_closure_forward struct which inherits from
            // range_adaptor_closure class template so that it is_range_closure_t template succeeds
            return range_adaptor_closure_forwarder{
                perfect_forwarding_call_wrapper{AZStd::forward<Target>(outerClosure), AZStd::forward<U>(closure) }
            };
        }
    };
} // namespace AZStd::ranges::views::Internal

namespace AZStd::ranges::Internal
{
    // Copyable Wrapper - https://eel.is/c++draft/ranges#range.copy.wrap
    // Used to wrap a class that copy constructible, but not necessarily copy assignable
    // and implements a copy assignment operator using the optional emplace function
    // to construct in place
    template<class T, class = void>
    class copyable_box;

    template<class T>
    class copyable_box<T, enable_if_t<is_copy_constructible_v<T>&& is_object_v<T>>>
    {
    public:
        template<class U = T, class = enable_if_t<default_initializable<U>>>
        constexpr copyable_box() noexcept(is_nothrow_constructible_v<T>)
            : copyable_box{ in_place }
        {}

        template<class... Args>
        explicit constexpr copyable_box(in_place_t, Args&&... args) noexcept(is_nothrow_constructible_v<T>)
            : m_copyable_wrapper{ in_place, AZStd::forward<Args>(args)... }
        {}

        constexpr copyable_box(const copyable_box&) = default;
        constexpr copyable_box(copyable_box&&) = default;

        constexpr copyable_box& operator=(const copyable_box& other) noexcept(is_nothrow_copy_constructible_v<T>)
        {
            if (this != &other)
            {
                if constexpr (copyable<T>)
                {
                    m_copyable_wrapper = other.m_copyable_wrapper;
                }
                else
                {
                    if (other)
                    {
                        emplace(other.m_copyable_wrapper);
                    }
                    else
                    {
                        reset();
                    }
                }
            }

            return *this;
        }

        constexpr copyable_box& operator=(copyable_box&& other) noexcept(is_nothrow_move_constructible_v<T>)
        {
            if (this != &other)
            {
                if constexpr (movable<T>)
                {
                    m_copyable_wrapper = AZStd::move(other.m_copyable_wrapper);
                }
                else
                {
                    if (other)
                    {
                        emplace(AZStd::move(other.m_copyable_wrapper));
                    }
                    else
                    {
                        reset();
                    }
                }
            }

            return *this;
        }
        constexpr T& operator*() & noexcept
        {
            return *m_copyable_wrapper;
        }
        constexpr const T& operator*() const& noexcept
        {
            return *m_copyable_wrapper;
        }
        constexpr T&& operator*() && noexcept
        {
            return AZStd::move(*m_copyable_wrapper);
        }
        constexpr const T&& operator*() const&& noexcept
        {
            return AZStd::move(*m_copyable_wrapper);
        }

        constexpr T* operator->() noexcept
        {
            return m_copyable_wrapper.operator->();
        }
        constexpr const T* operator->() const noexcept
        {
            return m_copyable_wrapper.operator->();
        }

        constexpr explicit operator bool()
        {
            return m_copyable_wrapper.has_value();
        }

        template<class... Args>
        constexpr T& emplace(Args&&... args)
        {
            return m_copyable_wrapper.emplace(AZStd::forward<Args>(args)...);
        }

        constexpr void reset() noexcept
        {
            m_copyable_wrapper.reset();
        }
    private:
        optional<T> m_copyable_wrapper;
    };

    // Non-propagating cache - https://eel.is/c++draft/ranges#range.nonprop.cache
    // Enables an input view to temporarily cache values over as it is iterated over.
    // For copy construction, a current instance is default initialized
    // For move construction, the input object is reset
    // On copy assignment, the current instance is reset
    // Onmove assignment, both the current and input are reset

    template<class T, class = void>
    class non_propagating_cache;

    template<class T>
    class non_propagating_cache<T, enable_if_t<is_object_v<T>>>
    {
    public:
        template<class U = T, class = enable_if_t<default_initializable<U>>>
        constexpr non_propagating_cache() noexcept(is_nothrow_constructible_v<T>)
            : non_propagating_cache{ in_place }
        {}

        template<class... Args>
        explicit constexpr non_propagating_cache(in_place_t, Args&&... args) noexcept
            : m_cache{ in_place, AZStd::forward<Args>(args)... }
        {}

        // Purposely does not initialize the wrapper
        constexpr non_propagating_cache(const non_propagating_cache&)
        {}
        constexpr non_propagating_cache(non_propagating_cache&& other)
        {
            other.reset();
        }

        constexpr auto operator=(const non_propagating_cache& other) noexcept
        {
            if (this != &other)
            {
                reset();
            }

            return *this;
        }

        constexpr auto operator=(non_propagating_cache&& other) noexcept
        {
            reset();
            other.reset();
            return *this;
        }

        constexpr T& operator*() & noexcept
        {
            return *m_cache;
        }
        constexpr const T& operator*() const& noexcept
        {
            return *m_cache;
        }
        constexpr T&& operator*() && noexcept
        {
            return AZStd::move(*m_cache);
        }
        constexpr const T&& operator*() const&& noexcept
        {
            return AZStd::move(*m_cache);
        }

        constexpr T* operator->() noexcept
        {
            return m_cache.operator->();
        }
        constexpr const T* operator->() const noexcept
        {
            return m_cache.operator->();
        }

        constexpr explicit operator bool()
        {
            return m_cache.has_value();
        }

        template<class... Args>
        constexpr T& emplace(Args&&... args)
        {
            return m_cache.emplace(AZStd::forward<Args>(args)...);
        }
        template<class I>
        constexpr auto emplace_deref(const I& i) -> enable_if_t<constructible_from<T, decltype(*i)>, T&>
        {
            return m_cache.emplace(*i);
        }

        constexpr void reset() noexcept
        {
            m_cache.reset();
        }
    private:
        optional<T> m_cache;
    };
}  // namespace AZStd::ranges::Internal
