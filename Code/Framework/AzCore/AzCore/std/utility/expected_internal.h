/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/utils.h>
#include <AzCore/std/typetraits/is_assignable.h>
#include <AzCore/std/typetraits/is_constructible.h>
#include <AzCore/std/typetraits/is_destructible.h>
#include <AzCore/std/typetraits/is_void.h>

namespace AZStd
{
    template <class T, class E>
    class expected;

    template<class E>
    class unexpected;

    //! Tag Type for in-place construction of the error type
    struct unexpect_t { explicit unexpect_t() = default; };
    inline constexpr unexpect_t unexpect{};
}

namespace AZStd::Internal
{
    template<class T, class E, class U, class G>
    constexpr bool not_convertible_or_constructible_from_other_std_expected_v
        = !is_constructible_v<T, expected<U, G>&>
        || !is_constructible_v<T, expected<U, G>>
        || !is_constructible_v<T, const expected<U, G>&>
        || !is_constructible_v<T, const expected<U, G>>
        || !is_convertible_v<expected<U, G>&, T>
        || !is_convertible_v<expected<U, G>&&, T>
        || !is_convertible_v<const expected<U, G>&, T>
        || !is_convertible_v<const expected<U, G>&&, T>
        || !is_constructible_v<unexpected<E>, expected<U, G>&>
        || !is_constructible_v<unexpected<E>, expected<U, G>>
        || !is_constructible_v<unexpected<E>, const expected<U, G>&>
        || !is_constructible_v<unexpected<E>, const expected<U, G>>;

    template<class T>
    constexpr bool is_std_unexpected_specialization_v = false;

    template<class E>
    constexpr bool is_std_unexpected_specialization_v<unexpected<E>> = true;

    // Due to not having C++20 support, the requires block cannot be used
    // to specify constraints on a class special member functions(default constructor, copy/move constructor, destructor)
    // without using function templates on the AZStd::expected class template.
    // The problem with using a function template though, is that it isn't default'able.
    // Therefore it means that the class cannot have trivial versions of these special member functions
    // To workaround this, inheritance is used instead to add constraints on the AZStd::expected class template
    // special member functions without making them template.
    // This allows them to be `= default` which has benefits such as allowing the special member functions to be used from a union type
    // when they are available

    //! Enumerations of availabilty for a special member function
    //! If it is trivial, it is default'ed,
    //! if it is not trivial, then it has a implmentation
    //! Finally if it is unavailable it is explicitly deleted
    enum class special_member_availability
    {
        Trivial,
        NonTrivial,
        Unavailable
    };

    // A destructor is implemented for the union
    //! expected union constructor and destructor constraints
    template <class T, class E, special_member_availability Destructor =
        (is_void_v<T> || is_trivially_destructible_v<T>) && is_trivially_destructible_v<E>
        ? special_member_availability::Trivial
        : special_member_availability::NonTrivial,
        special_member_availability Constructor =
        (is_void_v<T> || is_trivially_constructible_v<T>) && is_trivially_constructible_v<E>
        ? special_member_availability::Trivial
        : special_member_availability::NonTrivial>
    union expected_union;

    // For a union with types that have trivial destructors and trivial default constructors,
    // the destructor is default'ed
    template <class T, class E>
    union expected_union<T, E, special_member_availability::Trivial, special_member_availability::Trivial>
    {
        constexpr expected_union() = default;
        ~expected_union() = default;

        //! storage for the error type
        T m_value;
        //! storage for the error type
        E m_unexpected;
    };

    // For a union with types that have non-trivial destructors and trivial default constructors
    template <class T, class E>
    union expected_union<T, E, special_member_availability::NonTrivial, special_member_availability::Trivial>
    {
        constexpr expected_union() = default;
        ~expected_union() {}

        //! storage for the value type
        T m_value;
        //! storage for the error type
        E m_unexpected;
    };

    //! expected union specialization for void value type with an error type
    //! whose destructor is trivial and default constructor is trivial
    template <class E>
    union expected_union<void, E, special_member_availability::Trivial, special_member_availability::Trivial>
    {
        constexpr expected_union() = default;
        ~expected_union() = default;

        //! storage for the error type
        E m_unexpected;
    };

    //! expected union specialization for void value type with an error type
    //! whose destructor is non-trivial and default constructor is trivial
    template <class E>
    union expected_union<void, E, special_member_availability::NonTrivial, special_member_availability::Trivial>
    {
        constexpr expected_union() = default;
        ~expected_union() {}

