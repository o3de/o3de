/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/utility/pair_fwd.h>

#include <AzCore/std/typetraits/add_const.h>
#include <AzCore/std/typetraits/is_swappable.h>
#include <AzCore/std/utility/declval.h>
#include <AzCore/std/utility/tuple_concepts.h>

namespace AZStd
{
    // std::tuple_element_t is used for the std::get overloads
    using std::tuple_element_t;

    // std::index_sequence is being brought into the namespace
    // for use with the piecewise constructor
    using std::index_sequence;
}

 // The tuple_size and tuple_element classes need to be specialized in the std:: namespace since the AZStd:: namespace alias them
 // The tuple_size and tuple_element classes is to be specialized here for the AZStd::pair class
namespace std
{
    // Suppressing clang warning error: 'tuple_size' defined as a class template here but previously declared as a struct template [-Werror,-Wmismatched-tags]
    AZ_PUSH_DISABLE_WARNING(, "-Wmismatched-tags");
    template<class T1, class T2>
    struct tuple_size<AZStd::pair<T1, T2>>
        : public AZStd::integral_constant<size_t, 2>
    {};

    template<class T1, class T2>
    struct tuple_element<0, AZStd::pair<T1, T2>>
    {
        using type = T1;
    };

    template<class T1, class T2>
    struct tuple_element<1, AZStd::pair<T1, T2>>
    {
        using type = T2;
    };
    AZ_POP_DISABLE_WARNING;
} // namespace std


namespace AZStd
{
    //! Wraps the std::get function in the AZStd namespace
    //! This methods retrieves the tuple element at a particular index within the pair
    template<size_t I, class T1, class T2>
    constexpr tuple_element_t<I, pair<T1, T2>>& get(pair<T1, T2>& pairObj);

    //! Wraps the std::get function in the AZStd namespace
    //! This methods retrieves the tuple element at a particular index within the pair
    template<size_t I, class T1, class T2>
    constexpr const tuple_element_t<I, pair<T1, T2>>& get(const pair<T1, T2>& pairObj);

    //! Wraps the std::get function in the AZStd namespace
    //! This methods retrieves the tuple element at a particular index within the pair
    template<size_t I, class T1, class T2>
    constexpr tuple_element_t<I, pair<T1, T2>>&& get(pair<T1, T2>&& pairObj);

    //! Wraps the std::get function in the AZStd namespace
    //! This methods retrieves the tuple element at a particular index within the pair
    template<size_t I, class T1, class T2>
    constexpr const tuple_element_t<I, pair<T1, T2>>&& get(const pair<T1, T2>&& pairObj);

    //! Wraps the std::get function in the AZStd namespace
    //! This methods extracts an element from the pair with the specified type T
    //! If there is more than one T in the pair, then this function fails to compile
    template<class T, class U>
    constexpr T& get(pair<T, U>& pairObj);

    //! Wraps the std::get function in the AZStd namespace
    //! This methods extracts an element from the pair with the specified type T
    //! If there is more than one T in the pair, then this function fails to compile
    template<class T, class U>
    constexpr T& get(pair<U, T>& pairObj);

    //! Wraps the std::get function in the AZStd namespace
    //! This methods extracts an element from the pair with the specified type T
    //! If there is more than one T in the pair, then this function fails to compile
    template<class T, class U>
    constexpr const T& get(const pair<T, U>& pairObj);

    //! Wraps the std::get function in the AZStd namespace
    //! This methods extracts an element from the pair with the specified type T
    //! If there is more than one T in the pair, then this function fails to compile
    template<class T, class U>
    constexpr const T& get(const pair<U, T>& pairObj);

    //! Wraps the std::get function in the AZStd namespace
    //! This methods extracts an element from the pair with the specified type T
    //! If there is more than one T in the pair, then this function fails to compile
    template<class T, class U>
    constexpr T&& get(pair<T, U>&& pairObj);

    //! Wraps the std::get function in the AZStd namespace
    //! This methods extracts an element from the pair with the specified type T
    //! If there is more than one T in the pair, then this function fails to compile
    template<class T, class U>
    constexpr T&& get(pair<U, T>&& pairObj);

    //! Wraps the std::get function in the AZStd namespace
    //! This methods extracts an element from the pair with the specified type T
    //! If there is more than one T in the pair, then this function fails to compile
    template<class T, class U>
    constexpr const T&& get(const pair<T, U>&& pairObj);

    //! Wraps the std::get function in the AZStd namespace
    //! This methods extracts an element from the pair with the specified type T
    //! If there is more than one T in the pair, then this function fails to compile
    template<class T, class U>
    constexpr const T&& get(const pair<U, T>&& pairObj);
} // namespace AZStd


namespace AZStd::Internal
{
    template<size_t I, class P, bool TupleElementValid = !is_void_v<tuple_element_t<0, remove_cvref_t<P>>> >
    struct tuple_element_preserve_cvref;

