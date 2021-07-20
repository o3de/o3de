/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    class ReflectContext;
} // namespace Az

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Alias for the type of a local user id
    using LocalUserId = AZ::u32;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Reflection (AZ::u32 is already reflected)
    inline void LocalUserIdReflect(AZ::ReflectContext* context) { AZ_UNUSED(context); }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Convert to a string (use for debugging purposes only)
    inline AZStd::string LocalUserIdToString(const LocalUserId& localUserId) { return AZStd::string::format("%u", localUserId); }
} // namespace AzFramework