        //! storage for the error type
        E m_unexpected;
    };

    // For a union with types that have trivial destructors and non-trivial default constructors,
    template <class T, class E>
    union expected_union<T, E, special_member_availability::Trivial, special_member_availability::NonTrivial>
    {
        expected_union() {}
        ~expected_union() = default;

        //! storage for the error type
        T m_value;
        //! storage for the error type
        E m_unexpected;
    };

    // For a union with types that have both non-trivial destructors and non-trivial default constructors,
    template <class T, class E>
    union expected_union<T, E, special_member_availability::NonTrivial, special_member_availability::NonTrivial>
    {
        expected_union() {};
        ~expected_union() {}

        //! storage for the value type
        T m_value;
        //! storage for the error type
        E m_unexpected;
    };

    //! expected union specialization for void value type with an error type
    //! whose destructor is trivial and default constructor is non-trivial
    template <class E>
    union expected_union<void, E, special_member_availability::Trivial, special_member_availability::NonTrivial>
    {
        expected_union() {}
        ~expected_union() = default;

        //! storage for the error type
        E m_unexpected;
    };

    //! expected union specialization for void value type with an error type
    //! whose destructor is non-trivial and default constructor is non-trivial
    template <class E>
    union expected_union<void, E, special_member_availability::NonTrivial, special_member_availability::NonTrivial>
    {
        expected_union() {}
        ~expected_union() {}

        //! storage for the error type
        E m_unexpected;
    };

    template <class T, class E>
    struct expected_storage
    {
        constexpr expected_storage() = default;

        // in-place construction for the value type
        template<class... Args>
        constexpr expected_storage(in_place_t, Args&&... args);
        // in-place construction for the error type
        template<class... Args>
        constexpr expected_storage(unexpect_t, Args&&... args);

        //! stores the value type and error type for the expected type in a union
        union expected_union<T, E> m_storage;

        //! Default constructor stores a value initialized value type instance
        bool m_hasValue{ true };
    };

    // The destructor is always available
    // It is either trivial or non-trivial
    template <class T, class E, special_member_availability =
        (is_void_v<T> || is_trivially_destructible_v<T>) && is_trivially_destructible_v<E>
        ? special_member_availability::Trivial
        : special_member_availability::NonTrivial>
    struct expected_storage_destructor;

    // The copy constructor can be trivial or non-trivial
    // it will  available if either T or E is not copy constructible
    template <class T, class E, special_member_availability =
        (is_void_v<T> || is_trivially_copy_constructible_v<T>) && is_trivially_copy_constructible_v<E>
            ? special_member_availability::Trivial
            : (is_void_v<T> || is_copy_constructible_v<T>) && is_copy_constructible_v<E>
                ? special_member_availability::NonTrivial
                : special_member_availability::Unavailable>
    struct expected_storage_copy_constructor;

    // The move constructor can be trivial or non-trivial
    // it will available if either T or E is not move constructible
    template <class T, class E, special_member_availability =
        (is_void_v<T> || is_trivially_move_constructible_v<T>) && is_trivially_move_constructible_v<E>
        ? special_member_availability::Trivial
        : (is_void_v<T> || is_move_constructible_v<T>) && is_move_constructible_v<E>
            ? special_member_availability::NonTrivial
            : special_member_availability::Unavailable>
    struct expected_storage_move_constructor;

    // The copy assignment can be trivial or non-trivial
    // it will available if either T or E is not copy assignable
    template <class T, class E, special_member_availability =
        (is_void_v<T> || (is_trivially_copy_constructible_v<T> && is_trivially_copy_assignable_v<T>))
        && (is_trivially_copy_constructible_v<E> && is_trivially_copy_assignable_v<E>)
        ? special_member_availability::Trivial
        : (is_void_v<T> || (is_copy_constructible_v<T> && is_copy_assignable_v<T>))
        && (is_copy_constructible_v<E> && is_copy_assignable_v<E>)
        ? special_member_availability::NonTrivial
        : special_member_availability::Unavailable>
    struct expected_storage_copy_assignment;

    // The move assignment can be trivial or non-trivial
    // it will available if either T or E is not move assignable
    template <class T, class E, special_member_availability =
        (is_void_v<T> || (is_trivially_move_constructible_v<T> && is_trivially_move_assignable_v<T>))
            && (is_trivially_move_constructible_v<E> && is_trivially_move_assignable_v<E>)
        ? special_member_availability::Trivial
        : (is_void_v<T> || (is_move_constructible_v<T> && is_move_assignable_v<T>))
            && (is_move_constructible_v<E> && is_move_assignable_v<E>)
        ? special_member_availability::NonTrivial
        : special_member_availability::Unavailable>
    struct expected_storage_move_assignment;