    template<size_t I, class P>
    struct tuple_element_preserve_cvref<I, P, true>
    {
    private:
        static constexpr bool is_lvalue_reference = is_lvalue_reference_v<P>;
        static constexpr bool is_const = is_const_v<remove_reference<P>>;
        using raw_type = tuple_element_t<0, remove_cvref_t<P>>;
        using const_type = conditional_t<is_const, add_const_t<raw_type>, raw_type>;
        using reference_type = conditional_t<is_lvalue_reference, add_lvalue_reference_t<const_type>, add_rvalue_reference_t<const_type>>;
    public:
        using type = reference_type;
    };

    template<class PairType, class P, class = void>
    constexpr bool is_pair_like_constructible_for_t = false;

    template<class T1, class T2, class P>
    constexpr bool is_pair_like_constructible_for_t<pair<T1, T2>, P, enable_if_t<is_constructible_v<T1, typename tuple_element_preserve_cvref<0, P>::type> && is_constructible_v<T2, typename tuple_element_preserve_cvref<1, P>::type>>> = true;

    template<class PairType, class P, class = void>
    constexpr bool is_pair_like_assignable_for_t = false;

    template<class T1, class T2, class P>
    constexpr bool is_pair_like_assignable_for_t<
        pair<T1, T2>,
        P,
        enable_if_t<is_assignable_v<T1&, typename tuple_element_preserve_cvref<0, P>::type> && is_assignable_v<T2&, typename tuple_element_preserve_cvref<1, P>::type> >> = true;

    template<class T1, class T2, class P>
    constexpr bool is_pair_like_assignable_for_t<
        const pair<T1, T2>,
        P,
        enable_if_t<
            is_assignable_v<const T1&, typename tuple_element_preserve_cvref<0, P>::type> && is_assignable_v<const T2&, typename tuple_element_preserve_cvref<1, P>::type>>> =
        true;
}

namespace AZStd
{
    template<class T1, class T2>
    struct pair
    {
        // store a pair of values
        using first_type = T1;
        using second_type = T2;

        /// Construct from defaults
        constexpr pair();
        /// Constructs only the first element, default the second.
        constexpr pair(const T1& value1);
        /// Construct from specified values.
        constexpr pair(const T1& value1, const T2& value2);
        /// Copy constructor
        constexpr pair(const pair& rhs);
        // Move constructor
        constexpr pair(pair&& rhs);

        template<class U1 = T1, class U2 = T2, class = enable_if_t<is_constructible_v<T1, U1> && is_constructible_v<T2, U2>>>
        constexpr pair(U1&& value1, U2&& value2);

        // C++ 23 pair like constructor
        template<
            class P,
            class = enable_if_t<conjunction_v<
                bool_constant<pair_like<P>>
                , bool_constant<!Internal::is_subrange<P>>
                , bool_constant<Internal::is_pair_like_constructible_for_t<pair, P>>
            >>>
#if __cpp_conditional_explicit >= 201806L
        explicit(!is_convertible_v<get<0>(declval<P>()), T1> || !is_convertible_v<get<1>(declval<P>()), T2>)
#endif
        constexpr pair(P&& pairLike);

        // construct from compatible pair
        template<class U1, class U2, class = enable_if_t<is_constructible_v<T1, const U1&> && is_constructible_v<T2, const U2&>>>
#if __cpp_conditional_explicit >= 201806L
        explicit(!is_convertible_v<declval<const U1&>(), T1> || !is_convertible_v<declval<const U2&>(), T2>)
#endif
        constexpr pair(const pair<U1, U2>& rhs);

        // move constructor from rvalue pair
        template<class U1, class U2, class = enable_if_t<is_constructible_v<T1, U1> && is_constructible_v<T2, U2>>>
#if __cpp_conditional_explicit >= 201806L
        explicit(!is_convertible_v<declval<U1>(), T1> || !is_convertible_v<declval<U2>(), T2>)
#endif
        constexpr pair(pair<U1, U2>&& rhs);

        // C++23 non-const lvalue constructor
        template<class U1, class U2, class = enable_if_t<is_constructible_v<T1, U1&> && is_constructible_v<T2, U2&>>>
#if __cpp_conditional_explicit >= 201806L
        explicit(!is_convertible_v<declval<U1&>(), T1> || !is_convertible_v<declval<U2&>(), T2>)
#endif
        constexpr pair(pair<U1, U2>& rhs);
        // C++23 const rvalue constructor
        template<class U1, class U2, class = enable_if_t<is_constructible_v<T1, U1> && is_constructible_v<T2, U2>>>
#if __cpp_conditional_explicit >= 201806L
        explicit(!is_convertible_v<declval<const U1>(), T1> || !is_convertible_v<declval<const U2>(), T2>)
#endif
        constexpr pair(const pair<U1, U2>&& rhs);

        template<template<class...> class TupleType, class... Args1, class... Args2>
        constexpr pair(piecewise_construct_t, TupleType<Args1...> firstArgs, TupleType<Args2...> secondArgs);

        template<template<class...> class TupleType, class... Args1, class... Args2, size_t... I1, size_t... I2>
        constexpr pair(
            piecewise_construct_t,
            TupleType<Args1...>& firstArgs,
            TupleType<Args2...>& secondArgs,
            index_sequence<I1...>,
            index_sequence<I2...>);

