/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>

#include <limits>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace LocalUser
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Constant representing the max local player slot
    static const AZ::u32 LocalPlayerSlotMax = 4;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Constant representing any local player slot
    static const AZ::u32 LocalPlayerSlotAny = std::numeric_limits<AZ::u32>::max();

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Constant representing no local player slot
    static const AZ::u32 LocalPlayerSlotNone = std::numeric_limits<AZ::u32>::max() - 1;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Constant representing the primary local player slot
    static const AZ::u32 LocalPlayerSlotPrimary = 0;
} // namespace LocalUser
