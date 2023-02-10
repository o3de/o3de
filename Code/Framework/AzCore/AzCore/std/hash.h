/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZSTD_HASH_H
#define AZSTD_HASH_H 1

#include <limits>
#include <AzCore/std/function/invoke.h>
#include <AzCore/std/utils.h>
#include <AzCore/std/typetraits/conjunction.h>
#include <AzCore/std/typetraits/disjunction.h>
#include <AzCore/std/typetraits/is_abstract.h>
#include <AzCore/std/typetraits/negation.h>
#include <AzCore/std/typetraits/remove_cv.h>

#include "typetraits/has_member_function.h"
#include "typetraits/is_abstract.h"

namespace AZStd
{
    /**
     * \name HashFuntions Hash Functions
     * Follow the standard 20.8.12
     * @{
     */

    /// Default template (just try to cast the value to size_t)
    template<class T>
    struct hash
    {
        typedef T               argument_type;
        typedef AZStd::size_t   result_type;
        // You do not need to implement this for your custom type.  Since this function calls
        // static_cast<result_type>(value); (and result type is usually size_t) you can also, instead of making an
        // explicit instantiation of hash<T>, supply a
        // [[nodiscard]] explicit constexpr operator size_t() const
        // function on your type, which will then get called by this default implementation.
        constexpr result_type operator()(const argument_type& value) const { return static_cast<result_type>(value); }
        static bool OnlyUnspecializedTypesShouldHaveThis() { return true; }
    };

    /// define your own struct AZStd::hash<T> and HasherInvocable<T>::type will be AZStd::true_type
    AZ_HAS_STATIC_MEMBER(DefaultHash, OnlyUnspecializedTypesShouldHaveThis, bool, ());

    template<class T, bool isConstructible = AZStd::is_constructible<T>::value && !AZStd::is_abstract<T>::value>
    struct IsNumber
    {
        static constexpr bool value = false;
    };

    template <typename T>
    struct IsNumber<T, true>
    {
        static constexpr bool value = std::numeric_limits<T>::is_specialized;
    };
    
    template<typename KeyType>
    using HasherInvocable = AZStd::conjunction
        < AZStd::disjunction<AZStd::negation<HasDefaultHash<hash<KeyType>>>, IsNumber<KeyType>, AZStd::is_enum<KeyType>>
        , AZStd::is_default_constructible<hash<KeyType>>
        , AZStd::is_invocable_r<size_t, hash<KeyType>, const KeyType&>>;

    template<typename KeyType>
    inline constexpr bool HasherInvocable_v = HasherInvocable<KeyType>::value;
    
    template<typename... Types>
    using hash_enabled_concept = AZStd::bool_constant<AZStd::conjunction_v<HasherInvocable<Types>...>>;

    template<typename... Types>
    inline constexpr bool hash_enabled_concept_v = hash_enabled_concept<Types...>::value;

    template< class T >
    struct hash< const T* >
    {
        typedef const T*        argument_type;
        typedef AZStd::size_t   result_type;
        constexpr result_type operator()(argument_type value) const
        {
            // Implementation by Alberto Barbati and Dave Harris.
            AZStd::size_t x = static_cast<AZStd::size_t>(reinterpret_cast<AZStd::ptrdiff_t>(value));
            return x + (x >> 3);
        }
    };

    template< class T >
    struct hash< T* >
    {
        typedef T* argument_type;
        typedef AZStd::size_t   result_type;
        constexpr result_type operator()(argument_type value) const
        {
            // Implementation by Alberto Barbati and Dave Harris.
            AZStd::size_t x = static_cast<AZStd::size_t>(reinterpret_cast<AZStd::ptrdiff_t>(value));
            return x + (x >> 3);
        }
    };

    template <class T>
    constexpr void hash_combine(AZStd::size_t& seed, T const& v)
    {
        hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    template <class T1, class T2, class... RestTypes>
    constexpr void hash_combine(AZStd::size_t& seed, const T1& firstElement, const T2& secondElement, const RestTypes&... restElements)
    {
        hash_combine(seed, firstElement);
        hash_combine(seed, secondElement, restElements...);
    }

    template <class It>
    constexpr AZStd::size_t hash_range(It first, It last)
    {
        AZStd::size_t seed = 0;
        for (; first != last; ++first)
        {
            hash_combine(seed, *first);
        }
        return seed;
    }

    template <class It>
    constexpr void hash_range(AZStd::size_t& seed, It first, It last)
    {
        for (; first != last; ++first)
        {
            hash_combine(seed, *first);
        }
    }

    template< class T, unsigned N >
    struct hash< const T[N] >
    {
        //typedef const T[N]        argument_type;
        typedef AZStd::size_t   result_type;
        constexpr result_type operator()(const T(&value)[N]) const { return hash_range(value, value + N); }
    };

    template< class T, unsigned N >
    struct hash< T[N] >
        : public hash<const T[N]>
    {
        //typedef T[N]          argument_type;
    };

    template<>
    struct hash< float >
    {
        typedef float           argument_type;
        typedef AZStd::size_t   result_type;
        inline result_type operator()(argument_type value) const
        {
            union // cast via union to get around "strict type" compiler warnings
            {
                float* cast_float;
                AZ::u32* cast_u32;
            };
            cast_float = &value;
            AZ::u32 bits = *cast_u32;
            return static_cast<result_type>((bits == 0x80000000 ? 0 : bits));
        }
    };

    template<>
    struct hash< double >
    {
        typedef double          argument_type;
        typedef AZStd::size_t   result_type;
        inline result_type operator()(argument_type value) const
        {
            union // cast via union to get around "strict type" compiler warnings
            {
                double* cast_double;
                AZ::u64* cast_u64;
            };
            cast_double = &value;
            AZ::u64 bits = *cast_u64;
            return static_cast<result_type>((bits == 0x7fffffffffffffff ? 0 : bits));
        }
    };

    template<>
    struct hash< long double >
    {
        typedef long double     argument_type;
        typedef AZStd::size_t   result_type;
        inline result_type operator()(argument_type value) const
        {
            union // cast via union to get around "strict type" compiler warnings
            {
                long double* cast_long_double;
                AZ::u64* cast_u64; // use first 64 bits
            };
            cast_long_double = &value;
            AZ::u64 bits = *cast_u64;
            return static_cast<result_type>((bits == 0x7fffffffffffffff ? 0 : bits));
        }
    };

    template<typename T, typename U>
    struct hash< AZStd::pair<T, U> >
    {
        template<typename A, typename B, typename = AZStd::enable_if_t<hash_enabled_concept_v<A, B>>>
        constexpr size_t operator()(const AZStd::pair<A, B>& value) const
        {
            size_t seed = 0;
            hash_combine(seed, value.first);
            hash_combine(seed, value.second);
            return seed;
        }
    };
    //@} Hash functions

    // Bucket size suitable to hold n elements.
    AZStd::size_t hash_next_bucket_size(AZStd::size_t n);
}

#endif // AZSTD_HASH_H
#pragma once