        constexpr pair& operator=(const pair& rhs);
        constexpr const pair& operator=(const pair& rhs) const;
        constexpr pair& operator=(pair&& rhs);
        constexpr const pair& operator=(pair&& rhs) const;

        // copy conversion assignment
        template<class U1, class U2>
        constexpr auto operator=(const pair<U1, U2>& rhs)
            -> enable_if_t<is_assignable_v<T1&, const U1&> && is_assignable_v<T2&, const U2&>, pair&>;
        template<class U1, class U2>
        constexpr auto operator=(const pair<U1, U2>& rhs) const
            -> enable_if_t<is_assignable_v<const T1&, const U1&> && is_assignable_v<const T2&, const U2&>, const pair&>;

        // move conversion assignment
        template<class U1, class U2>
        constexpr auto operator=(pair<U1, U2>&& rhs) -> enable_if_t<is_assignable_v<T1&, U1> && is_assignable_v<T2&, U2>, pair&>;
        template<class U1, class U2>
        constexpr auto operator=(pair<U1, U2>&& rhs) const
            -> enable_if_t<is_assignable_v<const T1&, U1> && is_assignable_v<const T2&, U2>, const pair&>;

        // pair-like conversion forward assignment
        template<class P>
        constexpr auto operator=(P&& pairLike) -> enable_if_t<
            conjunction_v<
                bool_constant<pair_like<P>>,
                bool_constant<!AZStd::same_as<pair, remove_cvref_t<P>>>,
                bool_constant<!Internal::is_subrange<P>>,
                bool_constant<Internal::is_pair_like_assignable_for_t<pair, P>>>,
            pair&>;

        // This is an operator= overload that can change the values of the pair
        // members if the pair itself is const, but the members are references to a mutable type
        // i.e a `const pair<int&, bool&>` can still modify the int and bool elements, despite
        // the pair itself being const
        template<class P>
        constexpr auto operator=(P&& pairLike) const -> enable_if_t<
            conjunction_v<
                bool_constant<pair_like<P>>,
                bool_constant<!AZStd::same_as<pair, remove_cvref_t<P>>>,
                bool_constant<!Internal::is_subrange<P>>,
                bool_constant<Internal::is_pair_like_assignable_for_t<const pair, P>>>,
            const pair&>;

        constexpr auto swap(pair& rhs);
        constexpr auto swap(const pair& rhs) const;

        T1 first; // the first stored value
        T2 second; // the second stored value
    };

    // AZStd::pair deduction guides
    template<class T1, class T2>
    pair(T1, T2) -> pair<T1, T2>;

    // pair
    template<class T1, class T2>
    constexpr auto swap(AZStd::pair<T1, T2>& left, AZStd::pair<T1, T2>& right) -> enable_if_t<is_swappable_v<T1> && is_swappable_v<T2>>;

    // Swappable overload for a const pair with reference members which are swappable
    template<class T1, class T2>
    constexpr auto swap(const AZStd::pair<T1, T2>& left, const AZStd::pair<T1, T2>& right)
        -> enable_if_t<is_swappable_v<const T1> && is_swappable_v<const T2>>;

    template<
        class L1,
        class L2,
        class R1,
        class R2,
        class = AZStd::void_t<decltype(declval<L1>() == declval<R1>() && declval<L2>() == declval<R2>())>>
    constexpr bool operator==(const pair<L1, L2>& left, const pair<R1, R2>& right);

    template<
        class L1,
        class L2,
        class R1,
        class R2,
        class = AZStd::void_t<decltype(declval<L1>() == declval<R1>() && declval<L2>() == declval<R2>())>>
    constexpr bool operator!=(const pair<L1, L2>& left, const pair<R1, R2>& right);

    template<
        class L1,
        class L2,
        class R1,
        class R2,
        class =
            AZStd::void_t<decltype(declval<L1>() < declval<R1>() || (!(declval<R1>() < declval<L1>()) && declval<L2>() < declval<R2>()))>>
    constexpr bool operator<(const pair<L1, L2>& left, const pair<R1, R2>& right);

    template<
        class L1,
        class L2,
        class R1,
        class R2,
        class =
            AZStd::void_t<decltype(declval<L1>() < declval<R1>() || (!(declval<R1>() < declval<L1>()) && declval<L2>() < declval<R2>()))>>
    constexpr bool operator>(const pair<L1, L2>& left, const pair<R1, R2>& right);

    template<
        class L1,
        class L2,
        class R1,
        class R2,
        class =
            AZStd::void_t<decltype(declval<L1>() < declval<R1>() || (!(declval<R1>() < declval<L1>()) && declval<L2>() < declval<R2>()))>>
    constexpr bool operator<=(const pair<L1, L2>& left, const pair<R1, R2>& right);

    template<
        class L1,
        class L2,
        class R1,
        class R2,
        class =
            AZStd::void_t<decltype(declval<L1>() < declval<R1>() || (!(declval<R1>() < declval<L1>()) && declval<L2>() < declval<R2>()))>>
    constexpr bool operator>=(const pair<L1, L2>& left, const pair<R1, R2>& right);
} // namespace AZStd

#include <AzCore/std/utility/pair.inl>
