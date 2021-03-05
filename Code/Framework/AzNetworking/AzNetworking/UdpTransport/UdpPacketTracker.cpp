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
