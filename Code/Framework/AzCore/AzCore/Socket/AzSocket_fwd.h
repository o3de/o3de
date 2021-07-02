/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>

#define SOCKET_ERROR (-1)
#define AZ_SOCKET_INVALID (-1)

#include <AzCore/Socket/AzSocket_fwd_Platform.h>

// Type wrappers for sockets
typedef sockaddr    AZSOCKADDR;
typedef sockaddr_in AZSOCKADDR_IN;
typedef AZ::s32     AZSOCKET;

namespace AZ
{
    namespace AzSock
    {
        enum class AzSockError : AZ::s32;
        enum class AZSocketOption : AZ::s32;
        class AzSocketAddress;
    }
}
