/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/UdpTransport/UdpPacketIdWindow.h>
#include <AzNetworking/UdpTransport/UdpPacketHeader.h>
#include <AzNetworking/UdpTransport/UdpConnection.h>
#include <AzNetworking/DataStructures/FixedSizeBitset.h>
#include <AzCore/Console/ILogger.h>

namespace AzNetworking
{
    const char* GetEnumString(PacketAckState value)
    {
        switch (value)
        {
        case PacketAckState::Acked:
            return "PacketAckState::Acked";
        case PacketAckState::Nacked:
            return "PacketAckState::Nacked";
        case PacketAckState::Unknown_TooNew:
            return "PacketAckState::Unknown_TooNew";
        case PacketAckState::Unknown_TooOld:
            return "PacketAckState::Unknown_TooOld";
        }
        return "INVALID";
    }

    UdpPacketIdWindow::UdpPacketIdWindow()
        : m_headSequenceId(SequenceId{ 0 })
        , m_headPacketId(InvalidPacketId)
        , m_sequenceRolloverCount(SequenceRolloverCount{ 0 })
    {
        ;
    }

    void UdpPacketIdWindow::Reset()
    {
        m_headSequenceId = SequenceId{ 0 };
        m_headPacketId = InvalidPacketId;
        m_sequenceRolloverCount = SequenceRolloverCount{ 0 };
        m_ackWindow.Reset();
    }

    PacketAckState UdpPacketIdWindow::GetPacketAckStatus(PacketId packetId) const
    {
        // If we haven't heard from the remote endpoint yet, treat packets as having fallen outside our window
        if (m_headPacketId == InvalidPacketId)
        {
            return PacketAckState::Nacked;
        }

        // Check if the requested sequence id is newer than any acked packet received
        if (packetId > m_headPacketId)
        {
            AZLOG(NET_Acks, "Requested ack status for %u, head %u, too new", aznumeric_cast<uint32_t>(packetId), aznumeric_cast<uint32_t>(m_headPacketId));
            return PacketAckState::Unknown_TooNew;
        }

        const PacketId packetDelta = m_headPacketId - packetId;

        // Sequence is so old, it's now out of bounds of our packet ack tracker
        if (aznumeric_cast<uint32_t>(packetDelta) >= m_ackWindow.GetValidBitCount())
        {
            AZLOG(NET_Acks, "Requested ack status for %u, head %u, too old", aznumeric_cast<uint32_t>(packetId), aznumeric_cast<uint32_t>(m_headPacketId));
            return PacketAckState::Unknown_TooOld;
        }

        const bool acked = m_ackWindow.GetBit(aznumeric_cast<uint32_t>(packetDelta));
        return acked ? PacketAckState::Acked : PacketAckState::Nacked;
    }

    BitsetChunk& UdpPacketIdWindow::GetMostRecentAckState(BitsetChunk& outWindow) const
    {
        const uint64_t firstChunk = aznumeric_cast<uint64_t>(m_ackWindow.GetBitsetElement(0));
        const uint64_t secondChunk = aznumeric_cast<uint64_t>(m_ackWindow.GetBitsetElement(1));
        const uint32_t unusedHeadBits = m_ackWindow.GetUnusedHeadBits();
        const uint32_t usedHeadBits = m_ackWindow.NumBitsetChunkedBits - unusedHeadBits;
        outWindow = aznumeric_cast<BitsetChunk>((secondChunk << usedHeadBits) | firstChunk);
        return outWindow;
    }

