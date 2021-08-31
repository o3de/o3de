/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/UdpTransport/UdpFragmentQueue.h>
#include <AzNetworking/UdpTransport/UdpConnection.h>
#include <AzNetworking/UdpTransport/UdpPacketHeader.h>
#include <AzNetworking/Serialization/NetworkOutputSerializer.h>
#include <AzNetworking/Utilities/NetworkCommon.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ILogger.h>

namespace AzNetworking
{
    AZ_CVAR(AZ::TimeMs, net_UdpFragmentTimeoutMs, AZ::TimeMs{ 5000 }, nullptr, AZ::ConsoleFunctorFlags::Null, "Milliseconds to retain chunks of incomplete unreliable fragmented packets before timing them out");

    void UdpFragmentQueue::Update()
    {
        m_timeoutQueue.UpdateTimeouts(*this);
    }

    void UdpFragmentQueue::Reset()
    {
        m_timeoutQueue.Reset();
        m_sequenceGenerator.Reset();
        m_packetFragments.clear();
        m_latestReceivedFragmentSequence = InvalidSequenceId;
        m_deliveredFragments.Reset();
    }

    SequenceId UdpFragmentQueue::GetNextFragmentedSequenceId()
    {
        return m_sequenceGenerator.GetNextSequenceId();
    }

    PacketDispatchResult UdpFragmentQueue::ProcessReceivedChunk(UdpConnection* connection, IConnectionListener& connectionListener, UdpPacketHeader& header, ISerializer& serializer)
    {
        AZStd::unique_ptr<CorePackets::FragmentedPacket> packet = AZStd::make_unique<CorePackets::FragmentedPacket>();

        if (!serializer.Serialize(*packet, "Packet"))
        {
            AZLOG(NET_FragmentQueue, "Fragment failed serialization");
            return PacketDispatchResult::Failure;
        }

        const bool isReliable = header.GetIsReliable();
        const SequenceId fragmentSequence = packet->GetFragmentSequence();

        if (SequenceMoreRecent(fragmentSequence, m_latestReceivedFragmentSequence))
        {
            const SequenceId sequenceDelta = SequenceId(fragmentSequence - m_latestReceivedFragmentSequence);
            m_latestReceivedFragmentSequence = fragmentSequence;
            m_deliveredFragments.PushBackBits(static_cast<uint32_t>(sequenceDelta));
        }

        const SequenceId sequenceDelta = SequenceId(m_latestReceivedFragmentSequence - fragmentSequence);

        if (static_cast<uint32_t>(sequenceDelta) >= m_deliveredFragments.GetValidBitCount())
        {
            // Too old to process
            AZLOG(NET_FragmentQueue, "Fragment sequence ID is outside our tracked window");
            return PacketDispatchResult::Failure;
        }

        if (m_deliveredFragments.GetBit(static_cast<uint32_t>(sequenceDelta)))
        {
            // Received packet is a duplicate of one already forwarded to gameplay
            AZLOG(NET_FragmentQueue, "Received duplicate of fragmented packet %u, discarding", static_cast<uint32_t>(fragmentSequence));
            return PacketDispatchResult::Success;
        }

        const uint32_t chunkCount = packet->GetChunkCount();
        const uint32_t chunkIndex = packet->GetChunkIndex();

        // If this is the first time we've heard about this sequence, resize the vector appropriately
        const bool isNewPacketFragment = m_packetFragments.find(fragmentSequence) == m_packetFragments.end();
        PacketFragments& packetFragments = m_packetFragments[fragmentSequence];

        if (isNewPacketFragment)
        {
            packetFragments.resize(chunkCount);
        }

        if ((chunkCount != packetFragments.size()) || (chunkIndex >= chunkCount))
        {
            // Either we disagree on the number of chunks, or chunkIndex is bigger than the expected size, bail and disconnect
            AZLOG(NET_FragmentQueue, "Malformed chunk metadata in fragmented packet, chunkIndex %u, chunkCount %u, reservedSize %u", chunkIndex, chunkCount, static_cast<uint32_t>(packetFragments.size()));
            return PacketDispatchResult::Failure;
        }

        packetFragments[chunkIndex] = AZStd::move(packet);

        uint32_t totalPacketSize = 0;
        for (uint32_t index = 0; index < packetFragments.size(); ++index)
        {
            if (packetFragments[index] == nullptr)
            {
                if (!isReliable)
                {
                    m_timeoutQueue.RegisterItem(static_cast<uint64_t>(fragmentSequence), net_UdpFragmentTimeoutMs);
                }

                // We haven't received all chunks required to complete this packet yet
                return PacketDispatchResult::Success;
            }

            totalPacketSize += static_cast<uint32_t>(packetFragments[index]->GetChunkBuffer().GetSize());
        }

        // We now mark this sequence as delivered, so if by some chance all the individual chunks get redelivered again we don't double deliver the reconstructed packet
        m_deliveredFragments.SetBit(static_cast<uint32_t>(sequenceDelta), true);

        // All chunks have been received, reconstruct the original packet and deliver to the connection listener
        UdpPacketEncodingBuffer buffer;
        if (!buffer.Resize(totalPacketSize))
        {
            AZLOG_ERROR("Fragmented packet is too large to fit in UdpPacketEncodingBuffer");
            return PacketDispatchResult::Failure;
        }

        uint8_t* bufferPointer = buffer.GetBuffer();
        for (uint32_t index = 0; index < packetFragments.size(); ++index)
        {
            const uint32_t chunkSize = static_cast<uint32_t>(packetFragments[index]->GetChunkBuffer().GetSize());
            memcpy(bufferPointer, packetFragments[index]->GetChunkBuffer().GetBuffer(), chunkSize);
            bufferPointer += chunkSize;
        }

        // We can erase all the chunks now, packet is completed
        m_packetFragments.erase(fragmentSequence);

        NetworkOutputSerializer networkSerializer(buffer.GetBuffer(), static_cast<uint32_t>(buffer.GetSize()));
        {
            ISerializer& networkISerializer = networkSerializer; // To get the default typeinfo parameters in ISerializer

            // First, serialize out the header
            if (!header.SerializePacketFlags(networkSerializer))
            {
                AZLOG(NET_FragmentQueue, "Reconstructed fragmented packet failed packet flags serialization");
                return PacketDispatchResult::Failure;
            }

            if (!networkISerializer.Serialize(header, "Header"))
            {
                AZLOG(NET_FragmentQueue, "Reconstructed fragmented packet failed header serialization");
                return PacketDispatchResult::Failure;
            }
        }
        connection->GetPacketTracker().ProcessReceived(connection, header);
        PacketDispatchResult handledPacket;
        if (header.GetPacketType() < aznumeric_cast<PacketType>(CorePackets::PacketType::MAX))
        {
            handledPacket = connection->HandleCorePacket(connectionListener, header, networkSerializer);
        }
        else
        {
            handledPacket = connectionListener.OnPacketReceived(connection, header, networkSerializer);
        }

        return handledPacket;
    }

    TimeoutResult UdpFragmentQueue::HandleTimeout(TimeoutQueue::TimeoutItem& item)
    {
        const SequenceId fragmentSequence = static_cast<SequenceId>(item.m_userData & 0xFF);
        AZLOG(NET_FragmentQueue, "Timing out unreliable fragmented packet %u", static_cast<uint32_t>(fragmentSequence));
        m_packetFragments.erase(fragmentSequence);
        return TimeoutResult::Delete;
    }
}
