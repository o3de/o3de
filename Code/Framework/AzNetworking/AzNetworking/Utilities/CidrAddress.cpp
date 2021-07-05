/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/Utilities/CidrAddress.h>
#include <AzNetworking/Utilities/IpAddress.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Console/ILogger.h>

namespace AzNetworking
{
    CidrAddress::CidrAddress(const AZStd::string& cidrAddress)
    {
        [[maybe_unused]] const bool success = ParseAddress(cidrAddress);
        AZ_Assert(success, "Failed to construct CidrAddress from malformed input string");
    }

    bool CidrAddress::ParseAddress(const AZStd::string& cidrAddress)
    {
        AZStd::string ip(cidrAddress);

        // Extract the CIDR mask from the string.  If there is no mask defined then this is an explicit IP (even though it should end in /32)
        const AZStd::string::size_type maskLocation = cidrAddress.rfind("/");
        if (maskLocation != AZStd::string::npos)
        {
            // Determine the mask
            const int32_t mask = AZStd::min(atoi(cidrAddress.substr(maskLocation + 1).c_str()), 32);
            m_mask = (mask > 0) ? ~((1 << (32 - mask)) - 1) : 0;

            // remove the mask from the string so it's only the ip
            ip = cidrAddress.substr(0, maskLocation);
        }

        // Convert ip to a AzNetworking uint32
        AZStd::fixed_vector<uint8_t, 4> ipv4Quads;
        auto ParseIpv4 = [&ipv4Quads](AZStd::string_view token)
        {
            AZStd::fixed_string<32> ipv4Segment(token);
            char* segmentEnd;
            uint8_t octet = aznumeric_cast<uint8_t>(strtoul(ipv4Segment.c_str(), &segmentEnd, 10));
            ipv4Quads.push_back(octet);
        };

        AZ::StringFunc::TokenizeVisitor(ip, ParseIpv4, '.');

        if (ipv4Quads.size() == 4)
        {
            m_ip = AzNetworking::IpAddress(ipv4Quads[0], ipv4Quads[1], ipv4Quads[2], ipv4Quads[3], 0).GetAddress(ByteOrder::Host) & m_mask;
            return true;
        }

        AZLOG_ERROR("CIDR input string (%s) malformed, input should be formatted as aaa.bbb.ccc.ddd/mask", cidrAddress.c_str());
        return false;
    }

    bool CidrAddress::IsMatch(const IpAddress& address) const
    {
        const uint32_t uintAddress = address.GetAddress(ByteOrder::Host);
        return IsMatch(uintAddress);
    }

    bool CidrAddress::IsMatch(uint32_t address) const
    {
        // Apply the mask to query address and see if it matches the internal address
        const uint32_t maskedAddress = (address & m_mask);
        return maskedAddress == m_ip;
    }

    uint32_t CidrAddress::GetIp() const
    {
        return m_ip;
    }

    uint32_t CidrAddress::GetMask() const
    {
        return m_mask;
    }
}
