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
    using std::aligned_storage;
    using std::aligned_storage_t;

    template <typename T, size_t Alignment = alignof(T)>
    using aligned_storage_for_t = std::aligned_storage_t<sizeof(T), Alignment>;
}
