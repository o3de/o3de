/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