    //! expected destructor constraints
    // Trivial destructor case
    template <class T, class E>
    struct expected_storage_destructor<T, E, special_member_availability::Trivial>
        : expected_storage<T, E>
    {
        // Bring in the base class constructors
        using expected_storage<T, E>::expected_storage;

        // destructor is trivial, it is default'ed
        ~expected_storage_destructor() = default;

        // Make the other special member defaulted to use the expected_storage versions
        constexpr expected_storage_destructor() = default;
        constexpr expected_storage_destructor(const expected_storage_destructor&) = default;
        constexpr expected_storage_destructor(expected_storage_destructor&&) = default;
        constexpr expected_storage_destructor& operator=(const expected_storage_destructor&) = default;
        constexpr expected_storage_destructor& operator=(expected_storage_destructor&&) = default;
    };
    // Non-trivial destructor case
    template <class T, class E>
    struct expected_storage_destructor<T, E, special_member_availability::NonTrivial>
        : expected_storage<T, E>
    {
        // Bring in the base class constructors
        using expected_storage<T, E>::expected_storage;

        // destructor is non-trivial, it is defined in the inline file
        ~expected_storage_destructor();

        // Make the other special member defaulted to use the expected_storage versions
        constexpr expected_storage_destructor() = default;
        constexpr expected_storage_destructor(const expected_storage_destructor&) = default;
        constexpr expected_storage_destructor(expected_storage_destructor&&) = default;
        constexpr expected_storage_destructor& operator=(const expected_storage_destructor&) = default;
        constexpr expected_storage_destructor& operator=(expected_storage_destructor&&) = default;
    };

    //! expected copy constructor constraints
    //! it inherits the special member functions from the expected destructor case
    // Trivial copy constructor case
    template <class T, class E>
    struct expected_storage_copy_constructor<T, E, special_member_availability::Trivial>
        : expected_storage_destructor<T, E>
    {
        // Bring in the base class constructors
        using expected_storage_destructor<T, E>::expected_storage_destructor;

        // copy constructor is trivial, it is default'ed
        constexpr expected_storage_copy_constructor(const expected_storage_copy_constructor&) = default;

        // Make the other special member defaulted to use the expected_storage versions
        constexpr expected_storage_copy_constructor() = default;
        ~expected_storage_copy_constructor() = default;
        constexpr expected_storage_copy_constructor(expected_storage_copy_constructor&&) = default;
        constexpr expected_storage_copy_constructor& operator=(const expected_storage_copy_constructor&) = default;
        constexpr expected_storage_copy_constructor& operator=(expected_storage_copy_constructor&&) = default;
    };
    // Non-trivial copy constructor case
    template <class T, class E>
    struct expected_storage_copy_constructor<T, E, special_member_availability::NonTrivial>
        : expected_storage_destructor<T, E>
    {
        // Bring in the base class constructors
        using expected_storage_destructor<T, E>::expected_storage_destructor;

        // copy constructor is non-trivial, it is defined in the inline file
        constexpr expected_storage_copy_constructor(const expected_storage_copy_constructor&);

        // Make the other special member defaulted to use the expected_storage versions
        constexpr expected_storage_copy_constructor() = default;
        ~expected_storage_copy_constructor() = default;
        constexpr expected_storage_copy_constructor(expected_storage_copy_constructor&&) = default;
        constexpr expected_storage_copy_constructor& operator=(const expected_storage_copy_constructor&) = default;
        constexpr expected_storage_copy_constructor& operator=(expected_storage_copy_constructor&&) = default;
    };
    // Unavailable copy constructor case
    template <class T, class E>
    struct expected_storage_copy_constructor<T, E, special_member_availability::Unavailable>
        : expected_storage_destructor<T, E>
    {
        // Bring in the base class constructors
        using expected_storage_destructor<T, E>::expected_storage_destructor;

        // the copy constructor is deleted in this case
        constexpr expected_storage_copy_constructor(const expected_storage_copy_constructor&) = delete;

        // Make the other special member defaulted to use the expected_storage versions
        constexpr expected_storage_copy_constructor() = default;
        ~expected_storage_copy_constructor() = default;
        constexpr expected_storage_copy_constructor(expected_storage_copy_constructor&&) = default;
        constexpr expected_storage_copy_constructor& operator=(const expected_storage_copy_constructor&) = default;
        constexpr expected_storage_copy_constructor& operator=(expected_storage_copy_constructor&&) = default;
    };

