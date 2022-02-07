/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AzNetworking
{
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
