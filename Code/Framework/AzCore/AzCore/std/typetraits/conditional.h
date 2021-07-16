/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/typetraits/config.h>

namespace AZStd
{
    using std::conditional;
    using std::conditional_t;

    using std::enable_if;
    using std::enable_if_t;
} // namespace AZStd

// Backwards comaptible AZStd extensions for std::conditional and std::enable_if
namespace AZStd::Utils
{
    // Use AZStd::conditional_t instead
    template<bool Condition, typename T, typename F>
    using if_c = AZStd::conditional<Condition, T, F>;
    // Use AZStd::enable_if_t instead
    template<bool Condition, typename T = void>
    using enable_if_c = AZStd::enable_if<Condition, T>;
}
