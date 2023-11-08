/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/limits.h>
#include <stddef.h>

namespace AZStd
{
    inline constexpr size_t dynamic_extent = numeric_limits<size_t>::max();

    template <class T, size_t Extent = dynamic_extent>
    class span;
}
