/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/array_fwd.h>
#include <AzCore/std/ranges/subrange_fwd.h>
#include <AzCore/std/utility/tuple_fwd.h>
#include <AzCore/std/utility/pair_fwd.h>

namespace AZStd::Internal
{
    template<class T>
    constexpr bool is_tuple_like = false;

    template <class... Ts>
    constexpr bool is_tuple_like<tuple<Ts...>> = true;

    template<class T1, class T2>
    constexpr bool is_tuple_like<pair<T1, T2>> = true;

    template<class T, size_t N>
    constexpr bool is_tuple_like<array<T, N>> = true;

    template<class I, class S, ranges::subrange_kind K>
    constexpr bool is_tuple_like<ranges::subrange<I, S, K>> = true;

    template<class T>
    constexpr bool is_tuple = false;
    template<class... Ts>
    constexpr bool is_tuple<tuple<Ts...>> = true;

    template<class T>
    constexpr bool is_subrange_impl = false;

    template<class I, class S, ranges::subrange_kind K>
    constexpr bool is_subrange_impl<ranges::subrange<I, S, K>> = true;

    template<class T>
    constexpr bool is_subrange = is_subrange_impl<remove_cvref_t<T>>;

    struct requirements_fulfilled {};
}

namespace AZStd
{
    // Add tuple specialization for common_type and common_reference so that it can be used in range algorithms
    template<class T>
    concept tuple_like = Internal::is_tuple_like<remove_cvref_t<T>>;

    template<class T>
    concept pair_like =
        tuple_like<T>
        && tuple_size_v<remove_cvref_t<T>> == 2;
}

//! common_type and basic_common_reference are from the std namespace.
//! Their names are brought into the AZStd namespace via a "using" directive.
//! Therefore they need to be specialized in their original namespace.
namespace std
{
    template<class... TTypes, class... UTypes, template<class> class TQual, template<class> class UQual>
        requires requires { typename AZStd::tuple<AZStd::common_reference_t<TQual<TTypes>, UQual<UTypes>>...>; }
    struct basic_common_reference<AZStd::tuple<TTypes...>, AZStd::tuple<UTypes...>, TQual, UQual>
    {
        using type = AZStd::tuple<AZStd::common_reference_t<TQual<TTypes>, UQual<UTypes>>...>;
    };

    template<class T1, class T2, class U1, class U2, template<class> class TQual, template<class> class UQual>
        requires requires {
            typename AZStd::common_reference_t<TQual<T1>, UQual<U1>>;
            typename AZStd::common_reference_t<TQual<T2>, UQual<U2>>;
        }
    struct basic_common_reference<AZStd::pair<T1, T2>, AZStd::pair<U1, U2>, TQual, UQual>
    {
        using type = AZStd::pair<AZStd::common_reference_t<TQual<T1>, UQual<U1>>, AZStd::common_reference_t<TQual<T2>, UQual<U2>>>;
    };

    template<class... TTypes, class... UTypes>
        requires requires { typename tuple<common_type_t<TTypes, UTypes>...>; }
    struct common_type<tuple<TTypes...>, tuple<UTypes...>>
    {
        using type = tuple<common_type_t<TTypes, UTypes>...>;
    };
    template<class T1, class T2, class U1, class U2>
        requires requires {
            typename common_type_t<T1, U1>;
            typename common_type_t<T2, U2>;
        }
    struct common_type<AZStd::pair<T1, T2>, AZStd::pair<U1, U2>>
    {
        using type = AZStd::pair<common_type_t<T1, U1>, common_type_t<T2, U2>>;
    };
}
