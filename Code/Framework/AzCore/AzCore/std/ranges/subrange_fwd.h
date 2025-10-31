/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/concepts/concepts.h>

// Forward declare the subrange class to prevent a circular include with needing to know about the tuple type in order
// to define the pair-like concept
namespace AZStd::ranges
{
    enum class subrange_kind : bool
    {
        unsized,
        sized
    };
    template<class I, class S = I, subrange_kind K = sized_sentinel_for<S, I> ? subrange_kind::sized : subrange_kind::unsized, class = void>
    class subrange;
}
