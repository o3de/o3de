/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzNetworking/ConnectionLayer/IConnection.h>
#include <AzNetworking/ConnectionLayer/IConnectionListener.h>
#include <AzNetworking/UdpTransport/DtlsEndpoint.h>
#include <AzNetworking/UdpTransport/UdpPacketTracker.h>
#include <AzNetworking/UdpTransport/UdpReliableQueue.h>
#include <AzNetworking/UdpTransport/UdpFragmentQueue.h>
#include <AzCore/Console/ILogger.h>

namespace AzNetworking
{
    // Forwards
    class UdpPacketHeader;

    enum class PacketTimeoutResult
    {
        Acked,
        Lost,
        Pending
    };
    const char* GetEnumString(PacketTimeoutResult value);

    //! @class UdpConnection
    //! @brief Connection class for udp endpoints.
    class UdpConnection
    :   public IConnection
    {
        friend class UdpFragmentQueue;
        friend class UdpNetworkInterface;

    public:

        //! Constructor
        //! @param connectionId     the connection identifier to use for this connection
        //! @param remoteAddress    the remote address this connection
        //! @param networkInterface reference of the network interface that owns this connection instance
        //! @param connectionRole   whether this connection was the connector or acceptor
        UdpConnection(ConnectionId connectionId, const IpAddress& remoteAddress, UdpNetworkInterface& networkInterface, ConnectionRole connectionRole);
        ~UdpConnection() override;

        //! Helper to exchange dtls handshake data during connection handshake
        //! @param dtlsData    data buffer containing dtls handshake packet
        //! @return the current result code for the dtls handshake operation, failed, pending, or complete
        DtlsEndpoint::ConnectResult ProcessHandshakeData(const UdpPacketEncodingBuffer& dtlsData);

        //! Updates the connection heartbeat if active.
        //! @param currentTimeMs current wall clock time in milliseconds
        void UpdateHeartbeat(AZ::TimeMs currentTimeMs);

        //! IConnection interface.
        // @{
        bool SendReliablePacket(const IPacket& packet) override;
        PacketId SendUnreliablePacket(const IPacket& packet) override;
        bool WasPacketAcked(PacketId packetId) const override;
        ConnectionState GetConnectionState() const override;
        ConnectionRole GetConnectionRole() const override;
        bool Disconnect(DisconnectReason reason, TerminationEndpoint endpoint) override;
        void SetConnectionMtu(uint32_t connectionMtu) override;
        uint32_t GetConnectionMtu() const override;
        // @}

        //! Returns a suitable encryption endpoint for this connection type.
        //! @return reference to the connections encryption endpoint
        DtlsEndpoint& GetDtlsEndpoint();

        //! Retrieves packet delivery tracker instance for the specified connection.
        //! @return reference to the requested packet tracker instance
        const UdpPacketTracker& GetPacketTracker() const;

        //! Retrieves packet delivery tracker instance for the specified connection.
        //! @return reference to the requested packet tracker instance
        UdpPacketTracker& GetPacketTracker();

        //! Returns the number of unacked reliable messages still pending in the reliable queue.
        //! @return the number of unacked reliable messages still pending in the reliable queue
        uint32_t GetReliableQueueSize() const;

        //! Acks a packetId.
        //! @param packetId      the PacketId of the packet being acked
        //! @param currentTimeMs current wall clock time in milliseconds
        void ProcessAcked(PacketId packetId, AZ::TimeMs currentTimeMs);

        //! Sets the timeout identifier for this connection instance.
        //! @param timeoutId the timeoutId to use for this connection instance
        void SetTimeoutId(TimeoutId timeoutId);

        //! Retrieves the timeout identifier for this connection instance.
        //! @return the timeout identifier for this connection instance
        TimeoutId GetTimeoutId() const;

    protected:

        //! Prepare a reliable packet for transmission.
        //! @param packetId identifier of the packet being sent
        //! @param packet   reference to the packet being transmitted
        //! @return boolean true on success, false on failure
        bool PrepareReliablePacketForSend(PacketId packetId, SequenceId reliableSequenceId, const IPacket& packet);

        //! Process a packet for sending.
        //! @param packetId   identifier of the packet being sent
        //! @param packet     reference to the packet being transmitted
        //! @param packetSize packet size in bytes
        //! @param reliability whether or not to guarantee delivery
        void ProcessSent(PacketId packetId, const IPacket& packet, uint32_t packetSize, ReliabilityType reliability);

        //! Process a timed out packet header.
        //! @param packetId    identifier of the packet that timed out
        //! @param reliability whether or not the packet that timed out was marked reliable
        //! @return PacketTimeoutResult::Acked if the packet was confirmed to be received prior to timeout, PacketTimeoutResult::Lost if not
        PacketTimeoutResult ProcessTimeout(PacketId packetId, ReliabilityType reliability);

        //! Process a received packet header.
        //! @param header        the packet header received to process
        //! @param serializer    the output serializer containing the transmitted packet data
        //! @param packetSize    the size of the received packet in bytes
        //! @param currentTimeMs current wall clock time in milliseconds
        //! @return boolean true on successful handling of the received header
        bool ProcessReceived(UdpPacketHeader& header, const NetworkOutputSerializer& serializer, uint32_t packetSize, AZ::TimeMs currentTimeMs);

        //! Handle a core network packet.
        //! @param listener   a connection listener to receive connection related events
        //! @param header     the packet header received to process
        //! @param serializer the output serializer containing the transmitted packet data
        //! @return PacketDispatchResult result of processing the core packet
        PacketDispatchResult HandleCorePacket(IConnectionListener& listener, UdpPacketHeader& header, ISerializer& serializer);

        AZ_DISABLE_COPY_MOVE(UdpConnection);

        UdpNetworkInterface& m_networkInterface;
        UdpPacketTracker m_packetTracker;
        UdpReliableQueue m_reliableQueue;
        UdpFragmentQueue m_fragmentQueue;
        ConnectionState  m_state = ConnectionState::Disconnected;
        ConnectionRole   m_connectionRole = ConnectionRole::Connector;
        DtlsEndpoint     m_dtlsEndpoint;

        AZ::TimeMs m_lastSentPacketMs;
        uint32_t   m_unackedPacketCount = 0;
        uint32_t   m_connectionMtu = MaxUdpTransmissionUnit;

        TimeoutId m_timeoutId;
        uint32_t  m_timeoutCounter = 0;

        AZStd::mutex m_sendPacketMutex;
    };
}

#include <AzNetworking/UdpTransport/UdpConnection.inl>