    //! expected move constructor constraints
    //! it inherits the special member functions from the expected copy constructor case
    // Trivial move constructor case
    template <class T, class E>
    struct expected_storage_move_constructor<T, E, special_member_availability::Trivial>
        : expected_storage_copy_constructor<T, E>
    {
        // Bring in the base class constructors
        using expected_storage_copy_constructor<T, E>::expected_storage_copy_constructor;

        // move constructor is trivial, it is default'ed
        constexpr expected_storage_move_constructor(expected_storage_move_constructor&&) = default;

        // Make the other special member defaulted to use the expected_storage versions
        constexpr expected_storage_move_constructor() = default;
        ~expected_storage_move_constructor() = default;
        constexpr expected_storage_move_constructor(const expected_storage_move_constructor&) = default;
        constexpr expected_storage_move_constructor& operator=(const expected_storage_move_constructor&) = default;
        constexpr expected_storage_move_constructor& operator=(expected_storage_move_constructor&&) = default;
    };
    // Non-trivial move constructor case
    template <class T, class E>
    struct expected_storage_move_constructor<T, E, special_member_availability::NonTrivial>
        : expected_storage_copy_constructor<T, E>
    {
        // Bring in the base class constructors
        using expected_storage_copy_constructor<T, E>::expected_storage_copy_constructor;

        // move constructor is non-trivial, it is defined in the inline file
        constexpr expected_storage_move_constructor(expected_storage_move_constructor&&);

        // Make the other special member defaulted to use the expected_storage versions
        constexpr expected_storage_move_constructor() = default;
        ~expected_storage_move_constructor() = default;
        constexpr expected_storage_move_constructor(const expected_storage_move_constructor&) = default;
        constexpr expected_storage_move_constructor& operator=(const expected_storage_move_constructor&) = default;
        constexpr expected_storage_move_constructor& operator=(expected_storage_move_constructor&&) = default;
    };
    // Unavailable move constructor case
    template <class T, class E>
    struct expected_storage_move_constructor<T, E, special_member_availability::Unavailable>
        : expected_storage_copy_constructor<T, E>
    {
        // Bring in the base class constructors
        using expected_storage_copy_constructor<T, E>::expected_storage_copy_constructor;

        // the move constructor is deleted in this case
        constexpr expected_storage_move_constructor(expected_storage_move_constructor&&) = delete;

        // Make the other special member defaulted to use the expected_storage versions
        constexpr expected_storage_move_constructor() = default;
        ~expected_storage_move_constructor() = default;
        constexpr expected_storage_move_constructor(const expected_storage_move_constructor&) = default;
        constexpr expected_storage_move_constructor& operator=(const expected_storage_move_constructor&) = default;
        constexpr expected_storage_move_constructor& operator=(expected_storage_move_constructor&&) = default;
    };

