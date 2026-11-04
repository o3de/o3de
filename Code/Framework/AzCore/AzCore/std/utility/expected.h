/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/typetraits/is_swappable.h>
#include <AzCore/std/utility/expected_internal.h>

namespace AZStd
{
    //! Wraps the unexpected value in a type that distinguishes the unexpected value
    //! from the expected value.
    //! This avoids ambiguity when constructing an expected where value type and error type are the same
    //! i.e expected<int, int>
    template<class E>
    class unexpected
    {
    public:
        constexpr unexpected(const unexpected&) = default;
        constexpr unexpected(unexpected&&) = default;
        template<class Err>
            requires (!is_same_v<remove_cvref_t<Err>, unexpected<E>>)
                && (!is_same_v<remove_cvref_t<Err>, in_place_t>)
                && is_constructible_v<E, Err>
        constexpr explicit unexpected(Err&& err)
            : m_unexpected(AZStd::forward<Err>(err))
        {
        }
        template<class... Args>
            requires is_constructible_v<E, Args...>
        constexpr explicit unexpected(in_place_t, Args&&... args)
            : m_unexpected{ AZStd::forward<Args>(args)... }
        {
        }

        template<class U, class... Args>
            requires is_constructible_v<E, initializer_list<U>&, Args...>
        constexpr explicit unexpected(in_place_t, initializer_list<U> il, Args&&... args)
            : m_unexpected{ il, AZStd::forward<Args>(args)... }
        {
        }

        constexpr unexpected& operator=(const unexpected&) = default;
        constexpr unexpected& operator=(unexpected&&) = default;

        constexpr const E& error() const& noexcept
        {
            return m_unexpected;
        }

        constexpr E& error() & noexcept
        {
            return m_unexpected;
        }

        constexpr const E&& error() const&& noexcept
        {
            return AZStd::move(m_unexpected);
        }

        constexpr E&& error() && noexcept
        {
            return AZStd::move(m_unexpected);
        }

        constexpr void swap(unexpected& other) noexcept(is_nothrow_swappable_v<E>)
        {
            using AZStd::swap;
            swap(m_unexpected, other.m_unexpected);
        }

        template<class E2>
        friend constexpr bool operator==(const unexpected&, const unexpected<E2>&);

        template<class E2>
        friend constexpr void swap(unexpected<E2>& x, unexpected<E2>& y) noexcept(noexcept(x.swap(y)));

    private:
        E m_unexpected;
    };

    template<class E>
    unexpected(E) -> unexpected<E>;

    template<class E, class E2>
    constexpr bool operator==(const unexpected<E>& x, const unexpected<E2>& y)
    {
        static_assert(
            Internal::boolean_testable<decltype(x.error() == y.error())>,
            "Cannot invoke operator= on"
            " error types as they are not comparable");
        return x.error() == y.error();
    }

    template<class E, class E2>
    constexpr bool operator!=(const unexpected<E>& x, const unexpected<E2>& y)
    {
        return !operator==(x, y);
    }

    template<class E>
    constexpr void swap(unexpected<E>& x, unexpected<E>& y) noexcept(noexcept(x.swap(y)))
    {
        static_assert(is_swappable_v<E>, "Cannot invoke swap with an error type that is not swappable");
        x.swap(y);
    }

