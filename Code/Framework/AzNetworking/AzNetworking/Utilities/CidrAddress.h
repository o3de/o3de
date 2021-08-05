/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>

namespace AzNetworking
{
    class IpAddress;

    //! @class CidrAddress
    //! @brief Helper class that implements Classless Inter-Domain Routing (CIDR) IP address filtering.
    class CidrAddress
    {
    public:

        CidrAddress() = default;

        //! Construct from an input string.
        //! @param cidrAddress the input string to parse as a CIDR address (dotted quad/mask, ie 127.0.0.1/32)
        CidrAddress(const AZStd::string& cidrAddress);

        //! Parse the address from the provided string.
        //! @param cidrAddress the input string to parse as a CIDR address (dotted quad/mask, ie 127.0.0.1/32)
        //! @return boolean true if the input was successfully parsed, false on error
        bool ParseAddress(const AZStd::string& cidrAddress);

        //! Check to see if a AzNetworking uint IP matches this CIDR address instance.
        //! @param address input IpAddress to validate
        //! @return boolean true if the address matches, false if it fails
        bool IsMatch(const IpAddress& address) const;

        //! Check to see if a AzNetworking uint IP matches this CIDR address instance.
        //! @param address input IPv4 address as a uint32_t ** must be in host byte order **
        //! @return boolean true if the address matches, false if it fails
        bool IsMatch(uint32_t address) const;

        //! Returns the IP of this CIDR address in host byte order.
        //! @return the IP of this CIDR address in host byte order
        uint32_t GetIp() const;

        //! Returns the mask of this CIDR address.
        //! @return the mask of this CIDR address
        uint32_t GetMask() const;

    protected:

        uint32_t m_ip = 0;
        uint32_t m_mask = 0xFFFFFFFF; //< Default is /32, which is an explicit address
    };
}
