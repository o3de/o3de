/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZSTD_UTILS_H
#define AZSTD_UTILS_H 1

#include <AzCore/std/base.h>
#include <AzCore/std/typetraits/integral_constant.h>
#include <AzCore/std/typetraits/intrinsics.h>
#include <AzCore/std/typetraits/conditional.h>
#include <AzCore/std/typetraits/remove_reference.h>
#include <AzCore/std/typetraits/remove_const.h>
#include <AzCore/std/typetraits/is_enum.h>
#include <AzCore/std/typetraits/underlying_type.h>
#include <AzCore/std/typetraits/add_reference.h>
#include <AzCore/std/reference_wrapper.h>
#include <AzCore/std/typetraits/remove_reference.h>
#include <AzCore/std/typetraits/is_convertible.h>
#include <AzCore/std/typetraits/is_lvalue_reference.h>
#include <AzCore/std/typetraits/void_t.h>

#include <memory>

namespace AZStd
{
    //////////////////////////////////////////////////////////////////////////
    // rvalue
    // rvalue move
    template<class T>
    constexpr AZStd::remove_reference_t<T>&& move(T && t)
    {
        return static_cast<AZStd::remove_reference_t<T>&&>(t);
    }

    using std::forward;
    using std::declval;
    using std::exchange;

    template <class T>
    struct default_delete
    {
        template <class U, 
            class = typename enable_if<is_convertible<U*, T*>::value, void>::type>
        void operator()(U* ptr) const
        {
            delete ptr;
        }
    };

    template <class T>
    struct default_delete<T[]>
    {
        template<class U,
            class = typename enable_if<is_convertible<U(*)[], T(*)[]>::value, void>::type>
        default_delete(const default_delete<U[]>&)
        {
        }

        template<class U,
            class = typename enable_if<is_convertible<U(*)[], T(*)[]>::value, void>::type>
        void operator()(U *ptr) const
        {
            delete[] ptr;
        }
    };

    template <class T>
    struct no_delete
    {
        template <class U>
        void operator()(U*) const {}
    };
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Swap
    using std::swap;

    template<class ForwardIterator1, class ForwardIterator2>
    AZ_FORCE_INLINE void                    iter_swap(ForwardIterator1 a, ForwardIterator2 b)
    {
        AZStd::swap(*a, *b);
    }

    template<class ForwardIterator1, class ForwardIterator2>
    AZ_FORCE_INLINE ForwardIterator2       swap_ranges(ForwardIterator1 first1, ForwardIterator1 last1, ForwardIterator2 first2)
    {
        for (; first1 != last1; ++first1, ++first2)
        {
            AZStd::iter_swap(first1, first2);
        }

        return first2;
    }
    //////////////////////////////////////////////////////////////////////////

    // The structure that encapsulates index lists
    template <size_t... Is>
    struct index_sequence
    {
        static constexpr size_t size = sizeof...(Is);
    };

    // Collects internal details for generating index ranges [MIN, MAX)
    namespace Internal
    {
        // Declare primary template for index range builder
        template <size_t MIN, size_t N, size_t... Is>
        struct range_builder;

        // Base step
        template <size_t MIN, size_t... Is>
        struct range_builder<MIN, MIN, Is...>
        {
            typedef index_sequence<Is...> type;
        };

        // Induction step
        template <size_t MIN, size_t N, size_t... Is>
        struct range_builder : public range_builder<MIN, N - 1, N - 1, Is...>
        {
        };
    }

    // Create index range [0,N]
    template<size_t N>
    using make_index_sequence = typename Internal::range_builder<0, N>::type;

    struct piecewise_construct_t {};
    static constexpr piecewise_construct_t piecewise_construct{};

    template<class T1, class T2>
    struct pair
    {   // store a pair of values
        typedef pair<T1, T2>    this_type;
        typedef T1              first_type;
        typedef T2              second_type;

        /// Construct from defaults
        constexpr pair()
            : first(T1())
            , second(T2()) {}
        /// Constructs only the first element, default the second.
        constexpr pair(const T1& value1)
            : first(value1)
            , second(T2()) {}
        /// Construct from specified values.
        constexpr pair(const T1& value1, const T2& value2)
            : first(value1)
            , second(value2) {}
        /// Copy constructor
        constexpr pair(const this_type& rhs)
            : first(rhs.first)
            , second(rhs.second) {}
        // construct from compatible pair
        template<class Other1, class Other2>
        constexpr pair(const pair<Other1, Other2>& rhs)
            : first(rhs.first)
            , second(rhs.second) {}

