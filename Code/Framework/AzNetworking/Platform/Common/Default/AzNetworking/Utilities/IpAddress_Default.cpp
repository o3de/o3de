/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
