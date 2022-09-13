/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/concepts/concepts.h>
#include <AzCore/std/typetraits/type_identity.h>


namespace AZStd::ranges::Internal
{
    // Take advantage of type_identity type to "store" a type alias
    template<class IntType>
    static constexpr auto get_wider_signed_integer_type()
    {
        if constexpr (sizeof(IntType) < sizeof(AZ::s16))
        {
            return type_identity<AZ::s16>{};
        }
        else if constexpr (sizeof(IntType) < sizeof(AZ::s32))
        {
            return type_identity<AZ::s32>{};
        }
        else
        {
            return type_identity<AZ::s64>{};
        }

        static_assert(!::AZStd::Internal::is_integer_like<IntType> || sizeof(IntType) <= sizeof(AZ::s64),
            "integer type which is larger than the largest signed integer type has been found.");
    }

    template<class IntType>
    using get_wider_signed_integer_type_t = typename decltype(get_wider_signed_integer_type<IntType>())::type;

    template<class W>
    using IOTA_DIFF_T = conditional_t<!::AZStd::Internal::is_integer_like<W> || (sizeof(iter_difference_t<W>) > sizeof(W)),
        iter_difference_t<W>, get_wider_signed_integer_type_t<W>>;
}
