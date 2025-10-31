/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AZStd
{
    using std::make_index_sequence;

    // Needed to make std::swap functions availables via ADL
    // when using AZStd::swap is performed in the swap function below
    using std::swap;

    template<class T1, class T2>
    constexpr pair<T1, T2>::pair()
        : first(T1{})
        , second(T2{})
    {
    }
    /// Constructs only the first element, default the second.
    template<class T1, class T2>
    constexpr pair<T1, T2>::pair(const T1& value1)
        : first(value1)
        , second(T2())
    {
    }
    /// Construct from specified values.
    template<class T1, class T2>
    constexpr pair<T1, T2>::pair(const T1& value1, const T2& value2)
        : first(value1)
        , second(value2)
    {
    }
    /// Copy constructor
    template<class T1, class T2>
    constexpr pair<T1, T2>::pair(const pair& rhs) = default;

    /// Move constructor
    template<class T1, class T2>
    constexpr pair<T1, T2>::pair(pair&& rhs) = default;

    template<class T1, class T2>
    template<class U1, class U2, class>
    constexpr pair<T1, T2>::pair(U1&& value1, U2&& value2)
        : first(AZStd::forward<U1>(value1))
        , second(AZStd::forward<U2>(value2))
    {
    }

    // construct from compatible pair
    template<class T1, class T2>
    template<class U1, class U2, class>
    constexpr pair<T1, T2>::pair(const pair<U1, U2>& rhs)
        : first(get<0>(static_cast<decltype(rhs)>(rhs)))
        , second(get<1>(static_cast<decltype(rhs)>(rhs)))
    {
    }
    template<class T1, class T2>
    template<class U1, class U2, class>
    constexpr pair<T1, T2>::pair(pair<U1, U2>&& rhs)
        : first(get<0>(static_cast<decltype(rhs)>(rhs)))
        , second(get<1>(static_cast<decltype(rhs)>(rhs)))
    {
    }

    template<class T1, class T2>
    template<class U1, class U2, class>
    constexpr pair<T1, T2>::pair(pair<U1, U2>& rhs)
        : first(get<0>(static_cast<decltype(rhs)>(rhs)))
        , second(get<1>(static_cast<decltype(rhs)>(rhs)))
    {
    }

    template<class T1, class T2>
    template<class U1, class U2, class>
    constexpr pair<T1, T2>::pair(const pair<U1, U2>&& rhs)
        : first(get<0>(static_cast<decltype(rhs)>(rhs)))
        , second(get<1>(static_cast<decltype(rhs)>(rhs)))
    {
    }

    // Implementation of the AZStd::pair pair-like constructor/assignment functions
    // Pair like constructor defined here after the pair-like concept has been defined
    template<class T1, class T2>
    template<class P, class>
    constexpr pair<T1, T2>::pair(P&& pairLike)
        : first(get<0>(AZStd::forward<P>(pairLike)))
        , second(get<1>(AZStd::forward<P>(pairLike)))
    {
    }


    // pair code to inter operate with tuples
    template<class T1, class T2>
    template<template<class...> class TupleType, class... Args1, class... Args2, size_t... I1, size_t... I2>
    constexpr pair<T1, T2>::pair(piecewise_construct_t, [[maybe_unused]] TupleType<Args1...>& firstArgs, [[maybe_unused]] TupleType<Args2...>& secondArgs,
        AZStd::index_sequence<I1...>, AZStd::index_sequence<I2...>)
        : first(AZStd::forward<Args1>(get<I1>(firstArgs))...)
        , second(AZStd::forward<Args2>(get<I2>(secondArgs))...)
    {
        static_assert(AZStd::is_same_v<TupleType<Args2...>, tuple<Args2...>>, "AZStd::pair tuple constructor can be called with AZStd::tuple instances");
    }

    // Pair constructor overloads which take in a tuple is implemented here as tuple is not included at the place where pair declares the constructor
    template<class T1, class T2>
    template<template<class...> class TupleType, class... Args1, class... Args2>
    constexpr pair<T1, T2>::pair(piecewise_construct_t piecewise_construct,
        TupleType<Args1...> first_args,
        TupleType<Args2...> second_args)
        : pair(piecewise_construct, first_args, second_args, AZStd::make_index_sequence<sizeof...(Args1)>{}, AZStd::make_index_sequence<sizeof...(Args2)>{})
    {
        static_assert(AZStd::is_same_v<TupleType<Args1...>, tuple<Args1...>>, "AZStd::pair tuple constructor can be called with AZStd::tuple instances");
    }

    template<class T1, class T2>
    constexpr auto pair<T1, T2>::operator=(const pair& rhs) -> pair&
    {
        first = rhs.first;
        second = rhs.second;
        return *this;
    }
    template<class T1, class T2>
    constexpr auto pair<T1, T2>::operator=(const pair& rhs) const -> const pair&
    {
        first = rhs.first;
        second = rhs.second;
        return *this;
    }

    template<class T1, class T2>
    constexpr auto pair<T1, T2>::operator=(pair&& rhs) -> pair&
    {
        first = AZStd::move(rhs.first);
        second = AZStd::move(rhs.second);
        return *this;
    }
    template<class T1, class T2>
    constexpr auto pair<T1, T2>::operator=(pair&& rhs) const -> const pair&
    {
        first = AZStd::move(rhs.first);
        second = AZStd::move(rhs.second);
        return *this;
    }

    template<class T1, class T2>
    template<class U1, class U2>
    constexpr auto pair<T1, T2>::operator=(const pair<U1, U2>& rhs)
        -> enable_if_t<is_assignable_v<T1&, const U1&> && is_assignable_v<T2&, const U2&>, pair&>
    {
        first = rhs.first;
        second = rhs.second;
        return *this;
    }

    template<class T1, class T2>
    template<class U1, class U2>
    constexpr auto pair<T1, T2>::operator=(const pair<U1, U2>& rhs) const
        -> enable_if_t<is_assignable_v<const T1&, const U1&> && is_assignable_v<const T2&, const U2&>, const pair&>
    {
        first = rhs.first;
        second = rhs.second;
        return *this;
    }

    template<class T1, class T2>
    template<class U1, class U2>
    constexpr auto pair<T1, T2>::operator=(pair<U1, U2>&& rhs) -> enable_if_t<is_assignable_v<T1&, U1> && is_assignable_v<T2&, U2>, pair&>
    {
        first = AZStd::forward<U1>(rhs.first);
        second = AZStd::forward<U2>(rhs.second);
        return *this;
    }

    template<class T1, class T2>
    template<class U1, class U2>
    constexpr auto pair<T1, T2>::operator=(pair<U1, U2>&& rhs) const
        -> enable_if_t<is_assignable_v<const T1&, U1> && is_assignable_v<const T2&, U2>, const pair&>
    {
        first = AZStd::forward<U1>(rhs.first);
        second = AZStd::forward<U2>(rhs.second);
        return *this;
    }

    template<class T1, class T2>
    template<class P>
    constexpr auto pair<T1, T2>::operator=(P&& pairLike) -> enable_if_t<
        conjunction_v<
            bool_constant<pair_like<P>>,
            bool_constant<!AZStd::same_as<pair, remove_cvref_t<P>>>,
            bool_constant<!Internal::is_subrange<P>>,
            bool_constant<Internal::is_pair_like_assignable_for_t<pair, P>>>,
        pair&>
    {
        first = get<0>(AZStd::forward<P>(pairLike));
        second = get<1>(AZStd::forward<P>(pairLike));
        return *this;
    }

    // This is an operator= overload that can change the values of the pair
    // members if the pair itself is const, but the members are references to a mutable type
    // i.e a `const pair<int&, bool&>` can still modify the int and bool elements, despite
    // the pair itself being const
    template<class T1, class T2>
    template<class P>
    constexpr auto pair<T1, T2>::operator=(P&& pairLike) const -> enable_if_t<
        conjunction_v<
            bool_constant<pair_like<P>>,
            bool_constant<!AZStd::same_as<pair, remove_cvref_t<P>>>,
            bool_constant<!Internal::is_subrange<P>>,
            bool_constant<Internal::is_pair_like_assignable_for_t<const pair, P>>>,
        const pair&>
    {
        first = get<0>(AZStd::forward<P>(pairLike));
        second = get<1>(AZStd::forward<P>(pairLike));
        return *this;
    }

    template<class T1, class T2>
    constexpr auto pair<T1, T2>::swap(pair& rhs)
    {
        // exchange contents with _Right
        if (this != &rhs)
        { // different, worth swapping
            using AZStd::swap;
            swap(first, rhs.first);
            swap(second, rhs.second);
        }
    }

    template<class T1, class T2>
    constexpr auto pair<T1, T2>::swap(const pair& rhs) const
    {
        if (this != &rhs)
        {
            using AZStd::swap;
            swap(first, rhs.first);
            swap(second, rhs.second);
        }
    }

    // utility functions START for AZStd::pair
    template<class T1, class T2>
    constexpr auto swap(AZStd::pair<T1, T2>& left, AZStd::pair<T1, T2>& right) -> enable_if_t<is_swappable_v<T1> && is_swappable_v<T2>>
    { // swap _Left and right pairs
        left.swap(right);
    }

    // Swappable overload for a const pair with reference members which are swappable
    template<class T1, class T2>
    constexpr auto swap(const AZStd::pair<T1, T2>& left, const AZStd::pair<T1, T2>& right)
        -> enable_if_t<is_swappable_v<const T1> && is_swappable_v<const T2>>
    {
        left.swap(right);
    }

    template<class L1, class L2, class R1, class R2, class>
    constexpr bool operator==(const pair<L1, L2>& left, const pair<R1, R2>& right)
    { // test for pair equality
        return left.first == right.first && left.second == right.second;
    }

    template<class L1, class L2, class R1, class R2, class>
    constexpr bool operator!=(const pair<L1, L2>& left, const pair<R1, R2>& right)
    { // test for pair inequality
        return !(left == right);
    }

    template<class L1, class L2, class R1, class R2, class>
    constexpr bool operator<(const pair<L1, L2>& left, const pair<R1, R2>& right)
    { // test if left < right for pairs
        return (left.first < right.first || (!(right.first < left.first) && left.second < right.second));
    }

    template<class L1, class L2, class R1, class R2, class>
    constexpr bool operator>(const pair<L1, L2>& left, const pair<R1, R2>& right)
    { // test if left > right for pairs
        return right < left;
    }

    template<class L1, class L2, class R1, class R2, class>
    constexpr bool operator<=(const pair<L1, L2>& left, const pair<R1, R2>& right)
    { // test if left <= right for pairs
        return !(right < left);
    }

    template<class L1, class L2, class R1, class R2, class>
    constexpr bool operator>=(const pair<L1, L2>& left, const pair<R1, R2>& right)
    { // test if left >= right for pairs
        return !(left < right);
    }

    // make_pair
    template<class T1, class T2>
    constexpr auto make_pair(T1&& value1, T2&& value2)
    {
        using pair_type = pair<AZStd::unwrap_ref_decay_t<T1>, AZStd::unwrap_ref_decay_t<T2>>;
        return pair_type(AZStd::forward<T1>(value1), AZStd::forward<T2>(value2));
    }
} // namespace AZStd