        using TT1 = AZStd::remove_reference_t<T1>;
        using TT2 = AZStd::remove_reference_t<T2>;

        constexpr pair(TT1&& value1, TT2&& value2)
            : first(AZStd::move(value1))
            , second(AZStd::move(value2)) {}
        constexpr pair(const TT1& value1, TT2&& value2)
            : first(value1)
            , second(AZStd::move(value2)) {}
        constexpr pair(TT1&& value1, const TT2& value2)
            : first(AZStd::move(value1))
            , second(value2) {}
        template<class Other1, class Other2>
        constexpr pair(Other1&& value1, Other2&& value2)
            : first(AZStd::forward<Other1>(value1))
            , second(AZStd::forward<Other2>(value2)) {}
        constexpr pair(pair&& rhs)
            : first(AZStd::move(rhs.first))
            , second(AZStd::move(rhs.second)) {}
        template<class Other1, class Other2>
        constexpr pair(pair<Other1, Other2>&& rhs)
            : first(AZStd::forward<Other1>(rhs.first))
            , second(AZStd::forward<Other2>(rhs.second)) {}

        template<template<class...> class TupleType, class... Args1, class... Args2>
        constexpr pair(piecewise_construct_t,
            TupleType<Args1...> first_args,
            TupleType<Args2...> second_args);

        template<template<class...> class TupleType, class... Args1, class... Args2, size_t... I1, size_t... I2>
        constexpr pair(piecewise_construct_t, TupleType<Args1...>& first_args,
            TupleType<Args2...>& second_args, AZStd::index_sequence<I1...>,
            AZStd::index_sequence<I2...>);

        constexpr this_type& operator=(this_type&& rhs)
        {
            first = AZStd::move(rhs.first);
            second = AZStd::move(rhs.second);
            return *this;
        }

        template<class Other1, class Other2>
        constexpr this_type& operator=(const pair<Other1, Other2>&& rhs)
        {
            first = AZStd::move(rhs.first);
            second = AZStd::move(rhs.second);
            return *this;
        }

        void swap(this_type& rhs)
        {
            // exchange contents with _Right
            if (this != &rhs)
            {   // different, worth swapping
                AZStd::swap(first, rhs.first);
                AZStd::swap(second, rhs.second);
            }
        }

        constexpr this_type& operator=(const this_type& rhs)
        {
            first = rhs.first;
            second = rhs.second;
            return *this;
        }

        template<class Other1, class Other2>
        constexpr this_type& operator=(const pair<Other1, Other2>& rhs)
        {
            first = rhs.first;
            second = rhs.second;
            return *this;
        }

        T1 first;   // the first stored value
        T2 second;  // the second stored value
    };

    // pair
    template<class T1, class T2>
    constexpr void swap(AZStd::pair<T1, T2>& left, AZStd::pair<T1, T2>& _Right)
    {   // swap _Left and _Right pairs
        left.swap(_Right);
    }

    template<class L1, class L2, class R1, class R2, class = AZStd::void_t<decltype(declval<L1>() == declval<R1>() && declval<L2>() == declval<R2>())>>
    constexpr bool operator==(const pair<L1, L2>& left, const pair<R1, R2>& right)
    {   // test for pair equality
        return left.first == right.first && left.second == right.second;
    }

    template<class L1, class L2, class R1, class R2, class = AZStd::void_t<decltype(declval<L1>() == declval<R1>() && declval<L2>() == declval<R2>())>>
    constexpr bool operator!=(const pair<L1, L2>& left, const pair<R1, R2>& right)
    {   // test for pair inequality
        return !(left == right);
    }

    template<class L1, class L2, class R1, class R2, class = AZStd::void_t<decltype(declval<L1>() < declval<R1>() || (!(declval<R1>() < declval<L1>()) && declval<L2>() < declval<R2>()))>>
    constexpr bool operator<(const pair<L1, L2>& left, const pair<R1, R2>& right)
    {   // test if left < right for pairs
        return (left.first < right.first || (!(right.first < left.first) && left.second < right.second));
    }