    //! expected copy asiignment constraints
    //! it inherits the special member functions from the expected move constructor case
    // Trivial copy assignment case
    template <class T, class E>
    struct expected_storage_copy_assignment<T, E, special_member_availability::Trivial>
        : expected_storage_move_constructor<T, E>
    {
        // Bring in the base class constructors
        using expected_storage_move_constructor<T, E>::expected_storage_move_constructor;

        // copy assignment is trivial, it is default'ed
        constexpr expected_storage_copy_assignment& operator=(const expected_storage_copy_assignment&) = default;

        // Make the other special member defaulted to use the expected_storage versions
        constexpr expected_storage_copy_assignment() = default;
        ~expected_storage_copy_assignment() = default;
        constexpr expected_storage_copy_assignment(const expected_storage_copy_assignment&) = default;
        constexpr expected_storage_copy_assignment(expected_storage_copy_assignment&&) = default;
        constexpr expected_storage_copy_assignment& operator=(expected_storage_copy_assignment&&) = default;
    };
    // Non-trivial copy assignment case
    template <class T, class E>
    struct expected_storage_copy_assignment<T, E, special_member_availability::NonTrivial>
        : expected_storage_move_constructor<T, E>
    {
        // Bring in the base class constructors
        using expected_storage_move_constructor<T, E>::expected_storage_move_constructor;

        // copy assignment is non-trivial, it is defined in the inline file
        constexpr expected_storage_copy_assignment& operator=(const expected_storage_copy_assignment&);

        // Make the other special member defaulted to use the expected_storage versions
        constexpr expected_storage_copy_assignment() = default;
        ~expected_storage_copy_assignment() = default;
        constexpr expected_storage_copy_assignment(const expected_storage_copy_assignment&) = default;
        constexpr expected_storage_copy_assignment(expected_storage_copy_assignment&&) = default;
        constexpr expected_storage_copy_assignment& operator=(expected_storage_copy_assignment&&) = default;
    };
    // Unavailable copy assignment case
    template <class T, class E>
    struct expected_storage_copy_assignment<T, E, special_member_availability::Unavailable>
        : expected_storage_move_constructor<T, E>
    {
        // Bring in the base class constructors
        using expected_storage_move_constructor<T, E>::expected_storage_move_constructor;

        // the copy assignment is deleted in this case
        constexpr expected_storage_copy_assignment& operator=(const expected_storage_copy_assignment&) = delete;

        // Make the other special member defaulted to use the expected_storage versions
        constexpr expected_storage_copy_assignment() = default;
        ~expected_storage_copy_assignment() = default;
        constexpr expected_storage_copy_assignment(const expected_storage_copy_assignment&) = default;
        constexpr expected_storage_copy_assignment(expected_storage_copy_assignment&&) = default;
        constexpr expected_storage_copy_assignment& operator=(expected_storage_copy_assignment&&) = default;
    };

    //! expected move assignment constraints
    //! it inherits the special member functions from the expected copy assignment case
    // Trivial move assignment case
    template <class T, class E>
    struct expected_storage_move_assignment<T, E, special_member_availability::Trivial>
        : expected_storage_copy_assignment<T, E>
    {
        // Bring in the base class constructors
        using expected_storage_copy_assignment<T, E>::expected_storage_copy_assignment;

        // move assignment is trivial, it is default'ed
        constexpr expected_storage_move_assignment& operator=(expected_storage_move_assignment&&) = default;

        // Make the other special member defaulted to use the expected_storage versions
        constexpr expected_storage_move_assignment() = default;
        ~expected_storage_move_assignment() = default;
        constexpr expected_storage_move_assignment(const expected_storage_move_assignment&) = default;
        constexpr expected_storage_move_assignment(expected_storage_move_assignment&&) = default;
        constexpr expected_storage_move_assignment& operator=(const expected_storage_move_assignment&) = default;
    };
    // Non-trivial move assignment case
    template <class T, class E>
    struct expected_storage_move_assignment<T, E, special_member_availability::NonTrivial>
        : expected_storage_copy_assignment<T, E>
    {
        // Bring in the base class constructors
        using expected_storage_copy_assignment<T, E>::expected_storage_copy_assignment;

        // move assignment is non-trivial, it is defined in the inline file
        constexpr expected_storage_move_assignment& operator=(expected_storage_move_assignment&&);

        // Make the other special member defaulted to use the expected_storage versions
        constexpr expected_storage_move_assignment() = default;
        ~expected_storage_move_assignment() = default;
        constexpr expected_storage_move_assignment(const expected_storage_move_assignment&) = default;
        constexpr expected_storage_move_assignment(expected_storage_move_assignment&&) = default;
        constexpr expected_storage_move_assignment& operator=(const expected_storage_move_assignment&) = default;
    };
    // Unavailable move assignment case
    template <class T, class E>
    struct expected_storage_move_assignment<T, E, special_member_availability::Unavailable>
        : expected_storage_copy_assignment<T, E>
    {
        // Bring in the base class constructors
        using expected_storage_copy_assignment<T, E>::expected_storage_copy_assignment;

        // the move assignment is deleted in this case
        constexpr expected_storage_move_assignment& operator=(expected_storage_move_assignment&&) = delete;

        // Make the other special member defaulted to use the expected_storage versions
        constexpr expected_storage_move_assignment() = default;
        ~expected_storage_move_assignment() = default;
        constexpr expected_storage_move_assignment(const expected_storage_move_assignment&) = default;
        constexpr expected_storage_move_assignment(expected_storage_move_assignment&&) = default;
        constexpr expected_storage_move_assignment& operator=(const expected_storage_move_assignment&) = default;
    };
}

#include <AzCore/std/utility/expected_internal.inl>
