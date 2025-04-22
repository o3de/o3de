/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/typetraits/disjunction.h>
#include <AzCore/std/typetraits/is_array.h>
#include <AzCore/std/typetraits/is_function.h>
#include <AzCore/std/typetraits/is_reference.h>
#include <AzCore/std/typetraits/is_void.h>
#include <AzCore/std/typetraits/type_identity.h>

namespace AZStd::Internal
{
    // Extension to implement a type trait that returns false_type
    // for an incomplete class, union or bounded array of incomplete class or union type
    // An incomplete class of unbounded array is not one definition used and doesn't need to be checked
    template<class T, size_t = sizeof(T)>
    constexpr true_type is_complete_or_unbounded_impl(type_identity<T>);

    template<class TypeIdentity, class NestedType = typename TypeIdentity::type>
    constexpr auto is_complete_or_unbounded_impl(TypeIdentity)
        -> disjunction<
        is_reference<NestedType>,
        is_function<NestedType>,
        is_void<NestedType>,
        is_unbounded_array<NestedType>
        >;

    template<class T>
    using is_complete_or_unbounded = decltype(is_complete_or_unbounded_impl(type_identity<T>{}));

    template<class T>
    constexpr bool is_complete_or_unbounded_v = is_complete_or_unbounded<T>();
}
