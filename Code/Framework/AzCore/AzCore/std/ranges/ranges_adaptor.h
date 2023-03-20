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
#include <AzCore/std/tuple.h>

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

    // Return a closure in which the outer range adaptor invokes the inner range adaptor
    template<class RangeAdaptorFunctor>
    struct range_adaptor_closure_forwarder
        : RangeAdaptorFunctor
        , range_adaptor_closure<range_adaptor_closure_forwarder<RangeAdaptorFunctor>>
    {
        constexpr explicit range_adaptor_closure_forwarder(RangeAdaptorFunctor&& closure)
            : RangeAdaptorFunctor{ AZStd::forward<RangeAdaptorFunctor>(closure) }
        {}
    };

    // Returns a adaptor in which range_adaptor_closure in which a set of bound arguments can
    // be forwarded to a supplied closure
    template<class Adaptor, class... Args>
    struct range_adaptor_argument_forwarder
        : range_adaptor_closure<range_adaptor_argument_forwarder<Adaptor, Args...>>
    {
        template<class UAdaptor, class... UArgs, class = enable_if_t<
            convertible_to<UAdaptor, Adaptor>
            && convertible_to<tuple<UArgs...>, tuple<Args...>>
            >>
        constexpr explicit range_adaptor_argument_forwarder(UAdaptor adaptor, UArgs&&... args)
            : m_adaptor{ AZStd::forward<UAdaptor>(adaptor) }
            , m_forwardArgs{ AZStd::forward<UArgs>(args)... }
        {}

        template<class Range>
        constexpr decltype(auto) operator()(Range&& range) &
        {
            auto ForwardRangeAndArgs = [&adpator = m_adaptor, &range](auto&... args)
            {
                return adaptor(AZStd::forward<Range>(range), args...);
            };
            return AZStd::apply(ForwardRangeAndArgs, m_forwardArgs);
        }
        template<class Range>
        constexpr decltype(auto) operator()(Range&& range) const&
        {
            auto ForwardRangeAndArgs = [&adpator = m_adaptor, &range](const auto&... args)
            {
                return adaptor(AZStd::forward<Range>(range), args...);
            };
            return AZStd::apply(ForwardRangeAndArgs, m_forwardArgs);
        }
        // rvalue overloads
        template<class Range>
        constexpr decltype(auto) operator()(Range&& range) &&
        {
            auto ForwardRangeAndArgs = [&adaptor = m_adaptor, &range](auto&... args)
            {
                return adaptor(AZStd::forward<Range>(range), AZStd::move(args)...);
            };
            return AZStd::apply(ForwardRangeAndArgs, m_forwardArgs);
        }
        // It doesn't make sense to move invoke a const range_adaptor_argument_forwarder
        // that has been moved.
        template<class Range>
        constexpr decltype(auto) operator()(Range&& range) const&& = delete;
    private:
        Adaptor m_adaptor;
        tuple<Args...> m_forwardArgs;
    };

    // deduction guide - range_adpator_argument_forwarder
    template<class Adaptor, class... Args>
    range_adaptor_argument_forwarder(Adaptor&&, Args&&...)
        -> range_adaptor_argument_forwarder<AZStd::decay_t<Adaptor>, AZStd::decay_t<Args>...>;


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
    // Movable Wrapper - https://eel.is/c++draft/ranges#range.copy.wrap
    // Used to wrap a class that is copy constructible/move constructible,
    // but not necessarily copy assignable/move assignable
    // and implements the assignment operator using the optional emplace function
    // to construct in place
    template<class T, class = void>
    class movable_box;


    template<class T>
    class movable_box<T, enable_if_t<move_constructible<T>&& is_object_v<T>>>
    {
    public:
        template<class U = T, class = enable_if_t<default_initializable<U>>>
        constexpr movable_box() noexcept(is_nothrow_constructible_v<T>)
            : movable_box{ in_place }
        {}

        template<class... Args>
        explicit constexpr movable_box(in_place_t, Args&&... args) noexcept(is_nothrow_constructible_v<T>)
            : m_value{ in_place, AZStd::forward<Args>(args)... }
        {}

        explicit constexpr movable_box(const T& rawValue) noexcept(is_nothrow_copy_constructible_v<T>)
            : m_value{ rawValue }
        {}
        explicit constexpr movable_box(T&& rawValue) noexcept(is_nothrow_copy_constructible_v<T>)
            : m_value{ AZStd::move(rawValue) }
        {}

        constexpr movable_box(const movable_box&) = default;
        constexpr movable_box(movable_box&&) = default;

        // requires that the T is copy_constructible to implement copy assignment
        constexpr auto operator=(const movable_box& other) noexcept(is_nothrow_copy_constructible_v<T>)
            -> enable_if_t<copy_constructible<T>, movable_box&>
        {
            if (this != addressof(other))
            {
                if constexpr (copyable<T>)
                {
                    m_value = other.m_value;
                }
                else
                {
                    if (other)
                    {
                        emplace(other.m_value);
                    }
                    else
                    {
                        reset();
                    }
                }
            }

            return *this;
        }

        constexpr movable_box& operator=(movable_box&& other) noexcept(is_nothrow_move_constructible_v<T>)
        {
            if (this != addressof(other))
            {
                if constexpr (movable<T>)
                {
                    m_value = AZStd::move(other.m_value);
                }
                else
                {
                    if (other)
                    {
                        emplace(AZStd::move(other.m_value));
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
            return *m_value;
        }
        constexpr const T& operator*() const& noexcept
        {
            return *m_value;
        }
        constexpr T&& operator*() && noexcept
        {
            return AZStd::move(*m_value);
        }
        constexpr const T&& operator*() const&& noexcept
        {
            return AZStd::move(*m_value);
        }

        constexpr T* operator->() noexcept
        {
            return m_value.operator->();
        }
        constexpr const T* operator->() const noexcept
        {
            return m_value.operator->();
        }

        constexpr explicit operator bool()
        {
            return m_value.has_value();
        }

        template<class... Args>
        constexpr T& emplace(Args&&... args)
        {
            return m_value.emplace(AZStd::forward<Args>(args)...);
        }

        constexpr void reset() noexcept
        {
            m_value.reset();
        }
    private:
        optional<T> m_value;
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
