/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/base.h>
#include <AzCore/std/typetraits/void_t.h>
#include <AzCore/std/utils.h>

namespace AZStd
{
    namespace Internal
    {
        // The second template parameter is to the template parameter of the operator function that called is_transparent to to trigger SFINAE
        // error instead of a hard error
        // i.e 
        // template<typename ComparableToKey>
        // node_ptr_type DoUpperBound(const ComparableToKey& key, Internal::is_transparent<Compare>::value) const
        // will not cause a substitution failure if ComparableToKey is not used as part of the is_transparent template and therefore will cause a hard error
        //
        // template<typename ComparableToKey>
        // node_ptr_type DoUpperBound(const ComparableToKey& key, Internal::is_transparent<Compare, ComparableToKey>::value) const
        // will cause a substitution error in this case if the Compare function is missing the is_transparent type alias and therefore can proceed
        // to use the next template candidate
        template <class T, class Unused, class = void>
        struct is_transparent
            : false_type {};

        template <class T, class Unused>
        struct is_transparent<T, Unused, void_t<typename T::is_transparent>>
            : true_type {};
    }
    // Functors as required by the standard as of C++17.

    // 20.3.2, arithmetic operations:
    template <class T = void>
    struct plus
    {
        constexpr T operator()(const T& left, const T& right) const { return left + right; }
    };
    template <>
    struct plus<void>
    {
        template <class T, class U>
        constexpr auto operator()(T&& left, U&& right) const -> decltype(AZStd::forward<T>(left) + AZStd::forward<U>(right))
        {
            return AZStd::forward<T>(left) + AZStd::forward<U>(right);
        }
        using is_transparent = void;
    };
    template <class T = void>
    struct minus
    {
        constexpr T operator()(const T& left, const T& right) const { return left - right; }
    };
    template <>
    struct minus<void>
    {
        template <class T, class U>
        constexpr auto operator()(T&& left, U&& right) const -> decltype(AZStd::forward<T>(left) - AZStd::forward<U>(right))
        {
            return AZStd::forward<T>(left) - AZStd::forward<U>(right);
        }
        using is_transparent = void;
    };
    template <class T = void>
    struct multiplies
    {
        constexpr T operator()(const T& left, const T& right) const { return left * right; }
    };
    template <>
    struct multiplies<void>
    {
        template <class T, class U>
        constexpr auto operator()(T&& left, U&& right) const -> decltype(AZStd::forward<T>(left) * AZStd::forward<U>(right))
        {
            return AZStd::forward<T>(left) * AZStd::forward<U>(right);
        }
        using is_transparent = void;
    };
    template <class T = void>
    struct divides
    {
        constexpr T operator()(const T& left, const T& right) const { return left / right; }
    };
    template <>
    struct divides<void>
    {
        template <class T, class U>
        constexpr auto operator()(T&& left, U&& right) const -> decltype(AZStd::forward<T>(left) / AZStd::forward<U>(right))
        {
            return AZStd::forward<T>(left) / AZStd::forward<U>(right);
        }
        using is_transparent = void;
    };
    template <class T = void>
    struct modulus
    {
        constexpr T operator()(const T& left, const T& right) const { return left % right; }
    };
    template <>
    struct modulus<void>
    {
        template <class T, class U>
        constexpr auto operator()(T&& left, U&& right) const -> decltype(AZStd::forward<T>(left) % AZStd::forward<U>(right))
        {
            return AZStd::forward<T>(left) % AZStd::forward<U>(right);
        }
        using is_transparent = void;
    };
    template <class T = void>
    struct negate
    {
        constexpr T operator()(const T& left) const { return -left; }
    };
    template <>
    struct negate<void>
    {
        template <class T>
        constexpr auto operator()(T&& left) const -> decltype(-AZStd::forward<T>(left))
        {
            return -AZStd::forward<T>(left);
        }
        using is_transparent = void;
    };

