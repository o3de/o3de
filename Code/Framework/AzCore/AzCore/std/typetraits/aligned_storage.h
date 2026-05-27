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
    template<size_t Len, size_t Align = alignof(max_align_t)>
    struct aligned_storage
    {
        struct type
        {
            alignas(Align) byte data[Len];
        };
    };

    template<size_t Len, size_t Align = alignof(max_align_t)>
    using aligned_storage_t = typename aligned_storage<Len, Align>::type;

    template<typename T>
    using aligned_storage_for_t = aligned_storage_t<sizeof(T), alignof(T)>;
} // namespace AZStd
