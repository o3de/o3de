/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/ranges/ranges.h>

namespace AZStd::Internal
{
    // Exposition only concept for determining if the reference type being stored in a range
    // is convertible to T
    // https://eel.is/c++draft/container.requirements#container.alloc.reqmts-3
    template<class R, class T>
    concept container_compatible_range =
        ranges::input_range<R>
        && convertible_to<conditional_t<is_lvalue_reference_v<R>, ranges::range_reference_t<R>, ranges::range_rvalue_reference_t<R>>, T>;
}