    // 20.3.3, comparisons:
    template <class T = void>
    struct equal_to
    {
        constexpr bool operator()(const T& left, const T& right) const { return left == right; }
    };
    template <>
    struct equal_to<void>
    {
        template <class T, class U>
        constexpr auto operator()(T&& left, U&& right) const -> decltype(AZStd::forward<T>(left) == AZStd::forward<U>(right))
        {
            return AZStd::forward<T>(left) == AZStd::forward<U>(right);
        }
        using is_transparent = void;
    };
    template <class T = void>
    struct not_equal_to
    {
        constexpr bool operator()(const T& left, const T& right) const { return left != right; }
    };
    template <>
    struct not_equal_to<void>
    {
        template <class T, class U>
        constexpr auto operator()(T&& left, U&& right) const -> decltype(AZStd::forward<T>(left) != AZStd::forward<U>(right))
        {
            return AZStd::forward<T>(left) != AZStd::forward<U>(right);
        }
        using is_transparent = void;
    };
    template <class T = void>
    struct greater
    {
        constexpr bool operator()(const T& left, const T& right) const { return left > right; }
    };
    template <>
    struct greater<void>
    {
        template <class T, class U>
        constexpr auto operator()(T&& left, U&& right) const -> decltype(AZStd::forward<T>(left) > AZStd::forward<U>(right))
        {
            return AZStd::forward<T>(left) > AZStd::forward<U>(right);
        }
        using is_transparent = void;
    };
    template <class T = void>
    struct less
    {
        constexpr bool operator()(const T& left, const T& right) const { return left < right; }
    };
    template <>
    struct less<void>
    {
        template <class T, class U>
        constexpr auto operator()(T&& left, U&& right) const -> decltype(AZStd::forward<T>(left) < AZStd::forward<U>(right))
        {
            return AZStd::forward<T>(left) < AZStd::forward<U>(right);
        }
        using is_transparent = void;
    };
    template <class T = void>
    struct greater_equal
    {
        constexpr bool operator()(const T& left, const T& right) const { return left >= right; }
    };
    template <>
    struct greater_equal<void>
    {
        template <class T, class U>
        constexpr auto operator()(T&& left, U&& right) const -> decltype(AZStd::forward<T>(left) >= AZStd::forward<U>(right))
        {
            return AZStd::forward<T>(left) >= AZStd::forward<U>(right);
        }
        using is_transparent = void;
    };
    template <class T = void>
    struct less_equal
    {
        constexpr bool operator()(const T& left, const T& right) const { return left <= right; }
    };
    template <>
    struct less_equal<void>
    {
        template <class T, class U>
        constexpr auto operator()(T&& left, U&& right) const -> decltype(AZStd::forward<T>(left) <= AZStd::forward<U>(right))
        {
            return AZStd::forward<T>(left) <= AZStd::forward<U>(right);
        }
        using is_transparent = void;
    };

    // 20.3.4, logical operations:
    template <class T = void>
    struct logical_and
    {
        constexpr bool operator()(const T& left, const T& right) const { return left && right; }
    };
    template <>
    struct logical_and<void>
    {
        template <class T, class U>
        constexpr auto operator()(T&& left, U&& right) const -> decltype(AZStd::forward<T>(left) && AZStd::forward<U>(right))
        {
            return AZStd::forward<T>(left) && AZStd::forward<U>(right);
        }
        using is_transparent = void;
    };
    template <class T = void>
    struct logical_or
    {
        constexpr bool operator()(const T& left, const T& right) const { return left || right; }
    };
    template <>
    struct logical_or<void>
    {
        template <class T, class U>
        constexpr auto operator()(T&& left, U&& right) const -> decltype(AZStd::forward<T>(left) || AZStd::forward<U>(right))
        {
            return AZStd::forward<T>(left) || AZStd::forward<U>(right);
        }
        using is_transparent = void;
    };
    template <class T = void>
    struct logical_not
    {
        constexpr bool operator()(const T& left) const { return !left; }
    };
    template <>
    struct logical_not<void>
    {
        template <class T>
        constexpr auto operator()(T&& left) const -> decltype(!AZStd::forward<T>(left))
        {
            return !AZStd::forward<T>(left);
        }
        using is_transparent = void;
    };

    // 20.14.10 bitwise operations:
    template <class T = void>
    struct bit_and
    {
        constexpr T operator()(const T& left, const T& right) const { return left & right; }
    };
    template <>
    struct bit_and<void>
    {
        template <class T, class U>
        constexpr auto operator()(T&& left, U&& right) const -> decltype(AZStd::forward<T>(left) & AZStd::forward<U>(right))
        {
            return AZStd::forward<T>(left) & AZStd::forward<U>(right);
        }
        using is_transparent = void;
    };
    template <class T = void>
    struct bit_or
    {
        constexpr T operator()(const T& left, const T& right) const { return left | right; }
    };
    template <>
    struct bit_or<void>
    {
        template <class T, class U>
        constexpr auto operator()(T&& left, U&& right) const -> decltype(AZStd::forward<T>(left) | AZStd::forward<U>(right))
        {
            return AZStd::forward<T>(left) | AZStd::forward<U>(right);
        }
        using is_transparent = void;
    };
    template <class T = void>
    struct bit_xor
    {
        constexpr T operator()(const T& left, const T& right) const { return left ^ right; }
    };
    template <>
    struct bit_xor<void>
    {
        template <class T, class U>
        constexpr auto operator()(T&& left, U&& right) const -> decltype(AZStd::forward<T>(left) ^ AZStd::forward<U>(right))
        {
            return AZStd::forward<T>(left) ^ AZStd::forward<U>(right);
        }
        using is_transparent = void;
    };
    template <class T = void>
    struct bit_not
    {
        constexpr T operator()(const T& left) const { return ~left; }
    };
    template <>
    struct bit_not<void>
    {
        template <class T>
        constexpr auto operator()(T&& left) const -> decltype(~AZStd::forward<T>(left))
        {
            return ~AZStd::forward<T>(left);
        }
        using is_transparent = void;
    };
}
