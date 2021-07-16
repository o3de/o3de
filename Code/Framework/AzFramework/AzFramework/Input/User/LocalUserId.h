/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Input/User/LocalUserId_Platform.h>

#include <limits>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Constant representing any local user id
    static const LocalUserId LocalUserIdAny(std::numeric_limits<AZ::u32>::max());

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Constant representing no local user id
    static const LocalUserId LocalUserIdNone(std::numeric_limits<AZ::u32>::max() - 1);
} // namespace AzFramework