namespace AZStd::Internal
{
    template<size_t>
    struct get_pair;

    template<>
    struct get_pair<0>
    {
        template<class T1, class T2>
        static constexpr T1& get(AZStd::pair<T1, T2>& pairObj)
        {
            return pairObj.first;
        }

        template<class T1, class T2>
        static constexpr const T1& get(const AZStd::pair<T1, T2>& pairObj)
        {
            return pairObj.first;
        }

        template<class T1, class T2>
        static constexpr T1&& get(AZStd::pair<T1, T2>&& pairObj)
        {
            return AZStd::forward<T1>(pairObj.first);
        }

        template<class T1, class T2>
        static constexpr const T1&& get(const AZStd::pair<T1, T2>&& pairObj)
        {
            return AZStd::forward<const T1>(pairObj.first);
        }
    };
    template<>
    struct get_pair<1>
    {
        template<class T1, class T2>
        static constexpr T2& get(AZStd::pair<T1, T2>& pairObj)
        {
            return pairObj.second;
        }

        template<class T1, class T2>
        static constexpr const T2& get(const AZStd::pair<T1, T2>& pairObj)
        {
            return pairObj.second;
        }

        template<class T1, class T2>
        static constexpr T2&& get(AZStd::pair<T1, T2>&& pairObj)
        {
            return AZStd::forward<T2>(pairObj.second);
        }

