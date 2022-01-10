/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/tuple.h>

namespace AZStd
{
    template <typename T, size_t Index, bool CanBeEmptyBase>
    template <typename U, typename>
    inline constexpr compressed_pair_element<T, Index, CanBeEmptyBase>::compressed_pair_element(U&& value)
        : m_element{ AZStd::forward<U>(value) }
    {
    }

    template <typename T, size_t Index, bool CanBeEmptyBase>
    template <template <typename...> class TupleType, class... Args, size_t... Indices>
    inline constexpr compressed_pair_element<T, Index, CanBeEmptyBase>::compressed_pair_element(AZStd::piecewise_construct_t, TupleType<Args...>&& args, AZStd::index_sequence<Indices...>)
        : m_element{ AZStd::forward<Args>(AZStd::get<Indices>(AZStd::forward<TupleType<Args...>>(args) ))... }
    {
        (void)args;
    }


    template <typename T, size_t Index, bool CanBeEmptyBase>
    inline constexpr T& compressed_pair_element<T, Index, CanBeEmptyBase>::get()
    {
        return m_element;
    }

    template <typename T, size_t Index, bool CanBeEmptyBase>
    inline constexpr const T& compressed_pair_element<T, Index, CanBeEmptyBase>::get() const
    {
        return m_element;
    }

    // compressed_pair_element specialization for a non-final empty base class.
    template <typename T, size_t Index>
    template <typename U, typename>
    inline constexpr compressed_pair_element<T, Index, true>::compressed_pair_element(U&& value)
        : T{ AZStd::forward<U>(value) }
    {
    }

    template <typename T, size_t Index>
    template <template <typename...> class TupleType, class... Args, size_t... Indices>
    inline constexpr compressed_pair_element<T, Index, true>::compressed_pair_element(AZStd::piecewise_construct_t, TupleType<Args...>&& args, AZStd::index_sequence<Indices...>)
        : T{ AZStd::forward<Args>(AZStd::get<Indices>(AZStd::forward<TupleType<Args...>>(args) ))... }
    {
        (void)args;
    }

    template <typename T, size_t Index>
    inline constexpr T& compressed_pair_element<T, Index, true>::get()
    {
        return *this;
    }

    template <typename T, size_t Index>
    inline constexpr const T& compressed_pair_element<T, Index, true>::get() const
    {
        return *this;
    }

    // compressed_pair implementation
    template <typename T1, typename T2>
    template <typename, typename>
    inline constexpr compressed_pair<T1, T2>::compressed_pair()
    {
    }

    template <typename T1, typename T2>
    template <typename T, AZStd::enable_if_t<!is_same<remove_cvref_t<T>, compressed_pair<T1, T2>>::value, bool>>
    inline constexpr compressed_pair<T1, T2>::compressed_pair(T&& firstElement)
        : first_base_type{ AZStd::forward<T>(firstElement) }
        , second_base_type{}
    {
    }

    template <typename T1, typename T2>
    template <typename T>
    inline constexpr compressed_pair<T1, T2>::compressed_pair(skip_element_tag, T&& secondElement)
        : first_base_type{}
        , second_base_type{ AZStd::forward<T>(secondElement) }
    {
    }

    template <typename T1, typename T2>
    template <typename U1, typename U2>
    inline constexpr compressed_pair<T1, T2>::compressed_pair(U1&& firstElement, U2&& secondElement)
        : first_base_type{ AZStd::forward<U1>(firstElement) }
        , second_base_type{ AZStd::forward<U2>(secondElement) }
    {
    }

    template <typename T1, typename T2>
    template <template <typename...> class TupleType, typename... Args1, typename... Args2>
    inline constexpr compressed_pair<T1, T2>::compressed_pair(AZStd::piecewise_construct_t piecewiseTag, TupleType<Args1...>&& firstArgs, TupleType<Args2...>&& secondArgs)
        : first_base_type{ piecewiseTag, AZStd::forward<TupleType<Args1...>>(firstArgs), AZStd::make_index_sequence<sizeof...(Args1)>{} }
        , second_base_type{ piecewiseTag, AZStd::forward<TupleType<Args2...>>(secondArgs), AZStd::make_index_sequence<sizeof...(Args2)>{} }
    {
    }

    template <typename T1, typename T2>
    inline constexpr auto compressed_pair<T1, T2>::first() -> first_base_value_type&
    {
        return static_cast<first_base_type&>(*this).get();
    }

    template <typename T1, typename T2>
    inline constexpr auto compressed_pair<T1, T2>::first() const -> const first_base_value_type&
    {
        return static_cast<const first_base_type&>(*this).get();
    }
    
    template <typename T1, typename T2>
    inline constexpr auto compressed_pair<T1, T2>::second() -> second_base_value_type&
    {
        return static_cast<second_base_type&>(*this).get();
    }

    template <typename T1, typename T2>
    inline constexpr auto compressed_pair<T1, T2>::second() const -> const second_base_value_type&
    {
        return static_cast<const second_base_type&>(*this).get();
    }

    template <typename T1, typename T2>
    void compressed_pair<T1, T2>::swap(compressed_pair& other)
    {
        using AZStd::swap;
        swap(first(), other.first());
        swap(second(), other.second());
    }

    template <class T1, class T2>
    inline void swap(compressed_pair<T1, T2>& lhs, compressed_pair<T1, T2>& rhs)
    {
        lhs.swap(rhs);
    }
} // namespace AZStd
