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

namespace AZStd
{
    //! AZStd::pair to std::tuple function for replicating the functionality of std::tuple assignment operator from std::pair
    template<class T1, class T2>
    constexpr tuple<T1, T2> tuple_assign(const AZStd::pair<T1, T2>& azPair);

    //! AZStd::pair to std::tuple function for replicating the functionality of std::tuple assignment operator from std::pair
    template<class T1, class T2>
    constexpr tuple<T1, T2> tuple_assign(AZStd::pair<T1, T2>&& azPair);
} // namespace AZStd

// The tuple_size and tuple_element classes need to be specialized in the std:: namespace since the AZStd:: namespace alias them
// The tuple_size and tuple_element classes is to be specialized here for the AZStd::pair class
// The tuple_size and tuple_element classes is to be specialized here for the AZStd::array class

//std::tuple_size<std::pair> as defined by C++ 11 until C++ 14
//template< class T1, class T2 >
//struct tuple_size<std::pair<T1, T2>>;
//std::tuple_size<std::pair> as defined since C++ 14
//template <class T1, class T2>
//struct tuple_size<std::pair<T1, T2>> : std::integral_constant<std::size_t, 2> { };

//std::tuple_element<std::pair> as defined since C++ 11
//template< class T1, class T2 >
//struct tuple_element<0, std::pair<T1,T2> >;
//template< class T1, class T2 >
//struct tuple_element<1, std::pair<T1,T2> >;

namespace std
{
    // Suppressing clang warning error: 'tuple_size' defined as a class template here but previously declared as a struct template [-Werror,-Wmismatched-tags]
    AZ_PUSH_DISABLE_WARNING(, "-Wmismatched-tags")
    template<class T1, class T2>
    struct tuple_size<AZStd::pair<T1, T2>> : public AZStd::integral_constant<size_t, 2> {};

    template<class T1, class T2>
    struct tuple_element<0, AZStd::pair<T1, T2>>
    {
    public:
        using type = T1;
    };

    template<class T1, class T2>
    struct tuple_element<1, AZStd::pair<T1, T2>>
    {
    public:
        using type = T2;
    };

    template<class T, size_t N>
    struct tuple_size<AZStd::array<T, N>> : public AZStd::integral_constant<size_t, N> {};

    template<size_t I, class T, size_t N>
    struct tuple_element<I, AZStd::array<T, N>>
    {
        static_assert(I < N, "AZStd::tuple_element has been called on array with an index that is out of bounds");
        using type = T;
    };
    AZ_POP_DISABLE_WARNING
} // namespace std
