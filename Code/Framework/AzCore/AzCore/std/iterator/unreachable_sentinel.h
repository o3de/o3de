/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/base.h>
#include <AzCore/std/concepts/concepts.h>


namespace AZStd
{
    //! Represents the upper bound of an unbounded range
    //! It is never equal to the iterator being compared against.
    struct unreachable_sentinel_t
    {
        template<class I, class = enable_if_t<weakly_incrementable<I>>>
        friend bool operator==(unreachable_sentinel_t, const I&) noexcept
        {
            return false;
        }
        template<class I, class = enable_if_t<weakly_incrementable<I>>>
        friend bool operator!=(unreachable_sentinel_t, const I&) noexcept
        {
            return true;
        }
        template<class I, class = enable_if_t<weakly_incrementable<I>>>
        friend bool operator==(const I&, unreachable_sentinel_t) noexcept
        {
            return false;
        }
        template<class I, class = enable_if_t<weakly_incrementable<I>>>
        friend bool operator!=(const I&, unreachable_sentinel_t) noexcept
        {
            return true;
        }
    };

    inline constexpr unreachable_sentinel_t unreachable_sentinel{};
}
