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
    inline void UdpConnection::SetConnectionQuality(const ConnectionQuality& connectionQuality)
    {
        m_connectionQuality = connectionQuality;
    }

    inline const ConnectionQuality& UdpConnection::GetConnectionQuality() const
    {
        return m_connectionQuality;
    }

    inline DtlsEndpoint& UdpConnection::GetDtlsEndpoint()
    {
        return m_dtlsEndpoint;
    }

    inline const UdpPacketTracker& UdpConnection::GetPacketTracker() const
    {
        return m_packetTracker;
    }

    inline UdpPacketTracker& UdpConnection::GetPacketTracker()
    {
        return m_packetTracker;
    }

    inline uint32_t UdpConnection::GetReliableQueueSize() const
    {
        return m_reliableQueue.GetQueueSize();
    }

    inline void UdpConnection::SetTimeoutId(TimeoutId timeoutId)
    {
        m_timeoutId = timeoutId;
    }

    inline TimeoutId UdpConnection::GetTimeoutId() const
    {
        return m_timeoutId;
    }

    inline bool UdpConnection::PrepareReliablePacketForSend(PacketId packetId, SequenceId reliableSequenceId, const IPacket& packet)
    {
        return m_reliableQueue.PrepareForSend(packetId, reliableSequenceId, packet);
    }
}
