/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/utility/tuple_fwd.h>

#include <AzCore/std/containers/array.h>
#include <AzCore/std/function/invoke.h>
#include <AzCore/std/hash.h>
#include <AzCore/std/typetraits/conjunction.h>
#include <AzCore/std/typetraits/is_same.h>
#include <AzCore/std/typetraits/void_t.h>
#include <AzCore/std/utility/tuple_concepts.h>
#include <AzCore/std/utils.h>

namespace AZStd
{
    //! Creates an hash specialization for tuple types using the hash_combine function
    //! The std::tuple implementation does not have this support. This is an extension
    template <typename... Types>
    struct hash<AZStd::tuple<Types...>>
    {
        template<size_t... Indices>
        constexpr size_t ElementHasher(const AZStd::tuple<Types...>& value, AZStd::index_sequence<Indices...>) const
        {
            size_t seed = 0;
            int dummy[]{ 0, (hash_combine(seed, AZStd::get<Indices>(value)), 0)... };
            (void)dummy;
            return seed;
        }

        template<typename... TupleTypes, typename = AZStd::enable_if_t<hash_enabled_concept_v<TupleTypes...>>>
        constexpr size_t operator()(const AZStd::tuple<TupleTypes...>& value) const
        {
            return ElementHasher(value, AZStd::make_index_sequence<sizeof...(Types)>{});
        }
    };
} // namespace AZStd

namespace AZStd::Internal
{
    template<class... Types, size_t... Indices>
    auto swap_tuple_elements(const tuple<Types...>& left, const tuple<Types...>& right, AZStd::index_sequence<Indices...>)
    {
        using AZStd::swap;
        (swap(AZStd::get<Indices>(left), AZStd::get<Indices>(right)), ...);
    }
} // namespace AZStd::Internal

namespace AZStd
{
    // C++23 overload which allows swapping an rvalue tuple or a const lvalue tuple
    // that stores only reference and pointer types.
    // For example a temporary `tuple<int&, int&>` can bind to this function
    // as the purpose of allowing swap is to allow the references to the integers to be swapped,
    // not the entire tuple itself
    template<class... Types>
    auto swap(const tuple<Types...>& left, const tuple<Types...>& right) -> enable_if_t<(is_swappable_v<const Types> && ...)>
    {
        Internal::swap_tuple_elements(left, right, AZStd::index_sequence_for<Types...>{});
    }
} // namespace AZStd

// AZStd::apply implementation helper block
namespace AZStd::Internal
{
    template<class Fn, class Tuple, size_t... Is>
    constexpr auto apply_impl(Fn&& f, Tuple&& tupleObj, AZStd::index_sequence<Is...>)
        -> decltype(AZStd::invoke(AZStd::declval<Fn>(), AZStd::get<Is>(AZStd::declval<Tuple>())...))
    {
        (void)tupleObj;
        return AZStd::invoke(AZStd::forward<Fn>(f), AZStd::get<Is>(AZStd::forward<Tuple>(tupleObj))...);
    }
} // namespace AZStd::Internal

namespace AZStd
{
    template<class Fn, class Tuple>
    constexpr auto apply(Fn&& f, Tuple&& tupleObj) -> decltype(Internal::apply_impl(
        AZStd::declval<Fn>(), AZStd::declval<Tuple>(), AZStd::make_index_sequence<AZStd::tuple_size<AZStd::decay_t<Tuple>>::value>{}))
    {
        return Internal::apply_impl(
            AZStd::forward<Fn>(f),
            AZStd::forward<Tuple>(tupleObj),
            AZStd::make_index_sequence<AZStd::tuple_size<AZStd::decay_t<Tuple>>::value>{});
    }
} // namespace AZStd