    template<class L1, class L2, class R1, class R2, class = AZStd::void_t<decltype(declval<L1>() < declval<R1>() || (!(declval<R1>() < declval<L1>()) && declval<L2>() < declval<R2>()))>>
    constexpr bool operator>(const pair<L1, L2>& left, const pair<R1, R2>& right)
    {   // test if left > right for pairs
        return right < left;
    }

    template<class L1, class L2, class R1, class R2, class = AZStd::void_t<decltype(declval<L1>() < declval<R1>() || (!(declval<R1>() < declval<L1>()) && declval<L2>() < declval<R2>()))>>
    constexpr bool operator<=(const pair<L1, L2>& left, const pair<R1, R2>& right)
    {   // test if left <= right for pairs
        return !(right < left);
    }

    template<class L1, class L2, class R1, class R2, class = AZStd::void_t<decltype(declval<L1>() < declval<R1>() || (!(declval<R1>() < declval<L1>()) && declval<L2>() < declval<R2>()))>>
    constexpr bool operator>=(const pair<L1, L2>& left, const pair<R1, R2>& right)
    {   // test if left >= right for pairs
        return !(left < right);
    }

    //////////////////////////////////////////////////////////////////////////
    // Address of
    //////////////////////////////////////////////////////////////////////////
    namespace Internal
    {
        template<class T>
        struct addr_impl_ref
        {
            T& m_v;
            constexpr addr_impl_ref(T& v)
                : m_v(v) {}
            constexpr addr_impl_ref& operator=(const addr_impl_ref& v) { m_v = v; }
            constexpr operator T& () const { return m_v; }
        };

        template<class T>
        struct addressof_impl
        {
            static AZ_FORCE_INLINE T* f(T& v, long) { return reinterpret_cast<T*>(&const_cast<char&>(reinterpret_cast<const volatile char&>(v))); }
            static constexpr T* f(T* v, int)  { return v; }
        };
    }

    template<class T>
    T* addressof(T& v)
    {
        return Internal::addressof_impl<T>::f(Internal::addr_impl_ref<T>(v), 0);
    }
    // End addressof
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Ref wrapper
    template<class T>
    constexpr T* get_pointer(reference_wrapper<T> const& r)
    {
        return &r.get();
    }
    // end reference_wrapper
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // get_pointer
    // get_pointer(p) extracts a ->* capable pointer from p
    template<class T>
    constexpr T* get_pointer(T* p)    { return p; }
    //template<class T> T * get_pointer(std::auto_ptr<T> const& p)
    //{
    //  return p.get();
    //}
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // make_pair
    template<class T1, class T2>
    constexpr pair<typename AZStd::unwrap_reference<T1>::type, typename AZStd::unwrap_reference<T2>::type>
    make_pair(T1&& value1, T2&& value2)
    {
        typedef pair<typename AZStd::unwrap_reference<T1>::type, typename AZStd::unwrap_reference<T2>::type> pair_type;
        return pair_type(AZStd::forward<T1>(value1), AZStd::forward<T2>(value2));
    }

    template<class T1, class T2>
    constexpr pair<typename AZStd::unwrap_reference<T1>::type, typename AZStd::unwrap_reference<T2>::type>
    make_pair(const T1& value1, T2&& value2)
    {
        typedef pair<typename AZStd::unwrap_reference<T1>::type, typename AZStd::unwrap_reference<T2>::type> pair_type;
        return pair_type((typename AZStd::unwrap_reference<T1>::type)value1, AZStd::forward<T2>(value2));
    }

    template<class T1, class T2>
    constexpr pair<typename AZStd::unwrap_reference<T1>::type, typename AZStd::unwrap_reference<T2>::type>
    make_pair(T1&& value1, const T2& value2)
    {
        typedef pair<typename AZStd::unwrap_reference<T1>::type, typename AZStd::unwrap_reference<T2>::type> pair_type;
        return pair_type(AZStd::forward<T1>(value1), (typename AZStd::unwrap_reference<T2>::type)value2);
    }
    template<class T1, class T2>
    constexpr pair<typename AZStd::unwrap_reference<T1>::type, typename AZStd::unwrap_reference<T2>::type>
    make_pair(const T1& value1, const T2& value2)
    {
        typedef pair<typename AZStd::unwrap_reference<T1>::type, typename AZStd::unwrap_reference<T2>::type> pair_type;
        return pair_type((typename AZStd::unwrap_reference<T1>::type)value1, (typename AZStd::unwrap_reference<T2>::type)value2);
    }
    //////////////////////////////////////////////////////////////////////////

