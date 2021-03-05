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

#pragma once

namespace AzNetworking
{
    inline TcpPacketHeader::TcpPacketHeader(PacketType packetType, uint16_t packetSize)
        : m_packetType(packetType)
        , m_packetSize(packetSize)
    {
        ;
    }

    inline PacketType TcpPacketHeader::GetPacketType() const
    {
        return m_packetType;
    }

    inline PacketId TcpPacketHeader::GetPacketId() const
    {
        return InvalidPacketId;
    }

    inline uint16_t TcpPacketHeader::GetPacketSize() const
    {
        return m_packetSize;
    }

    inline bool TcpPacketHeader::IsPacketFlagSet(PacketFlag flag) const
    {
        return m_packetFlags.GetBit(aznumeric_cast<uint32_t>(flag));
    }

    inline void TcpPacketHeader::SetPacketFlag(PacketFlag flag, bool value)
    {
        m_packetFlags.SetBit(aznumeric_cast<uint32_t>(flag), value);
    }
}
