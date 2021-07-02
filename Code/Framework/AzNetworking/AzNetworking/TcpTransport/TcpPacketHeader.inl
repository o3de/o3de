/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