    template<class T, bool isEnum = AZStd::is_enum<T>::value>
    struct RemoveEnum
    {
        typedef typename AZStd::underlying_type<T>::type type;
    };

    template<class T>
    struct RemoveEnum<T, false>
    {
        typedef T type;
    };

    template<class T>
    using RemoveEnumT = typename RemoveEnum<T>::type;

    template<class T>
    struct HandleLambdaPointer;

    template<class R, class L, class... Args>
    struct HandleLambdaPointer<R(L::*)(Args...)>
    {
        typedef R(type)(Args...);
    };

    template<class R, class L, class... Args>
    struct HandleLambdaPointer<R(L::*)(Args...) const>
    {
        typedef R(type)(Args...);
    };

    template<class T>
    struct RemoveFunctionConst
    {
        typedef typename HandleLambdaPointer<decltype(&T::operator())>::type type; // lambda matching handling
    };

    template<class R, class... Args>
    struct RemoveFunctionConst<R(Args...)>
    {
        typedef R(type)(Args...);
    };

#if __cpp_noexcept_function_type
    // C++17 makes exception specifications as part of the type in paper P0012R1
    // Therefore noexcept overloads must distinguished from non-noexcept overloads
    template<class R, class... Args>
    struct RemoveFunctionConst<R(Args...) noexcept>
    {
        typedef R(type)(Args...) noexcept;
    };
#endif

    template<class R, class C, class... Args>
    struct RemoveFunctionConst<R(C::*)(Args...)>
    {
        using type = R(C::*)(Args...);
    };

#if __cpp_noexcept_function_type
    // C++17 makes exception specifications as part of the type in paper P0012R1
    // Therefore noexcept overloads must distinguished from non-noexcept overloads
    template<class R, class C, class... Args>
    struct RemoveFunctionConst<R(C::*)(Args...) noexcept>
    {
        using type = R(C::*)(Args...) noexcept;
    };
#endif

    template<class R, class C, class... Args>
    struct RemoveFunctionConst<R(C::*)(Args...) const>
    {
        using type = R(C::*)(Args...);
    };

#if __cpp_noexcept_function_type
    // C++17 makes exception specifications as part of the type in paper P0012R1
    // Therefore noexcept overloads must distinguished from non-noexcept overloads
    template<class R, class C, class... Args>
    struct RemoveFunctionConst<R(C::*)(Args...) const noexcept>
    {
        using type = R(C::*)(Args...) noexcept;
    };
#endif

    template <template <class> class, typename...>
    struct sequence_and;

    template <template <class> class trait, typename T1, typename... Ts>
    struct sequence_and<trait, T1, Ts...>
    {
        static const bool value = trait<T1>::value && sequence_and<trait, Ts...>::value;
    };

    template <template <class> class trait>
    struct sequence_and<trait>
    {
        static const bool value = true;
    };

    template <template <class> class, typename...>
    struct sequence_or;

    template <template <class> class trait, typename T1, typename... Ts>
    struct sequence_or<trait, T1, Ts...>
    {
        static const bool value = trait<T1>::value || sequence_or<trait, Ts...>::value;
    };

    template <template <class> class trait>
    struct sequence_or<trait>
    {
        static const bool value = false;
    };

    struct in_place_t
    {
        explicit constexpr in_place_t() = default;
    };
    constexpr in_place_t in_place{};

    template<typename T>
    struct in_place_type_t
    {
        explicit constexpr in_place_type_t() = default;
    };
    template <typename T>
    constexpr in_place_type_t<T> in_place_type{};

    template<size_t I>
    struct in_place_index_t
    {
        explicit constexpr in_place_index_t() = default;
    };
    template<size_t I>
    constexpr in_place_index_t<I> in_place_index{}; 

    namespace Internal
    {
        template <typename T>
        struct is_in_place_index
            : false_type
        {};

        template <size_t Index>
        struct is_in_place_index<in_place_index_t<Index>>
            : true_type
        {};
        template <typename T>
        using is_in_place_index_t = typename is_in_place_index<T>::type;
    }
}

#endif // AZSTD_UTILS_H
#pragma once
