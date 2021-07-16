/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzNetworking/DataStructures/RingBufferBitset.h>
#include <AzNetworking/Utilities/NetworkCommon.h>
#include <AzNetworking/UdpTransport/UdpPacketHeader.h>

namespace AzNetworking
{
    class UdpConnection;

    enum class PacketAckState
    {
        Acked, 
        Nacked, 
        Unknown_TooNew, 
        Unknown_TooOld
    };

    const char *GetEnumString(PacketAckState value);

    //! @class UdpPacketIdWindow
    //! @brief Wrapper class that handles management of ack status for a large range of packet id's.
    class UdpPacketIdWindow
    {
    public:

        static const uint32_t PacketWindowAckCount = 16384; // The total number of packet id's to track

        using PacketAckContainer = RingbufferBitset<PacketWindowAckCount>;

        UdpPacketIdWindow();
        ~UdpPacketIdWindow();

        //! Resets the packet ack window, putting it back into a default initialized state.
        void Reset();

        //! Retrieves the latest ack status for a given PacketId.
        //! @param packetId the identifier of the packet to lookup the latest ack status of
        //! @return the current status for the given PacketId
        PacketAckState GetPacketAckStatus(PacketId packetId) const;

        //! Returns the latest known SequenceId managed by this UdpPacketIdWindow instance.
        //! @return the latest known SequenceId managed by this UdpPacketIdWindow instance
        SequenceId GetHeadSequenceId() const;

        //! Returns the SequenceRolloverCount for the number of times the sequence ids have rolled over.
        //! @return the number of times the sequence ids have rolled over
        SequenceRolloverCount GetSequenceRolloverCount() const;

        //! Returns the latest ack vector contained in this UdpPacketIdWindow.
        //! @param outWindow the window to store the most recent ack state in
        //! @return reference to the window where the ack vector was stored
        BitsetChunk& GetMostRecentAckState(BitsetChunk& outWindow) const;

        //! Returns the underlying packet ack container used for retaining packet ack status.
        //! @return the underlying packet ack container used for retaining packet ack status
        const PacketAckContainer& GetPacketAckContainer() const;

        //! Updates the internal ack state for the newly received PacketId.
        //! @param header the packet header received to process
        //! @return boolean false if updating failed, and the packet should be discarded without further processing
        bool UpdateForReceivedPacket(UdpPacketHeader& header);

        //! Updates the internal ack state to replicate the received remote ack status.
        //! @param connection the connection this packet was received on, used for ack callbacks
        //! @param header     the packet header received to process
        void UpdateForRemoteAckStatus(UdpConnection* connection, UdpPacketHeader& header);

        //! Prints the most recent ack window status to the Logger.
        void PrintStatus() const;

    private:

        SequenceId m_headSequenceId;
        PacketId   m_headPacketId;
        PacketAckContainer m_ackWindow;
        SequenceRolloverCount m_sequenceRolloverCount;
    };
}

#include <AzNetworking/UdpTransport/UdpPacketIdWindow.inl>