        template<class T1, class T2>
        static constexpr const T2&& get(const AZStd::pair<T1, T2>&& pairObj)
        {
            return AZStd::forward<const T2>(pairObj.second);
        }
    };
} // namespace AZStd::Internal

namespace AZStd
{
    //! Wraps the std::get function in the AZStd namespace
    //! This methods retrieves the tuple element at a particular index within the pair
    template<size_t I, class T1, class T2>
    constexpr AZStd::tuple_element_t<I, AZStd::pair<T1, T2>>& get(AZStd::pair<T1, T2>& pairObj)
    {
        return Internal::get_pair<I>::get(pairObj);
    }

    //! Wraps the std::get function in the AZStd namespace
    //! This methods retrieves the tuple element at a particular index within the pair
    template<size_t I, class T1, class T2>
    constexpr const AZStd::tuple_element_t<I, AZStd::pair<T1, T2>>& get(const AZStd::pair<T1, T2>& pairObj)
    {
        return Internal::get_pair<I>::get(pairObj);
    }

    //! Wraps the std::get function in the AZStd namespace
    //! This methods retrieves the tuple element at a particular index within the pair
    template<size_t I, class T1, class T2>
    constexpr AZStd::tuple_element_t<I, AZStd::pair<T1, T2>>&& get(AZStd::pair<T1, T2>&& pairObj)
    {
        return Internal::get_pair<I>::get(AZStd::move(pairObj));
    }

