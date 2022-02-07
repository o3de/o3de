/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzNetworking/PacketLayer/IPacket.h>
#include <AzNetworking/PacketLayer/IPacketHeader.h>
#include <AzNetworking/AutoGen/CorePackets.AutoPackets.h>
#include <AzNetworking/ConnectionLayer/SequenceGenerator.h>
#include <AzNetworking/DataStructures/RingBufferBitset.h>
#include <AzNetworking/DataStructures/TimeoutQueue.h>
#include <AzNetworking/UdpTransport/UdpSocket.h>
#include <AzCore/std/containers/unordered_map.h>

namespace AzNetworking
{
    class IConnectionListener;
    class UdpConnection;
    class UdpPacketHeader;

    //! @class UdpFragmentQueue
    //! @brief Class for reconstructing packet chunks into the original unsegmented packet.
    class UdpFragmentQueue
        : public ITimeoutHandler
    {

    public:
        virtual ~UdpFragmentQueue() = default;

        //! Updates the UdpFragmentQueue timeout queue.
        void Update();

        //! Resets all internal state.
        void Reset();

        //! Returns the next (outgoing) fragmented sequenceId for this FragmentQueue instance.
        SequenceId GetNextFragmentedSequenceId();

        //! Processes a received chunk and delivers the final reconstructed packet if possible.
        //! @param connection         pointer to the connection this packet chunk was received on
        //! @param connectionListener the connection listener for delivery of completed packets
        //! @param header             the chunk packet header
        //! @param serializer         the serializer containing the chunk body
        //! @return PacketDispatchResult result of processing the chunk
        PacketDispatchResult ProcessReceivedChunk(UdpConnection* connection, IConnectionListener& connectionListener, UdpPacketHeader& header, ISerializer& serializer);

    private:

        //! Handler callback for timed out items.
        //! @param item containing registered timeout details
        //! @return ETimeoutResult for whether to re-register or discard the timeout params
        TimeoutResult HandleTimeout(TimeoutQueue::TimeoutItem& item) override;

        TimeoutQueue m_timeoutQueue;
        SequenceGenerator m_sequenceGenerator;

        using PacketFragments = AZStd::vector<AZStd::unique_ptr<CorePackets::FragmentedPacket>>;
        AZStd::unordered_map<SequenceId, PacketFragments> m_packetFragments;

        static constexpr uint32_t PacketWindowAckCount = 16384; // The total number of packet id's to track
        using PacketAckContainer = RingbufferBitset<PacketWindowAckCount>;

        SequenceId m_latestReceivedFragmentSequence = InvalidSequenceId;
        PacketAckContainer m_deliveredFragments;
    };
}
