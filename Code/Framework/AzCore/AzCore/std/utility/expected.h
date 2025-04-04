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
        template<class Err = E, class = enable_if_t<!is_same_v<remove_cvref_t<Err>, unexpected>
            && !is_same_v<remove_cvref_t<Err>, in_place_t>&& is_constructible_v<E, Err>>>
        constexpr explicit unexpected(Err&&);
        template<class... Args, class = enable_if_t<is_constructible_v<E, Args...>>>
        constexpr explicit unexpected(in_place_t, Args&&...);
        template<class U, class... Args, class = enable_if_t<is_constructible_v<E, initializer_list<U>&, Args...>>>
        constexpr explicit unexpected(in_place_t, initializer_list<U>, Args&&...);

        constexpr unexpected& operator=(const unexpected&) = default;
        constexpr unexpected& operator=(unexpected&&) = default;

        constexpr const E& error() const& noexcept;
        constexpr E& error() & noexcept;
        constexpr const E&& error() const&& noexcept;
        constexpr E&& error() && noexcept;

        constexpr void swap(unexpected& other) noexcept(is_nothrow_swappable_v<E>);

        template<class E2>
        friend constexpr bool operator==(const unexpected&, const unexpected<E2>&);

        template<class E2>
        friend constexpr void swap(unexpected<E2>& x, unexpected<E2>& y) noexcept(noexcept(x.swap(y)));

    private:
        E m_unexpected;
    };

    template<class E>
    unexpected(E) -> unexpected<E>;

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
        template <bool Enable = is_void_v<T> || is_default_constructible_v<T>, class = enable_if_t<Enable>>
        constexpr expected() noexcept;

        //! copy constructor - Defaults to the `expected_storage_copy_constructor` for constraint checks
        constexpr expected(const expected& rhs) = default;

        //! move constructor - Defaults to the `expected_storage_move_constructor` for constraint checks
        constexpr expected(expected&& rhs) = default;

        //! Direct initialization copy constructor
        //! Allows construction of an expected from a type that can direct initialize T
        template<class U, class G, class = enable_if_t<
            /* constraint checks */ ((is_void_v<T> && is_void_v<U>) || is_constructible_v<T, add_lvalue_reference_t<const U>>)
            && is_constructible_v<E, const G&>
            && Internal::not_convertible_or_constructible_from_other_std_expected_v<T, E, U, G>>>
#if __cpp_conditional_explicit >= 201806L
        explicit(!is_convertible_v<add_lvalue_reference_t<const U>, T> || !is_convertible_v<const G&, E>)
#endif
        constexpr expected(const expected<U, G>& rhs);

        //! Direct initialization move constructor
        //! The first `class =` parameter check the condition for whether the constructor should be explicit
        //! The second `class=` paramter checks the constraints on the constructor
        template<class U, class G, class = enable_if_t<
            /* constraint checks */ ((is_void_v<T> && is_void_v<U>) || is_constructible_v<T, U>)
            && is_constructible_v<E, G>
            && Internal::not_convertible_or_constructible_from_other_std_expected_v<T, E, U, G>>>
#if __cpp_conditional_explicit >= 201806L
        explicit(!is_convertible_v<U, T> || !is_convertible_v<G, E>)
#endif
        constexpr expected(expected<U, G>&& rhs);

        // Direct non-list initialization for value type from U
        template<class U = T, class = enable_if_t<!is_void_v<T>
            && !is_same_v<remove_cvref_t<U>, in_place_t>
            && !is_same_v<expected, remove_cvref_t<U>>
            && !Internal::is_std_unexpected_specialization_v<remove_cvref_t<U>>
            && is_constructible_v<T, U> >>
#if __cpp_conditional_explicit >= 201806L
        explicit(!is_convertible_v<U, T>)
#endif
        constexpr expected(U&& v);

        // Direct non-list initialization for error type
        template<class G, class = enable_if_t<is_constructible_v<E, const G&> >>
#if __cpp_conditional_explicit >= 201806L
        explicit(!is_convertible_v<const G&, E>)
#endif
        constexpr expected(const unexpected<G>& err);


        template<class G, class = enable_if_t<is_constructible_v<E, G> >>
#if __cpp_conditional_explicit >= 201806L
        explicit(!is_convertible_v<G, E>)
