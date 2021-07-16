/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/function/invoke.h>
#include <AzCore/std/utils.h>
#include <AzCore/std/typetraits/is_same.h>
#include <AzCore/std/typetraits/void_t.h>
#include <tuple>
#include <AzCore/std/typetraits/conjunction.h>

namespace AZStd
{
    template<class... Types>
    using tuple = std::tuple<Types...>;

    template<class T>
    using tuple_size = std::tuple_size<T>;

    template<size_t I, class T>
    using tuple_element = std::tuple_element<I, T>;

    template<size_t I, class T>
    using tuple_element_t = typename std::tuple_element<I, T>::type;

    // Placeholder structure that can be assigned any value with no effect.
    // This is used by AZStd::tie as placeholder for unused arugments
    using ignore_t = AZStd::decay_t<decltype(std::ignore)>;
    decltype(std::ignore) ignore = std::ignore;

    using std::make_tuple;
    using std::tie;
    using std::forward_as_tuple;
    using std::tuple_cat;
    using std::get;

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
}

namespace AZStd
{
    // pair code to inter operate with tuples
    template<class T1, class T2>
    template<template<class...> class TupleType, class... Args1, class... Args2, size_t... I1, size_t... I2>
    constexpr pair<T1, T2>::pair(piecewise_construct_t, TupleType<Args1...>& first_args, TupleType<Args2...>& second_args,
        AZStd::index_sequence<I1...>, AZStd::index_sequence<I2...>)
        : first(AZStd::forward<Args1>(AZStd::get<I1>(first_args))...)
        , second(AZStd::forward<Args2>(AZStd::get<I2>(second_args))...)
    {
        (void)first_args;
        (void)second_args;
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

    namespace Internal
    {
        template<size_t> struct get_pair;

        template<>
        struct get_pair<0>
        {
            template<class T1, class T2>
            static constexpr T1& get(AZStd::pair<T1, T2>& pairObj) { return pairObj.first; }

            template<class T1, class T2>
            static constexpr const T1& get(const AZStd::pair<T1, T2>& pairObj) { return pairObj.first; }

            template<class T1, class T2>
            static constexpr T1&& get(AZStd::pair<T1, T2>&& pairObj) { return AZStd::forward<T1>(pairObj.first); }

            template<class T1, class T2>
            static constexpr const T1&& get(const AZStd::pair<T1, T2>&& pairObj) { return AZStd::forward<const T1>(pairObj.first); }
        };
        template<>
        struct get_pair<1>
        {
            template<class T1, class T2>
            static constexpr T2& get(AZStd::pair<T1, T2>& pairObj) { return pairObj.second; }

            template<class T1, class T2>
            static constexpr const T2& get(const AZStd::pair<T1, T2>& pairObj) { return pairObj.second; }

            template<class T1, class T2>
            static constexpr T2&& get(AZStd::pair<T1, T2>&& pairObj) { return AZStd::forward<T2>(pairObj.second); }

            template<class T1, class T2>
            static constexpr const T2&& get(const AZStd::pair<T1, T2>&& pairObj) { return AZStd::forward<const T2>(pairObj.second); }
        };
    }

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

    //! AZStd::pair to std::tuple function for replicating the functionality of std::tuple assignment operator from std::pair
    template<class T1, class T2>
    constexpr tuple<T1, T2> tuple_assign(const AZStd::pair<T1, T2>& azPair)
    {
        return std::make_tuple(azPair.first, azPair.second);
    }

    //! AZStd::pair to std::tuple function for replicating the functionality of std::tuple assignment operator from std::pair
    template<class T1, class T2>
    constexpr tuple<T1, T2> tuple_assign(AZStd::pair<T1, T2>&& azPair)
    {
        return std::make_tuple(AZStd::move(azPair.first), AZStd::move(azPair.second));
    }
}

namespace AZStd
{
    // implementation of the std::get function within the AZStd::namespace which allows AZStd::apply to be used
    // with AZStd::array
    template<size_t I, class T, size_t N>
    constexpr T& get(AZStd::array<T, N>& arr)
    {
        static_assert(I < N, "AZStd::get has been called on array with an index that is out of bounds");
        return arr[I];
    };

    template<size_t I, class T, size_t N>
    constexpr const T& get(const AZStd::array<T, N>& arr)
    {
        static_assert(I < N, "AZStd::get has been called on array with an index that is out of bounds");
        return arr[I];
    };

    template<size_t I, class T, size_t N>
    constexpr T&& get(AZStd::array<T, N>&& arr)
    {
        static_assert(I < N, "AZStd::get has been called on array with an index that is out of bounds");
        return AZStd::move(arr[I]);
    };

    template<size_t I, class T, size_t N>
    constexpr const T&& get(const AZStd::array<T, N>&& arr)
    {
        static_assert(I < N, "AZStd::get has been called on array with an index that is out of bounds");
        return AZStd::move(arr[I]);
    };
}

// AZStd::apply implemenation helper block 
namespace AZStd
{
    namespace Internal
    {
        template<class Fn, class Tuple, size_t... Is>
        constexpr auto apply_impl(Fn&& f, Tuple&& tupleObj, AZStd::index_sequence<Is...>) -> decltype(AZStd::invoke(AZStd::declval<Fn>(), AZStd::get<Is>(AZStd::declval<Tuple>())...))
        {
            (void)tupleObj;
            return AZStd::invoke(AZStd::forward<Fn>(f), AZStd::get<Is>(AZStd::forward<Tuple>(tupleObj))...);
        }
    }

    template<class Fn, class Tuple>
    constexpr auto apply(Fn&& f, Tuple&& tupleObj)
        -> decltype(Internal::apply_impl(AZStd::declval<Fn>(), AZStd::declval<Tuple>(), AZStd::make_index_sequence<AZStd::tuple_size<AZStd::decay_t<Tuple>>::value>{}))
    {
        return Internal::apply_impl(AZStd::forward<Fn>(f), AZStd::forward<Tuple>(tupleObj), AZStd::make_index_sequence<AZStd::tuple_size<AZStd::decay_t<Tuple>>::value>{});
    }
}

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
}


// Adds typeinfo specialization for tuple type
namespace AZ
{
    AZ_TYPE_INFO_INTERNAL_SPECIALIZED_TEMPLATE_PREFIX_UUID(AZStd::tuple, "tuple", "{F99F9308-DC3E-4384-9341-89CBF1ABD51E}", AZ_TYPE_INFO_INTERNAL_TYPENAME_VARARGS);
}
