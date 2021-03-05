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

#include <AzCore/Socket/AzSocket.h>

namespace AZ
{
    namespace AzSock
    {
        AzSocketAddress::AzSocketAddress()
        {
            Reset();
        }

        AzSocketAddress& AzSocketAddress::operator=(const AZSOCKADDR& addr)
        {
            m_sockAddr = *reinterpret_cast<const AZSOCKADDR_IN*>(&addr);
            return *this;
        }

        bool AzSocketAddress::operator==(const AzSocketAddress& rhs) const
        {
            return m_sockAddr.sin_family == rhs.m_sockAddr.sin_family
                && m_sockAddr.sin_addr.s_addr == rhs.m_sockAddr.sin_addr.s_addr
                && m_sockAddr.sin_port == rhs.m_sockAddr.sin_port;
        }

        const AZSOCKADDR* AzSocketAddress::GetTargetAddress() const
        {
            return reinterpret_cast<const AZSOCKADDR*>(&m_sockAddr);
        }

        AZStd::string AzSocketAddress::GetIP() const
        {
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, const_cast<in_addr*>(&m_sockAddr.sin_addr), ip, AZ_ARRAY_SIZE(ip));
            return AZStd::string(ip);
        }

        AZStd::string AzSocketAddress::GetAddress() const
        {
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, const_cast<in_addr*>(&m_sockAddr.sin_addr), ip, AZ_ARRAY_SIZE(ip));
            return AZStd::string::format("%s:%d", ip, AZ::AzSock::NetToHostShort(m_sockAddr.sin_port));
        }

        AZ::u16 AzSocketAddress::GetAddrPort() const
        {
            return AZ::AzSock::NetToHostShort(m_sockAddr.sin_port);
        }

        void AzSocketAddress::SetAddrPort(AZ::u16 port)
        {
            m_sockAddr.sin_port = AZ::AzSock::HostToNetShort(port);
        }

        bool AzSocketAddress::SetAddress(const AZStd::string& ip, AZ::u16 port)
        {
            AZ_Assert(!ip.empty(), "Invalid address string!");
            Reset();
            return AZ::AzSock::ResolveAddress(ip, port, m_sockAddr);
        }

        bool AzSocketAddress::SetAddress(AZ::u32 ip, AZ::u16 port)
        {
            Reset();
            m_sockAddr.sin_addr.s_addr = AZ::AzSock::HostToNetLong(ip);
            m_sockAddr.sin_port = AZ::AzSock::HostToNetShort(port);
            return true;
        }

        void AzSocketAddress::Reset()
        {
            memset(&m_sockAddr, 0, sizeof(m_sockAddr));
            m_sockAddr.sin_family = AF_INET;
            m_sockAddr.sin_addr.s_addr = INADDR_ANY;
        }
    }
}