    //! Implementation of C++23 [std::expected](https://en.cppreference.com/w/cpp/utility/expected)
    //! The AZ::Outcome class inherits from it with a few differences to the initial state
    //! AZStd::expected default constructs with the expected(success) value.
    //! AZStd::expected supports operator*, operator-> and emplace functions
    //! AZStd::expected does NOT support an error type of void, AZStd::optional<T> should be used instead
    //! AZ::Outcome default constructs with the failure(unexpected) value.
    //! AZ::Outcome also supports void error type, which is not needed, as in that case an AZStd::optional is a better choice
    //! differences from std::expected
    //! Since O3DE doesn't use exceptions, noexcept isn't used throughout the code. Therefore the constraints
    //! on needing types to be noexcept are not in the AZStd::expected implementation
    template <class T, class E>
    class expected
        : Internal::expected_storage_move_assignment<T, E>
    {
        using base_type = Internal::expected_storage_move_assignment<T, E>;
    public:
        using value_type = T;
        using error_type = E;
        using unexpected_type = unexpected<E>;
        template<class U>
        using rebind = expected<U, error_type>;

        // Default constructor for expected which is only available in overload resolution
        // if the expected type is void or it is default constructible
        constexpr expected() noexcept
            requires is_void_v<T>
                || is_default_constructible_v<T>
            : base_type{ in_place }
        {
        }

        //! copy constructor - Defaults to the `expected_storage_copy_constructor` for constraint checks
        constexpr expected(const expected& rhs) = default;

        //! move constructor - Defaults to the `expected_storage_move_constructor` for constraint checks
        constexpr expected(expected&& rhs) = default;

        //! Direct initialization copy constructor
        //! Allows construction of an expected from a type that can direct initialize T
        template<class U, class G>
            requires ((is_void_v<T> && is_void_v<U>) || is_constructible_v<T, add_lvalue_reference_t<const U>>)
                && is_constructible_v<E, const G&>
                && Internal::not_convertible_or_constructible_from_other_std_expected_v<T, E, U, G>
#if !defined(O3DE_DISABLE_CONDITIONAL_EXPLICIT) && __cpp_conditional_explicit >= 201806L
        explicit(!is_convertible_v<add_lvalue_reference_t<const U>, T> || !is_convertible_v<const G&, E>)
#endif
        constexpr expected(const expected<U, G>& rhs)
        {
            if (rhs.has_value())
            {
                if constexpr (!is_void_v<T>)
                {
                    AZStd::construct_at(addressof(this->m_storage.m_value), AZStd::forward<const U&>(rhs.value()));
                }
            }
            else
            {
                AZStd::construct_at(addressof(this->m_storage.m_unexpected), AZStd::forward<const G&>(rhs.error()));
            }

            this->m_hasValue = rhs.has_value();
        }

        //! Direct initialization move constructor
        //! The first `class =` parameter check the condition for whether the constructor should be explicit
        //! The second `class=` paramter checks the constraints on the constructor
        template<class U, class G>
            requires ((is_void_v<T> && is_void_v<U>) || is_constructible_v<T, U>)
                && is_constructible_v<E, G>
                && Internal::not_convertible_or_constructible_from_other_std_expected_v<T, E, U, G>
#if !defined(O3DE_DISABLE_CONDITIONAL_EXPLICIT) && __cpp_conditional_explicit >= 201806L
        explicit(!is_convertible_v<U, T> || !is_convertible_v<G, E>)
#endif
        constexpr expected(expected<U, G>&& rhs)
        {
            if (rhs.has_value())
            {
                if constexpr (!is_void_v<T>)
                {
                    AZStd::construct_at(addressof(this->m_storage.m_value), AZStd::forward<U>(rhs.value()));
                }
            }
            else
            {
                AZStd::construct_at(addressof(this->m_storage.m_unexpected), AZStd::forward<G>(rhs.error()));
            }

            this->m_hasValue = rhs.has_value();
        }

        // Direct non-list initialization for value type from U
        template<class U>
            requires (!is_void_v<T>)
                && (!is_same_v<remove_cvref_t<U>, in_place_t>)
                && (!is_same_v<expected<T, E>, remove_cvref_t<U>>)
                && (!Internal::is_std_unexpected_specialization_v<remove_cvref_t<U>>)
                && is_constructible_v<T, U>
#if !defined(O3DE_DISABLE_CONDITIONAL_EXPLICIT) && __cpp_conditional_explicit >= 201806L
        explicit(!is_convertible_v<U, T>)
#endif
        constexpr expected(U&& v)
            : base_type{ in_place, AZStd::forward<U>(v) }
        {
        }

        // Direct non-list initialization for error type
        template<class G>
            requires is_constructible_v<E, const G&>
#if !defined(O3DE_DISABLE_CONDITIONAL_EXPLICIT) && __cpp_conditional_explicit >= 201806L
        explicit(!is_convertible_v<const G&, E>)
#endif
        constexpr expected(const unexpected<G>& err)
            : base_type{ unexpect, AZStd::forward<const G&>(err.error()) }
        {
        }


        template<class G>
            requires is_constructible_v<E, G>
#if !defined(O3DE_DISABLE_CONDITIONAL_EXPLICIT) && __cpp_conditional_explicit >= 201806L
        explicit(!is_convertible_v<G, E>)
#endif
        constexpr expected(unexpected<G>&& err)
            : base_type{ unexpect, AZStd::forward<G>(err.error()) }
        {
        }


        //! Direct non-list initialization for expected type with variadic arguments
        template<class... Args>
            requires (!is_void_v<T>)
                && is_constructible_v<T, Args...>
        constexpr explicit expected(in_place_t, Args&&... args)
            : base_type{ in_place, AZStd::forward<Args>(args)... }
        {
        }

        template<class U, class... Args>
            requires (!is_void_v<T>)
                && is_constructible_v<T, initializer_list<U>&, Args...>
        constexpr explicit expected(in_place_t, initializer_list<U> il, Args&&...args)
            : base_type{ in_place, il, AZStd::forward<Args>(args)... }
        {
        }

        //! expected<void, E> specialization for in-place constructor
        template<bool = true>
        constexpr explicit expected(in_place_t)
            requires is_void_v<T>
            : expected{}
        {
        }

        //! Direct non-list initialization for error type with variadic arguments
        template<class... Args>
            requires is_constructible_v<E, Args...>
        constexpr explicit expected(unexpect_t, Args&&... args)
            : base_type{ unexpect, AZStd::forward<Args>(args)... }
        {
        }

        template<class U, class... Args>
            requires is_constructible_v<E, initializer_list<U>&, Args...>
        constexpr explicit expected(unexpect_t, initializer_list<U> il, Args&&...args)
            : base_type{ unexpect, il, AZStd::forward<Args>(args)... }
        {
        }

        //! destructor - Defaults to the `expected_union` for constraint checks
        ~expected() = default;

        //! Copy-assignment from other expected.
        constexpr expected& operator=(const expected& rhs) = default;

        //! Move-assignment from other expected.
        constexpr expected& operator=(expected&& rhs) noexcept(
            is_nothrow_move_assignable_v<T> && is_nothrow_move_constructible_v<T>
            && is_nothrow_move_assignable_v<E> && is_nothrow_move_constructible_v<E>) = default;

        //! Direct initializes value into expected.
        template<class U>
            requires (!is_void_v<T>)
                && (!is_same_v<expected<T, E>, remove_cvref_t<U>>)
                && (!Internal::is_std_unexpected_specialization_v<remove_cvref_t<U>>)
                && is_constructible_v<T, U>
                && is_assignable_v<add_lvalue_reference_t<T>, U>
        constexpr auto operator=(U&& value) -> expected&
        {
            if (has_value())
            {
                // direct intialize from value
                this->m_storage.m_value = AZStd::forward<U>(value);
            }
            else
            {
                // destroy the error type and direct initialize the value type
                Internal::reinit_expected(this->m_storage.m_value, this->m_storage.m_unexpected, AZStd::forward<U>(value));
                this->m_hasValue = true;
            }

            return *this;
        }

        //! Copy error into expected.
        template<class G>
            requires is_constructible_v<E, const G&>
                && is_assignable_v<E&, const G&>
        constexpr auto operator=(const unexpected<G>& error) -> expected&
        {
            if (!has_value())
            {
                // direct intialize from error
                this->m_storage.m_unexpected = AZStd::forward<const G&>(error.error());
            }
            else
            {
                // destroy the value type and copy the error type
                if constexpr (!is_void_v<T>)
                {
                    Internal::reinit_expected(this->m_storage.m_unexpected, this->m_storage.m_value, AZStd::forward<const G&>(error.error()));
                }
                else
                {
                    AZStd::construct_at(addressof(this->m_storage.m_unexpected), AZStd::forward<const G&>(error.error()));
                }
                this->m_hasValue = false;
            }

            return *this;
        }

        //! Move error into expected.
        template<class G>
            requires is_constructible_v<E, G>
                && is_assignable_v<E&, G>
        constexpr auto operator=(unexpected<G>&& error) -> expected&
        {
            if (!has_value())
            {
                // direct intialize from error
                this->m_storage.m_unexpected = AZStd::forward<G>(error.error());
            }
            else
            {
                // destroy the value type and move the error type
                if constexpr (!is_void_v<T>)
                {
                    Internal::reinit_expected(this->m_storage.m_unexpected, this->m_storage.m_value, AZStd::forward<G>(error.error()));
                }
                else
                {
                    AZStd::construct_at(addressof(this->m_storage.m_unexpected), AZStd::forward<G>(error.error()));
                }
                this->m_hasValue = false;
            }

            return *this;
        }

        //! emplace overloads for when T is not a void type
        template<class... Args>
        constexpr decltype(auto) emplace(Args&&... args) noexcept
        {
            static_assert(!is_void_v<T> && is_constructible_v<T, Args...>);
            if (has_value())
            {
                AZStd::destroy_at(addressof(this->m_storage.m_value));
            }
            else
            {
                AZStd::destroy_at(addressof(this->m_storage.m_unexpected));
                this->m_hasValue = true;
            }

            return *AZStd::construct_at(addressof(this->m_storage.m_value), AZStd::forward<Args>(args)...);
        }

        template<class U, class... Args>
        constexpr decltype(auto) emplace(initializer_list<U> il, Args&&... args) noexcept
        {
            static_assert(!is_void_v<T>&& is_constructible_v<T, initializer_list<U>&, Args...>);
            if (has_value())
            {
                AZStd::destroy_at(addressof(this->m_storage.m_value));
            }
            else
            {
                AZStd::destroy_at(addressof(this->m_storage.m_unexpected));
                this->m_hasValue = true;
            }

            return *AZStd::construct_at(addressof(this->m_storage.m_value), il, AZStd::forward<Args>(args)...);
        }

        //! emplace overload for when T is void
        constexpr void emplace() noexcept
            requires is_void_v<T>
        {
            if (!has_value())
            {
                AZStd::destroy_at(addressof(this->m_storage.m_unexpected));
                this->m_hasValue = true;
            }
        }

        // [expected.object.swap], swap
        constexpr auto swap(expected& rhs) noexcept(is_nothrow_move_constructible_v<T>
            && is_nothrow_swappable_v<T>
            && is_nothrow_move_constructible_v<E>
            && is_nothrow_swappable_v<E>) -> void
            requires (!is_void_v<T>)
                && is_swappable_v<T>
                && is_swappable_v<E>
                && is_move_constructible_v<T>
                && is_move_constructible_v<E>
        {
            if (has_value())
            {
                if (rhs.has_value())
                {
                    // Both expected have a value so swap the value instance
                    using AZStd::swap;
                    swap(this->m_storage.m_value, rhs.m_storage.m_value);
                }
                else
                {
                    // Swap the value type from this instance with the error type from the other
                    if constexpr (is_nothrow_move_constructible_v<E>)
                    {
                        E tmp(AZStd::move(rhs.m_storage.m_unexpected));
                        AZStd::destroy_at(addressof(rhs.m_storage.m_unexpected));
                        AZStd::construct_at(addressof(rhs.m_storage.m_value), AZStd::move(this->m_storage.m_value));
                        AZStd::destroy_at(addressof(this->m_storage.m_value));
                        AZStd::construct_at(addressof(this->m_storage.m_unexpected), AZStd::move(tmp));
                    }
                    else
                    {
                        T tmp(AZStd::move(this->m_storage.m_value));
                        AZStd::destroy_at(addressof(this->m_storage.m_value));
                        AZStd::construct_at(addressof(this->m_storage.m_unexpected), AZStd::move(rhs.m_storage.m_unexpected));
                        AZStd::destroy_at(addressof(rhs.m_storage.m_unexpected));
                        AZStd::construct_at(addressof(rhs.m_storage.m_value), AZStd::move(tmp));
                    }

                    this->m_hasValue = false;
                    rhs.m_hasValue = true;
                }
            }
            else
            {
                if (rhs.has_value())
                {
                    // Call swap on the rhs to have the (*this->m_storage.has_value() && !rhs.has_value()) case above triggered
                    rhs.swap(*this);
                }
                else
                {
                    // Neither expected has a value so swap the unexpected instance
                    using AZStd::swap;
                    swap(this->m_storage.m_unexpected, rhs.m_storage.m_unexpected);
                }
            }
        }

        // swap internal
        friend constexpr void swap(expected& x, expected& y) noexcept(noexcept(x.swap(y)))
        {
            x.swap(y);
        }

        // [expected.object.obs], observers
        //! Returns whether the expected contains a value type(true) or error type(false)
        constexpr explicit operator bool() const noexcept
        {
            return this->m_hasValue;
        }

        constexpr bool has_value() const noexcept
        {
            return this->m_hasValue;
        }

        constexpr auto operator->() const noexcept -> const T*
            requires (!is_void_v<T>)
        {
            AZ_Assert(has_value(), "expected object doesn't have a value, a pointer to unitialized value type will be returned");
            return addressof(this->m_storage.m_value);
        }

        constexpr auto operator->() noexcept -> T*
            requires (!is_void_v<T>)
        {
            AZ_Assert(has_value(), "expected object doesn't have a value, a pointer to unitialized value type will be returned");
            return addressof(this->m_storage.m_value);
        }

        // Overloads when T is not a void type
        constexpr auto operator*() const & noexcept -> add_lvalue_reference_t<const T>
            requires (!is_void_v<T>)
        {
            AZ_Assert(has_value(), "expected doesn't have a value, uninitalized data will be returned");
            return this->m_storage.m_value;
        }

        constexpr auto operator*() & noexcept -> add_lvalue_reference_t<T>
            requires (!is_void_v<T>)
        {
            AZ_Assert(has_value(), "expected doesn't have a value, uninitalized data will be returned");
            return this->m_storage.m_value;
        }

        constexpr auto operator*() const && noexcept -> add_rvalue_reference_t<const T>
            requires (!is_void_v<T>)
        {
            AZ_Assert(has_value(), "expected doesn't have a value, uninitalized data will be returned");
            return AZStd::move(this->m_storage.m_value);
        }

        constexpr auto operator*() && noexcept -> add_rvalue_reference_t<T>
            requires (!is_void_v<T>)
        {
            AZ_Assert(has_value(), "expected doesn't have a value, uninitalized data will be returned");
            return AZStd::move(this->m_storage.m_value);
        }

        constexpr auto value() const & -> add_lvalue_reference_t<const T>
            requires (!is_void_v<T>)
        {
            AZ_Assert(has_value(), "expected doesn't have a value, uninitalized data will be returned");
            return this->m_storage.m_value;
        }

        constexpr auto value() & -> add_lvalue_reference_t<T>
            requires (!is_void_v<T>)
        {
            AZ_Assert(has_value(), "expected doesn't have a value, uninitalized data will be returned");
            return this->m_storage.m_value;
        }

        constexpr auto value() const && -> add_rvalue_reference_t<const T>
            requires (!is_void_v<T>)
        {
            AZ_Assert(has_value(), "expected doesn't have a value, uninitalized data will be returned");
            return AZStd::move(this->m_storage.m_value);
        }

        constexpr auto value() && -> add_rvalue_reference_t<T>
            requires (!is_void_v<T>)
        {
            AZ_Assert(has_value(), "expected doesn't have a value, uninitalized data will be returned");
            return AZStd::move(this->m_storage.m_value);
        }

        // Overloads when T is a void type
        constexpr void operator*() const & noexcept
            requires is_void_v<T>
        {
            AZ_Assert(has_value(), "expected doesn't have a value, void will be returned");
        }

        constexpr void operator*() const && noexcept
            requires is_void_v<T>
        {
            AZ_Assert(has_value(), "expected doesn't have a value, void will be returned");
        }

        constexpr void value() const &
            requires is_void_v<T>
        {
            AZ_Assert(has_value(), "expected doesn't have a value, uninitalized data will be returned");
        }

        constexpr void value() &&
            requires is_void_v<T>
        {
            AZ_Assert(has_value(), "expected doesn't have a value, uninitalized data will be returned");
        }

        // Error overloads
        constexpr const E& error() const & noexcept
        {
            AZ_Assert(!has_value(), "expected has have a value, an error type cannot be returned");
            return this->m_storage.m_unexpected;

        }

        constexpr E& error() & noexcept
        {
            AZ_Assert(!has_value(), "expected has have a value, an error type cannot be returned");
            return this->m_storage.m_unexpected;
        }

        constexpr const E&& error() const && noexcept
        {
            AZ_Assert(!has_value(), "expected has have a value, an error type cannot be returned");
            return AZStd::move(this->m_storage.m_unexpected);
        }

        constexpr E&& error() && noexcept
        {
            AZ_Assert(!has_value(), "expected has have a value, an error type cannot be returned");
            return AZStd::move(this->m_storage.m_unexpected);
        }

        template<class U>
            requires (!is_void_v<T>)
        constexpr auto value_or(U&& value) const& -> T
        {
            static_assert(is_copy_constructible_v<T> && is_convertible_v<U, T>);
            return has_value() ? (**this) : static_cast<T>(AZStd::forward<U>(value));
        }

        template<class U>
            requires (!is_void_v<T>)
        constexpr auto value_or(U&& value) && -> T
        {
            static_assert(is_move_constructible_v<T> && is_convertible_v<U, T>);
            return has_value() ? AZStd::move(**this) : static_cast<T>(AZStd::forward<U>(value));
        }

        // [expected.object.eq], equality operators
        template<class T2, class E2>
        friend constexpr bool operator==(const expected& x, const expected<T2, E2>& y)
        {
            return x.compare_equal_internal(y);
        }
        template<class T2, class E2>
        friend constexpr bool operator!=(const expected& x, const expected<T2, E2>& y)
        {
            return !operator==(x, y);
        }

        template<class T2>
        friend constexpr bool operator==(const expected& x, const T2& value)
        {
            static_assert(!is_void_v<T> && !is_void_v<T2>);
            return x.compare_equal_internal(value);
        }
        template<class T2>
        friend constexpr bool operator!=(const expected& x, const T2& value)
        {
            static_assert(!is_void_v<T> && !is_void_v<T2>);
            return !operator==(x, value);
        }

        template<class E2>
        friend constexpr bool operator==(const expected& x, const unexpected<E2>& err)
        {
            return x.compare_equal_internal(err);
        }
        template<class E2>
        friend constexpr bool operator!=(const expected& x, const unexpected<E2>& err)
        {
            return !operator==(x, err);
        }

    private:
        template<class T2, class E2>
        constexpr bool compare_equal_internal(const expected<T2, E2>& y) const
        {
            if (has_value() != y.has_value())
            {
                return false;
            }

            if constexpr (is_void_v<T>)
            {
                // Both arguments must be void in this case
                static_assert(is_void_v<T2>, "Both expected objects value_type must be void");
                return has_value() || error() == y.error();
            }
            else
            {
                static_assert(!is_void_v<T2>, "Neither expected objects value_type can be void");
                return has_value() ? *this == *y : error() == y.error();
            }
        }

        template<class T2>
        constexpr bool compare_equal_internal(const T2& value) const
        {
            return has_value() && bool(**this == value);
        }

        template<class E2>
        constexpr bool compare_equal_internal(const unexpected<E2>& err) const
        {
            return !has_value() && bool(error() == err.error());
        }
    };


} // namespace AZStd
