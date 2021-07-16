/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GridMate_Traits_Platform.h>
#include <GridMate/Carrier/Utils.h>
#include <GridMate/Carrier/Driver.h>

#include <AzCore/PlatformIncl.h>
#include <WinSock2.h>
#include <Ws2tcpip.h>

namespace GridMate
{
    const char* GridMate::Utils::GetBroadcastAddress(int familyType)
    {
        if (familyType == Driver::BSD_AF_INET6)
        {
            return "FF02::1";
        }
        else if (familyType == Driver::BSD_AF_INET)
        {
            return "255.255.255.255";
        }
        else
        {
            return "";
        }
    }
}