    bool UdpPacketIdWindow::UpdateForReceivedPacket(UdpPacketHeader& header)
    {
        const SequenceId receivedSequenceId = header.GetLocalSequenceId();

        if (SequenceMoreRecent(receivedSequenceId, m_headSequenceId))
        {
            const SequenceId sequenceDelta = SequenceId(receivedSequenceId - m_headSequenceId);

            m_ackWindow.PushBackBits(aznumeric_cast<uint32_t>(sequenceDelta));
            m_ackWindow.SetBit(0, true);

            // We've already determined that receivedSequenceId is 'newer' than m_headSequenceId
            // So if receivedSequenceId is numerically less than m_headSequenceId, we have rolled over
            if (receivedSequenceId < m_headSequenceId)
            {
                ++m_sequenceRolloverCount;
            }

            // This is the newest packet, it's rollover count is always the most recent rollover count
            header.SetLocalRolloverCount(m_sequenceRolloverCount);
            m_headSequenceId = receivedSequenceId;
            m_headPacketId = MakePacketId(m_sequenceRolloverCount, m_headSequenceId);
        }
        else
        {
            // This is an out of order packet, we've previously received a newer sequence value
            const SequenceId sequenceDelta = m_headSequenceId - receivedSequenceId;

            if (aznumeric_cast<uint32_t>(sequenceDelta) >= m_ackWindow.GetValidBitCount())
            {
                // Too old to process
                AZLOG(NET_DebugUdp, "Discarding old packet, sequence is too old to process");
                return false;
            }

            if (m_ackWindow.GetBit(aznumeric_cast<uint32_t>(sequenceDelta)))
            {
                // Received packet is a duplicate of one already processed
                AZLOG(NET_DebugUdp, "Discarding packet due to duplicated sequence id");
                return false;
            }

            m_ackWindow.SetBit(aznumeric_cast<uint32_t>(sequenceDelta), true);

            // Received sequence is 'older' than head sequence
            if (receivedSequenceId < m_headSequenceId)
            {
                // If the 'older' received sequence is numerically less than head sequence, the rollover count is unchanged
                header.SetLocalRolloverCount(m_sequenceRolloverCount);
            }
            else
            {
                // If the 'older' received sequence is numerically greater than than head sequence, we've received a packet bound to the previous rollover count
                header.SetLocalRolloverCount(m_sequenceRolloverCount - SequenceRolloverCount{ 1 });
            }
        }

        return true;
    }

    void UdpPacketIdWindow::UpdateForRemoteAckStatus(UdpConnection* connection, UdpPacketHeader& header)
    {
        const SequenceId  receivedSequenceId = header.GetRemoteSequenceId();
        const BitsetChunk sequenceWindow = header.GetSequenceWindow();

        // Reconstruct the ack state of the remote connection based on remote sequence number and ack bits
        // Align the received and cached ack bit vectors
        if (SequenceMoreRecent(receivedSequenceId, m_headSequenceId))
        {
            // We've already determined that receivedSequenceId is 'newer' than m_headSequenceId
            // So if receivedSequenceId is numerically less than m_headSequenceId, we have rolled over
            if (receivedSequenceId < m_headSequenceId)
            {
                ++m_sequenceRolloverCount;
            }

            AZLOG
            (
                NET_DebugUdp,
                "Updating ack data, old head sequence %u, new head sequence %u, ack vector %X", 
                aznumeric_cast<uint32_t>(m_headSequenceId), 
                aznumeric_cast<uint32_t>(receivedSequenceId), 
                sequenceWindow
            );

            const AZ::TimeMs currentTimeMs = AZ::GetElapsedTimeMs();
            const SequenceId sequenceIdDelta = SequenceId(receivedSequenceId - m_headSequenceId);
            const PacketId   receivedPacketId = MakePacketId(m_sequenceRolloverCount, receivedSequenceId);

            m_headSequenceId = receivedSequenceId;
            m_headPacketId = receivedPacketId;
            m_ackWindow.PushBackBits(aznumeric_cast<uint32_t>(sequenceIdDelta));

            for (uint32_t bit = 0; bit < m_ackWindow.NumBitsetChunkedBits; ++bit)
            {
                if (!GetBitHelper(sequenceWindow, bit))
                {
                    continue;
                }

                if (!m_ackWindow.GetBit(bit))
                {
                    m_ackWindow.SetBit(bit, true);
                    AZLOG(NET_DebugUdp, "Acking packet ID %u", aznumeric_cast<uint32_t>(receivedPacketId) - bit);
                    if (connection != nullptr)
                    {
                        connection->ProcessAcked(receivedPacketId - aznumeric_cast<PacketId>(bit), currentTimeMs);
                    }
                }
            }
        }
    }

    void UdpPacketIdWindow::PrintStatus() const
    {
        AZLOG_INFO
        (
            "%u - %08X:%08X:%08X:%08X", 
            aznumeric_cast<uint32_t>(m_headSequenceId), 
            aznumeric_cast<uint32_t>(m_ackWindow.GetBitsetElement(0)), 
            aznumeric_cast<uint32_t>(m_ackWindow.GetBitsetElement(1)), 
            aznumeric_cast<uint32_t>(m_ackWindow.GetBitsetElement(2)), 
            aznumeric_cast<uint32_t>(m_ackWindow.GetBitsetElement(3))
        );
    }
}
