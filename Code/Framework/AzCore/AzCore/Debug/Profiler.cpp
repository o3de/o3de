/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/Profiler.h>

#include <AzCore/Math/Crc.h>

namespace AZ::Debug
{
    uint32_t ProfileScope::GetSystemID(const char* system)
    {
        // TODO: stable ids for registered budgets
        return AZ::Crc32(system);
    }
} // namespace AZ::Debug