    //! Wraps the std::get function in the AZStd namespace
    //! This methods retrieves the tuple element at a particular index within the pair
    template<size_t I, class T1, class T2>
    constexpr const AZStd::tuple_element_t<I, AZStd::pair<T1, T2>>&& get(const AZStd::pair<T1, T2>&& pairObj)
    {
        return Internal::get_pair<I>::get(AZStd::move(pairObj));
    }

    //! Wraps the std::get function in the AZStd namespace
    //! This methods extracts an element from the pair with the specified type T
    //! If there is more than one T in the pair, then this function fails to compile
    template<class T, class U>
    constexpr T& get(AZStd::pair<T, U>& pairObj)
    {
        return Internal::get_pair<0>::get(pairObj);
    }

    //! Wraps the std::get function in the AZStd namespace
    //! This methods extracts an element from the pair with the specified type T
    //! If there is more than one T in the pair, then this function fails to compile
    template<class T, class U>
    constexpr T& get(AZStd::pair<U, T>& pairObj)
    {
        return Internal::get_pair<1>::get(pairObj);
    }

    //! Wraps the std::get function in the AZStd namespace
    //! This methods extracts an element from the pair with the specified type T
    //! If there is more than one T in the pair, then this function fails to compile
    template<class T, class U>
    constexpr const T& get(const AZStd::pair<T, U>& pairObj)
    {
        return Internal::get_pair<0>::get(pairObj);
    }

    //! Wraps the std::get function in the AZStd namespace
    //! This methods extracts an element from the pair with the specified type T
    //! If there is more than one T in the pair, then this function fails to compile
    template<class T, class U>
    constexpr const T& get(const AZStd::pair<U, T>& pairObj)
    {
        return Internal::get_pair<1>::get(pairObj);
    }

    //! Wraps the std::get function in the AZStd namespace
    //! This methods extracts an element from the pair with the specified type T
    //! If there is more than one T in the pair, then this function fails to compile
    template<class T, class U>
    constexpr T&& get(AZStd::pair<T, U>&& pairObj)
    {
        return Internal::get_pair<0>::get(AZStd::move(pairObj));
    }

    //! Wraps the std::get function in the AZStd namespace
    //! This methods extracts an element from the pair with the specified type T
    //! If there is more than one T in the pair, then this function fails to compile
    template<class T, class U>
    constexpr T&& get(AZStd::pair<U, T>&& pairObj)
    {
        return Internal::get_pair<1>::get(AZStd::move(pairObj));
    }

    //! Wraps the std::get function in the AZStd namespace
    //! This methods extracts an element from the pair with the specified type T
    //! If there is more than one T in the pair, then this function fails to compile
    template<class T, class U>
    constexpr const T&& get(const AZStd::pair<T, U>&& pairObj)
    {
        return Internal::get_pair<0>::get(AZStd::move(pairObj));
    }

    //! Wraps the std::get function in the AZStd namespace
    //! This methods extracts an element from the pair with the specified type T
    //! If there is more than one T in the pair, then this function fails to compile
    template<class T, class U>
    constexpr const T&& get(const AZStd::pair<U, T>&& pairObj)
    {
        return Internal::get_pair<1>::get(AZStd::move(pairObj));
    }
} // namespace AZStd
