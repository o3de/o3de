/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/UdpTransport/UdpPacketTracker.h>

namespace AzNetworking
{
    UdpPacketTracker::UdpPacketTracker()
        : m_nextPacketId(InvalidPacketId)
    {
        ;
    }

    UdpPacketTracker::~UdpPacketTracker()
    {
        ;
    }

    void UdpPacketTracker::Reset()
    {
        m_nextPacketId = InvalidPacketId;
        m_receivedWindow.Reset();
        m_acknowledgedWindow.Reset();
    }

    bool UdpPacketTracker::ProcessReceived(UdpConnection* connection, UdpPacketHeader& header)
    {
        if (!m_receivedWindow.UpdateForReceivedPacket(header))
        {
            return false;
        }
        m_acknowledgedWindow.UpdateForRemoteAckStatus(connection, header);
        return true;
    }

    PacketAckState UdpPacketTracker::GetPacketAckStatus(PacketId packetId) const
    {
        return m_acknowledgedWindow.GetPacketAckStatus(packetId);
    }
}
