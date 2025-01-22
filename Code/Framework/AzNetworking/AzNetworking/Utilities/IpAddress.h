/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <stdint.h>
#include <AzCore/std/hash.h>
#include <AzCore/std/string/fixed_string.h>

namespace AzNetworking
{
    class ISerializer;

    enum class ByteOrder
    {
        Host
    ,   Network
    };

    enum class ProtocolType
    {
        Tcp
    ,   Udp
    };

    //! @class IpAddress
    //! @brief Wrapper for dealing with internet Ip addresses.
    class IpAddress
    {
    public:

        static constexpr uint32_t MaxIpStringLength = 32;
        static constexpr uint32_t InvalidAddress = 0;
        static constexpr uint32_t InvalidPort = 0;
        using IpString = AZStd::fixed_string<MaxIpStringLength>;

        IpAddress() = default;

        //! Construct from a string hostname.
        //! @param hostname hostname to convert to an IpAddress "amazon.com" or "127.0.0.1"
        //! @param service  service or port number "80" or "http"
        IpAddress(const char* hostname, const char* service, ProtocolType type);

        //! Construct from a string hostname.
        //! @param hostname hostname to convert to an IpAddress "amazon.com" or "127.0.0.1"
        //! @param port     the port number
        IpAddress(const char* hostname, uint16_t port, ProtocolType type);

        //! Construct from a set of Ipv4 quads.
        //! @param quadA first quad of the address
        //! @param quadB second quad of the address
        //! @param quadC third quad of the address
        //! @param quadD forth quad of the address
        //! @param port  port number given in host byte order
        IpAddress(uint8_t quadA, uint8_t quadB, uint8_t quadC, uint8_t quadD, uint16_t port);

        //! Construct from an IPv4Address and port number given in the provided byte order.
        //! @param byteOrder the byte order of the provided parameters
        //! @param address   IPv4Address given in host byte order
        //! @param port      port number given in host byte order
        IpAddress(ByteOrder byteOrder, uint32_t address, uint16_t port);

        virtual ~IpAddress() = default;

        //! Returns the address in requested byte order
        //! @return internal IPv4Address in requested byte order
        uint32_t GetAddress(ByteOrder byteOrder) const;

        //! Returns the port number in requested byte order
        //! @return internal port number in requested byte order
        uint16_t GetPort(ByteOrder byteOrder) const;

        //! Return the first dotted quad of the internal Ipv4Address
        //! @return first dotted quad of the internal Ipv4Address
        uint8_t GetQuadA() const;

        //! Return the second dotted quad of the internal Ipv4Address
        //! @return second dotted quad of the internal Ipv4Address
        uint8_t GetQuadB() const;

        //! Return the third dotted quad of the internal Ipv4Address
        //! @return third dotted quad of the internal Ipv4Address
        uint8_t GetQuadC() const;

        //! Return the forth dotted quad of the internal Ipv4Address
        //! @return forth dotted quad of the internal Ipv4Address
        uint8_t GetQuadD() const;

        //! Returns the address in a human readable string form.
        //! @return the address in a human readable string form
        IpString GetString() const;

        //! Returns just the ip address with no port number in a human readable string form.
        //! @return just the ip address with no port number in a human readable string form
        IpString GetIpString() const;

        //! Returns whether or not this IP address is valid.
        //! @return true if the IPv4 address and port are valid; otherwise false.
        bool IsValid() const;

        //! Equality operator.
        //! @param rhs base type value to compare against
        //! @return boolean true if this == rhs
        bool operator ==(const IpAddress& rhs) const;

        //! Inequality operator
        //! @param rhs base type value to compare against
        //! @return boolean true if this != rhs
        bool operator !=(const IpAddress& rhs) const;

        //! Strictly less than operator.
        //! @param rhs base type value to compare against
        //! @return boolean true if this < rhs
        bool operator < (const IpAddress& rhs) const;

        //! Less than equal to operator.
        //! @param rhs base type value to compare against
        //! @return boolean true if this <= rhs
        bool operator <= (const IpAddress& rhs) const;

        //! Strictly greater than operator.
        //! @param rhs base type value to compare against
        //! @return boolean true if this > rhs
        bool operator > (const IpAddress& rhs) const;

        //! Greater than equal to operator.
        //! @param rhs base type value to compare against
        //! @return boolean true if this >= rhs
        bool operator >= (const IpAddress& rhs) const;

        //! Serializes the ipAddress using the provided serializer instance.
        //! @param serializer ISerializer instance to use for serialization
        //! @return boolean true for success, false for serialization failure
        bool Serialize(ISerializer& serializer);

    private:

        uint32_t m_ipv4Address = 0;
        uint16_t m_port = 0;
    };
}

#include <AzNetworking/Utilities/IpAddress.inl>
