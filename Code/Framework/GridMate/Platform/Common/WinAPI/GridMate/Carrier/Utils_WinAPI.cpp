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
    string Utils::GetMachineAddress(int familyType)
    {
        string machineName;
        char name[MAX_PATH];
        int result = gethostname(name, sizeof(name));
        AZ_Error("GridMate", result == 0, "Failed in gethostname with result=%d, WSAGetLastError=%d!", result, WSAGetLastError());
        if (result)
        {
            return "";
        }
        addrinfo  hints = { 0 };
        addrinfo* addrInfo;
        hints.ai_family = (familyType == Driver::BSD_AF_INET6) ? AF_INET6 : AF_INET;
        hints.ai_flags = AI_CANONNAME;
        result = getaddrinfo(name, nullptr, &hints, &addrInfo);
        AZ_Error("GridMate", result == 0, "Failed in getaddrinfo with %d!", WSAGetLastError());
        if (addrInfo->ai_family == AF_INET6)
        {
            inet_ntop(addrInfo->ai_family, &reinterpret_cast<sockaddr_in6*>(addrInfo->ai_addr)->sin6_addr, name, AZ_ARRAY_SIZE(name));
        }
        else if (addrInfo->ai_family == AF_INET)
        {
            inet_ntop(addrInfo->ai_family, &reinterpret_cast<sockaddr_in*>(addrInfo->ai_addr)->sin_addr, name, AZ_ARRAY_SIZE(name));
        }
        else
        {
            AZ_Assert(false, "Invalid address family");
        }

        freeaddrinfo(addrInfo);
        machineName = name;
        return machineName;
    }
}

