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

namespace AzNetworking
{
    namespace Platform
    {
        void InitIpAddress(uint32_t& ipv4Address, uint16_t& port,
            const char* hostname, const char* service, ProtocolType type);
    }

    IpAddress::IpAddress(const char* hostname, const char* service, ProtocolType type)
    {
        Platform::InitIpAddress(m_ipv4Address, m_port, hostname, service, type);
    }

    IpAddress::IpAddress(const char* hostname, uint16_t port, ProtocolType type)
        : IpAddress(hostname, nullptr, type)
    {
        m_port = port;
    }

    IpAddress::IpAddress(uint8_t quadA, uint8_t quadB, uint8_t quadC, uint8_t quadD, uint16_t port)
        : m_ipv4Address((quadA << 24) | (quadB << 16) | (quadC << 8) | quadD)
        , m_port(port)
    {
        ;
    }

    IpAddress::IpAddress(ByteOrder byteOrder, uint32_t address, uint16_t port)
        : m_ipv4Address((byteOrder == ByteOrder::Network) ? ntohl(address) : address)
        , m_port((byteOrder == ByteOrder::Network) ? ntohs(port) : port)
    {
        ;
    }

    uint32_t IpAddress::GetAddress(ByteOrder byteOrder) const
    {
        return (byteOrder == ByteOrder::Network) ? htonl(m_ipv4Address) : m_ipv4Address;
    }

    uint16_t IpAddress::GetPort(ByteOrder byteOrder) const
    {
        return (byteOrder == ByteOrder::Network) ? htons(m_port) : m_port;
    }

    IpAddress::IpString IpAddress::GetString() const
    {
        return IpString::format("%hhu.%hhu.%hhu.%hhu:%hu", GetQuadA(), GetQuadB(), GetQuadC(), GetQuadD(), GetPort(ByteOrder::Host));
    }

    IpAddress::IpString IpAddress::GetIpString() const
    {
        return IpString::format("%hhu.%hhu.%hhu.%hhu", GetQuadA(), GetQuadB(), GetQuadC(), GetQuadD());
    }

    bool IpAddress::IsValid() const
    {
        return m_ipv4Address != InvalidAddress && m_port != InvalidPort;
    }

    bool IpAddress::Serialize(ISerializer& serializer)
    {
        serializer.Serialize(m_ipv4Address, "Ipv4Address");
        serializer.Serialize(m_port, "Port");
        return serializer.IsValid();
    }
}
