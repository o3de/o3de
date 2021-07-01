/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzNetworking/PacketLayer/IPacket.h>
#include <AzNetworking/PacketLayer/IPacketHeader.h>
#include <AzNetworking/Serialization/ISerializer.h>
#include <AzNetworking/ConnectionLayer/ConnectionMetrics.h>
#include <AzNetworking/DataStructures/RingBufferBitset.h>

namespace AzNetworking
{
    class UdpPacketTracker;

    //! @class UdpPacketHeader
    //! @brief Udp packet header class.
    class UdpPacketHeader final
        : public IPacketHeader
    {
        friend class UdpPacketIdWindow;

    public:

        AZ_RTTI(UdpPacketHeader, "{21A11FF3-6829-4A59-9906-C06EF7F39AC1}", IPacketHeader);

        //! Default constructor, for when receiving a header from a remote connection.
        UdpPacketHeader();

        //! Constructor for generating a new header to send to a remote connection.
        //! @param packetTracker    packet delivery tracker instance for the connection in question
        //! @param packetType       type of packet
        //! @param reliableSequence reliable sequence value, or InvalidSequenceId if the packet is unreliable
        UdpPacketHeader(UdpPacketTracker& packetTracker, PacketType packetType, SequenceId reliableSequence);

        //! Constructor for generating generic header with just a packet id, used for dispatching a bulk message.
        //! @param packetType type of packet
        //! @param packetId   packet id we're duplicating
        UdpPacketHeader(PacketType packetType, PacketId packetId);

        //! Full constructor for unit tests.
        //! @param packetType         type of packet
        //! @param localSequence      local sequence number, this is the sequence for this packet instance
        //! @param remoteSequence     remote sequence number for ack replication, this is the latest sequence we've received from the remote endpoint
        //! @param reliableSequence   if this is a reliable packet, this is the reliable sequence number
        //! @param sequenceWindow     this is the ack vector for ack replication, corresponding to remoteSequence
        //! @param localRolloverCount this is the reconstructed rollover count, used to convert localSequence to a full PacketId
        UdpPacketHeader
        (
            PacketType  packetType, 
            SequenceId  localSequence, 
            SequenceId  remoteSequence, 
            SequenceId  reliableSequence, 
            BitsetChunk sequenceWindow, 
            SequenceRolloverCount localRolloverCount
        );

        ~UdpPacketHeader() override = default;

        //! IPacketHeader interface.
        // @{
        PacketType GetPacketType() const override;
        PacketId GetPacketId() const override;
        bool IsPacketFlagSet(PacketFlag flag) const override;
        void SetPacketFlag(PacketFlag flag, bool value) override;
        // @}

        //! Sets the packet flag bitset for this packet.
        //! @param flags The packet flag bitset
        void SetPacketFlags(PacketFlagBitset flags);

        //! Returns whether or not this header is for a reliably transmitted packet.
        //! @return whether or not this header is for a reliably transmitted packet
        bool GetIsReliable() const;

        //! Retrieve the local sequence from this packet header.
        //! @return packet header local sequence
        SequenceId GetLocalSequenceId() const;

        //! Retrieve the remote sequence being acked.
        //! @return packet header remote sequence
        SequenceId GetRemoteSequenceId() const;

        //! Retrieve the reliable sequence if this was a reliable packet, InvalidSequenceId otherwise.
        //! @return packet header reliable sequence if this was a reliable packet, InvalidSequenceId otherwise
        SequenceId GetReliableSequenceId() const;

        //! Retrieve the sequence window from this packet header.
        //! @return the sequence window from this packet header
        BitsetChunk GetSequenceWindow() const;

        //! Retrieve the sequence rollover count from this packet header.
        //! @return the sequence rollover count from this packet header
        SequenceRolloverCount GetSequenceRolloverCount() const;

        //! Base serialize method for all serializable structures or classes to implement.
        //! @param serializer ISerializer instance to use for serialization
        //! @return boolean true for success, false for serialization failure
        bool Serialize(ISerializer& serializer);

        //! Specialized serialize method for UDP Packet Flags
        //! @param serializer ISerializer instance to use for serialization
        //! @return boolean true for success, false for serialization failure
        bool SerializePacketFlags(ISerializer& serializer);

    private:

        void SetLocalRolloverCount(SequenceRolloverCount rolloverCount);

        PacketType  m_packetType;
        SequenceId  m_localSequence;
        SequenceId  m_remoteSequence;
        SequenceId  m_reliableSequence;
        BitsetChunk m_sequenceWindow;

        SequenceRolloverCount m_localRolloverCount;

        // UDP Packet flags are not serialized by the Serialize method and must be serialized separately
        PacketFlagBitset m_packetFlags;
    };
}

#include <AzNetworking/UdpTransport/UdpPacketHeader.inl>
