/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzNetworking/PacketLayer/IPacket.h>
#include <AzNetworking/ConnectionLayer/IConnection.h>
#include <AzNetworking/ConnectionLayer/SequenceGenerator.h>
#include <AzNetworking/UdpTransport/UdpPacketIdWindow.h>
#include <AzCore/std/containers/unordered_map.h>

namespace AzNetworking
{
    class NetworkOutputSerializer;
    class UdpConnection;
    class UdpNetworkInterface;
    class UdpPacketHeader;

    struct PendingPacket
    {
        SequenceId m_reliableSequenceId;
        AZStd::unique_ptr<IPacket> m_packet;
    };

    //! @class UdpReliableQueue
    //! @brief provides a reliability queue on top of the unreliable UDP connection layer.
    class UdpReliableQueue
    {
    public:

        UdpReliableQueue() = default;
        ~UdpReliableQueue();

        //! Returns the next sequence id for this generator instance.
        //! @return the next sequence id for this generator instance
        SequenceId GetNextSequenceId();

        //! Returns the number of unacked reliable messages still pending in the reliable queue.
        //! @return the number of unacked reliable messages still pending in the reliable queue
        uint32_t GetQueueSize() const;

        //! Called when we're going to transmit a packet that we want to be reliable.
        //! @param packetId           packet id of the packet we're sending
        //! @param reliableSequenceId the reliable sequence identifier of the packet we're sending
        //! @param packet             reference to the packet being transmitted
        //! @return boolean true on success, false on failure
        bool PrepareForSend(PacketId packetId, SequenceId reliableSequenceId, const IPacket& packet);

        //! Called when a reliable packet has been received.
        //! @param header the header for the received reliable packet
        bool OnPacketReceived(const UdpPacketHeader& header);

        //! Called when a packet is acked by the remote connection.
        //! @param networkInterface reference to the network interface bound to the UdpConnection instance
        //! @param connection       reference of the connection instance generating the event
        //! @param packetId         packet id of the acked packet
        void OnPacketAcked(UdpNetworkInterface& networkInterface, UdpConnection& connection, PacketId packetId);

        //! Called when a packet is deemed lost by the remote connection.
        //! @param networkInterface reference to the network interface bound to the UdpConnection instance
        //! @param connection       reference of the connection instance generating the event
        //! @param packetId         packet id of the lost packet
        //! @return boolean true if the packet was lost and no retry attempt was made, false otherwise
        bool OnPacketLost(UdpNetworkInterface& networkInterface, UdpConnection& connection, PacketId packetId);

    private:

        static constexpr uint32_t PacketWindowAckCount = 16384; // The total number of packet id's to track

        using PacketAckContainer = RingbufferBitset<PacketWindowAckCount>;
        using PendingPacketMap = AZStd::unordered_map<PacketId, PendingPacket>;

        SequenceGenerator  m_reliableSequenceGenerator;
        SequenceId         m_lastReceivedReliableSequenceId = InvalidSequenceId;
        PacketAckContainer m_receivedSequenceHistory;
        PendingPacketMap   m_packetWindow;
    };
}
