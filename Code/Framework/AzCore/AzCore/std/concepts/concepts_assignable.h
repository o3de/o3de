/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once


#include <AzCore/std/typetraits/common_reference.h>
#include <AzCore/std/typetraits/conjunction.h>
#include <AzCore/std/typetraits/integral_constant.h>
#include <AzCore/std/typetraits/is_same.h>
#include <AzCore/std/typetraits/remove_cvref.h>
#include <AzCore/std/utility/declval.h>

namespace AZStd::Internal
{

    template<class LHS, class RHS, class = void>
    constexpr bool assignable_from_impl = false;
    template<class LHS, class RHS>
    constexpr bool assignable_from_impl<LHS, RHS, enable_if_t<conjunction_v<
        is_lvalue_reference<LHS>,
        bool_constant<common_reference_with<const remove_reference_t<LHS>&, const remove_reference_t<RHS>&>>,
        bool_constant<same_as<decltype(declval<LHS>() = declval<RHS>()), LHS>> >>> = true;
}

namespace AZStd
{
    template<class LHS, class RHS>
    /*concept*/ constexpr bool assignable_from = Internal::assignable_from_impl<LHS, RHS>;
}
