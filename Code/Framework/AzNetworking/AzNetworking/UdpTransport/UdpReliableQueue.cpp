/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzNetworking/UdpTransport/UdpReliableQueue.h>
#include <AzNetworking/UdpTransport/UdpConnection.h>
#include <AzNetworking/UdpTransport/UdpNetworkInterface.h>
#include <AzNetworking/UdpTransport/UdpPacketHeader.h>
#include <AzNetworking/Serialization/NetworkOutputSerializer.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ILogger.h>

namespace AzNetworking
{
    AZ_CVAR(uint32_t, net_MaxReliablePacketsInWindow, 16384, nullptr, AZ::ConsoleFunctorFlags::Null, "The maximum number of reliable packets to allow to be queued up before triggering a disconnect");

    UdpReliableQueue::~UdpReliableQueue()
    {
        ;
    }

    SequenceId UdpReliableQueue::GetNextSequenceId()
    {
        return m_reliableSequenceGenerator.GetNextSequenceId();
    }

    uint32_t UdpReliableQueue::GetQueueSize() const
    {
        return static_cast<uint32_t>(m_packetWindow.size());
    }

    bool UdpReliableQueue::PrepareForSend(PacketId packetId, SequenceId reliableSequenceId, const IPacket& packet)
    {
        AZLOG(NET_ReliableQueueDebug, "Inserting packetId %u with reliable sequenceId %u", static_cast<uint32_t>(packetId), static_cast<uint32_t>(reliableSequenceId));
        if (m_packetWindow.size() > net_MaxReliablePacketsInWindow)
        {
            return false;
        }

        PendingPacketMap::const_iterator iter = m_packetWindow.find(packetId);
        if (iter != m_packetWindow.end())
        {
            AZ_Assert(false, "Attempted to reinsert an existing packetId into the reliable queue");
            return false;
        }
        m_packetWindow[packetId] = { reliableSequenceId, packet.Clone() };
        return true;
    }

    bool UdpReliableQueue::OnPacketReceived(const UdpPacketHeader& header)
    {
        const SequenceId receivedSequence = header.GetReliableSequenceId();

        if (SequenceMoreRecent(receivedSequence, m_lastReceivedReliableSequenceId))
        {
            const SequenceId sequenceDelta = SequenceId(receivedSequence - m_lastReceivedReliableSequenceId);
            m_lastReceivedReliableSequenceId = receivedSequence;
            m_receivedSequenceHistory.PushBackBits(static_cast<uint32_t>(sequenceDelta));
            m_receivedSequenceHistory.SetBit(0, true);
        }
        else
        {
            const SequenceId sequenceDelta = SequenceId(m_lastReceivedReliableSequenceId - receivedSequence);

            if (static_cast<uint32_t>(sequenceDelta) >= m_receivedSequenceHistory.GetValidBitCount())
            {
                // Too old to process
                // @TODO: disconnect? out of range sequence is potentially an unrecoverable error
                AZLOG(NET_ReliableQueue, "Reliable sequenceId is outside our tracked reliable packet window");
                return false;
            }

            if (m_receivedSequenceHistory.GetBit(static_cast<uint32_t>(sequenceDelta)))
            {
                // Received packet is a duplicate of one already processed
                AZLOG(NET_ReliableQueue, "Received duplicate of reliable packetId %u, discarding", static_cast<uint32_t>(receivedSequence));
                return false;
            }

            m_receivedSequenceHistory.SetBit(static_cast<uint32_t>(sequenceDelta), true);
        }

        return true;
    }

    void UdpReliableQueue::OnPacketAcked([[maybe_unused]] UdpNetworkInterface& networkInterface,
        [[maybe_unused]] UdpConnection& connection, PacketId packetId)
    {
        AZLOG(NET_ReliableQueueDebug, "Acked packetId %u", static_cast<uint32_t>(packetId));
        PendingPacketMap::const_iterator iter = m_packetWindow.find(packetId);
        if (iter != m_packetWindow.end())
        {
            m_packetWindow.erase(iter);
        }
    }

    bool UdpReliableQueue::OnPacketLost(UdpNetworkInterface& networkInterface, UdpConnection& connection, PacketId packetId)
    {
        AZLOG(NET_ReliableQueueDebug, "Lost packetId %u", static_cast<uint32_t>(packetId));

        bool result = false;
        AZStd::unique_ptr<IPacket> lostPacket;
        SequenceId lostReliableSequenceId = InvalidSequenceId;

        PendingPacketMap::iterator iter = m_packetWindow.find(packetId);
        if (iter != m_packetWindow.end())
        {
            AZ_Assert(iter->second.m_packet != nullptr, "Timed out reliable packet was nullptr");
            lostPacket = AZStd::move(iter->second.m_packet); // This transfers ownership out of the pending packet to this local scoped alloc
            lostReliableSequenceId = iter->second.m_reliableSequenceId;
            m_packetWindow.erase(iter);
        }
        else
        {
            AZLOG_ERROR("Failed to find timed out packetId %u in reliable queue", static_cast<uint32_t>(packetId));
        }

        if (lostReliableSequenceId != InvalidSequenceId)
        {
            AZLOG(NET_ReliableQueue, "Resending reliable packetId %u due to loss", static_cast<uint32_t>(lostReliableSequenceId));

            // This punches down an abstraction layer purposefully to resend using the existing reliable SequenceId
            // NOTE: This will call back into UdpReliableQueue::PrepareForSend!!
            if (networkInterface.SendPacket(connection, *lostPacket, lostReliableSequenceId) == InvalidPacketId)
            {
                // Packet failed to retransmit, meaning no retry attempt was made
                // Since we've lost a reliable packet, the appropriate response is to terminate the connection
                connection.Disconnect(DisconnectReason::ReliableTransportFailure, TerminationEndpoint::Local);
                result = true;
            }

            networkInterface.GetMetrics().m_resentPackets++;
        }

        return result;
    }
}
