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
