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
    /*concept*/ constexpr bool tuple_like = Internal::is_tuple_like<remove_cvref_t<T>>;

    template<class T, class = void>
    /*concept*/ constexpr bool pair_like = false;

    template<class T>
    /*concept*/ constexpr bool
        pair_like<T, enable_if_t<tuple_like<T>>> = tuple_size_v<remove_cvref_t<T>> == 2;

    template<class... TTypes, class... UTypes, template<class> class TQual, template<class> class UQual>
    struct basic_common_reference<tuple<TTypes...>, tuple<UTypes...>, TQual, UQual>
        : enable_if_t<Internal::sfinae_trigger_v<tuple<common_reference_t<TQual<TTypes>, UQual<UTypes>>...>>,
        Internal::requirements_fulfilled>
    {
        using type = tuple<common_reference_t<TQual<TTypes>, UQual<UTypes>>...>;
    };

    template<class T1, class T2, class U1, class U2, template<class> class TQual, template<class> class UQual>
    struct basic_common_reference<pair<T1, T2>, pair<U1, U2>, TQual, UQual>
        : enable_if_t<Internal::sfinae_trigger_v<common_reference_t<TQual<T1>, UQual<U1>>, common_reference_t<TQual<T2>, UQual<U2>>>,
        Internal::requirements_fulfilled>
    {
        using type = pair<common_reference_t<TQual<T1>, UQual<U1>>, common_reference_t<TQual<T2>, UQual<U2>>>;
    };
}

//! common_type is from the std namespace.
//! Its name was brought into the AZStd namespace via a "using" directive.
//! Therefore it needs to be specialized in its original namespace.
namespace std
{
    template<class... TTypes, class... UTypes>
    struct common_type<tuple<TTypes...>, tuple<UTypes...>>
        : AZStd::enable_if_t<AZStd::Internal::sfinae_trigger_v<tuple<common_type_t<TTypes, UTypes>...>>,
        AZStd::Internal::requirements_fulfilled>
    {
        using type = tuple<common_type_t<TTypes, UTypes>...>;
    };
    template<class T1, class T2, class U1, class U2>
    struct common_type<AZStd::pair<T1, T2>, AZStd::pair<U1, U2>>
        : AZStd::enable_if_t<AZStd::Internal::sfinae_trigger_v<common_type_t<T1, U1>, common_type_t<T2, U2>>,
        AZStd::Internal::requirements_fulfilled>
    {
        using type = AZStd::pair<common_type_t<T1, U1>, common_type_t<T2, U2>>;
    };
}

// The tuple_size and tuple_element classes need to be specialized in the std:: namespace since the AZStd:: namespace alias them
// The tuple_size and tuple_element classes is to be specialized here for the AZStd::array class

namespace std
{
    // Suppressing clang warning error: 'tuple_size' defined as a class template here but previously declared as a struct template [-Werror,-Wmismatched-tags]
    AZ_PUSH_DISABLE_WARNING(, "-Wmismatched-tags")
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
