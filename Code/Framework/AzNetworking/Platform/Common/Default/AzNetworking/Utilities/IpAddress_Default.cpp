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

#include <AzNetworking/Utilities/IpAddress.h>

#include <AzNetworking/AzNetworking_Traits_Platform.h>
#include <AzNetworking/Serialization/ISerializer.h>
#include <AzNetworking/Utilities/Endian.h>
#include <AzNetworking/Utilities/NetworkIncludes.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Console/ILogger.h>
#include <stdlib.h>

namespace AzNetworking
{
    namespace Platform
    {
        void InitIpAddress(uint32_t& ipv4Address, uint16_t& port,
            const char* hostname, const char* service, ProtocolType type)
        {
            struct addrinfo* res = nullptr;
            {
                struct addrinfo hints;
                memset(&hints, 0, sizeof hints);
                hints.ai_family = AF_INET;
                hints.ai_socktype = (type == ProtocolType::Tcp) ? SOCK_STREAM : SOCK_DGRAM;
                getaddrinfo(hostname, service, &hints, &res);
            }

            if (res == nullptr)
            {
                AZLOG_WARN("Hostname lookup for %s:%s returned a NULL result", hostname, service);
                return;
            }

            if (res->ai_family == AF_INET)
            {
                // For an Ipv4 address, the return address format is:
                // 2 most significant bytes are port number in network byte order
                // Next 4 bytes are IPv4 address in network byte order
                const uint8_t* addr = (const uint8_t*)res->ai_addr->sa_data;
                uint16_t& osPort = (uint16_t&)(addr[0]);
                uint32_t& osAddr = (uint32_t&)(addr[2]);
                ipv4Address = ntohl(osAddr);
                port = ntohs(osPort);
            }

            freeaddrinfo(res);
        }
    }
}