#endif
        constexpr expected(unexpected<G>&& err);


        //! Direct non-list initialization for expected type with variadic arguments
        template<class... Args, class = enable_if_t<!is_void_v<T>&& is_constructible_v<T, Args...>, int>>
        constexpr explicit expected(in_place_t, Args&&... args);

        template<class U, class... Args, class = enable_if_t<!is_void_v<T>&& is_constructible_v<T, initializer_list<U>&, Args...>, int>>
        constexpr explicit expected(in_place_t, initializer_list<U> il, Args&&...args);

        //! expected<void, E> specialization for in-place constructor
        template<bool Enable = is_void_v<T>, class = enable_if_t<Enable>>
        constexpr explicit expected(in_place_t);

        //! Direct non-list initialization for error type with variadic arguments
        template<class... Args, class = enable_if_t<is_constructible_v<E, Args...>, int>>
        constexpr explicit expected(unexpect_t, Args&&... args);

        template<class U, class... Args, class = enable_if_t<is_constructible_v<E, initializer_list<U>&, Args...>, int>>
        constexpr explicit expected(unexpect_t, initializer_list<U> il, Args&&...args);

        //! destructor - Defaults to the `expected_union` for constraint checks
        ~expected() = default;

        //! Copy-assignment from other expected.
        constexpr expected& operator=(const expected& rhs) = default;

        //! Move-assignment from other expected.
        constexpr expected& operator=(expected&& rhs) noexcept(
            is_nothrow_move_assignable_v<T> && is_nothrow_move_constructible_v<T>
            && is_nothrow_move_assignable_v<E> && is_nothrow_move_constructible_v<E>) = default;

        //! Direct initializes value into expected.
        template<class U = T, class = enable_if_t<!is_void_v<T>
            && !is_same_v<expected, remove_cvref_t<U>>
            && !Internal::is_std_unexpected_specialization_v<remove_cvref_t<U>>
            && is_constructible_v<T, U>
            && is_assignable_v<add_lvalue_reference_t<T>, U>>>
        constexpr auto operator=(U&& value) -> expected&;

        //! Copy error into expected.
        template<class G>
        constexpr auto operator=(const unexpected<G>& error) -> enable_if_t<is_constructible_v<E, const G&>
            && is_assignable_v<E&, const G&>,
            expected&>;

        //! Move error into expected.
        template<class G>
        constexpr auto operator=(unexpected<G>&& error) -> enable_if_t<is_constructible_v<E, G>
            && is_assignable_v<E&, G>,
            expected&>;

        //! emplace overloads for when T is not a void type
        template<class... Args>
        constexpr decltype(auto) emplace(Args&&...) noexcept;
        template<class U, class... Args>
        constexpr decltype(auto) emplace(initializer_list<U>, Args&&...) noexcept;

        //! emplace overload for when T is void
        template<bool Enable = is_void_v<T>, class = enable_if_t<Enable>>
        constexpr void emplace() noexcept;

        // [expected.object.swap], swap
        template<bool Enable = !is_void_v<T> && is_swappable_v<T> && is_swappable_v<E>
            && is_move_constructible_v<T> && is_move_constructible_v<E>>
        constexpr auto swap(expected& rhs) noexcept(is_nothrow_move_constructible_v<T>
            && is_nothrow_swappable_v<T>
            && is_nothrow_move_constructible_v<E>
            && is_nothrow_swappable_v<E>) -> enable_if_t<Enable>;

        // swap internal
        friend constexpr void swap(expected& x, expected& y) noexcept(noexcept(x.swap(y)))
        {
            x.swap(y);
        }

        // [expected.object.obs], observers
        //! Returns whether the expected contains a value type(true) or error type(false)
        constexpr explicit operator bool() const noexcept;
        constexpr bool has_value() const noexcept;

        template<bool Enable = !is_void_v<T>>
        constexpr auto operator->() const noexcept -> enable_if_t<Enable, const T*>;
        template<bool Enable = !is_void_v<T>>
        constexpr auto operator->() noexcept -> enable_if_t<Enable, T*>;

        // Overloads when T is not a void type
        template<bool Enable = !is_void_v<T>>
        constexpr auto operator*() const & noexcept -> enable_if_t<Enable, add_lvalue_reference_t<const T>>;
        template<bool Enable = !is_void_v<T>>
        constexpr auto operator*() & noexcept -> enable_if_t<Enable, add_lvalue_reference_t<T>>;
        template<bool Enable = !is_void_v<T>>
        constexpr auto operator*() const && noexcept -> enable_if_t < Enable, add_rvalue_reference_t<const T>>;
        template<bool Enable = !is_void_v<T>>
        constexpr auto operator*() && noexcept -> enable_if_t<Enable, add_rvalue_reference_t<T>>;

        template<bool Enable = !is_void_v<T>>
        constexpr auto value() const & -> enable_if_t<Enable, add_lvalue_reference_t<const T>>;
        template<bool Enable = !is_void_v<T>>
        constexpr auto value() & -> enable_if_t<Enable, add_lvalue_reference_t<T>>;
        template<bool Enable = !is_void_v<T>>
        constexpr auto value() const && -> enable_if_t<Enable, add_rvalue_reference_t<const T>>;
        template<bool Enable = !is_void_v<T>>
        constexpr auto value() && -> enable_if_t<Enable, add_rvalue_reference_t<T>>;

        // Overloads when T is a void type
        template<bool Enable = is_void_v<T>, enable_if_t<Enable>* = nullptr>
        constexpr void operator*() const noexcept;

        template<bool Enable = is_void_v<T>, enable_if_t<Enable>* = nullptr>
        constexpr void value() const&;
        template<bool Enable = is_void_v<T>, enable_if_t<Enable>* = nullptr>
        constexpr void value() &&;

        // Error overloads
        constexpr const E& error() const & noexcept;
        constexpr E& error() & noexcept;
        constexpr const E&& error() const && noexcept;
        constexpr E&& error() && noexcept;
        //! Need to use the U template parameter in the enable_if_t check to defer evaluation
        //! until the function is instantiated.
        template<class U>
        constexpr auto value_or(U&&) const& -> enable_if_t<Internal::sfinae_trigger_v<U> && !is_void_v<T>, T>;
        template<class U>
        constexpr auto value_or(U&&) && -> enable_if_t<Internal::sfinae_trigger_v<U> && !is_void_v<T>, T>;

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
        constexpr bool compare_equal_internal(const expected<T2, E2>&) const;
        template<class T2>
        constexpr bool compare_equal_internal(const T2&) const;
        template<class E2>
        constexpr bool compare_equal_internal(const unexpected<E2>&) const;
    };


} // namespace AZStd

#include <AzCore/std/utility/expected.inl>
