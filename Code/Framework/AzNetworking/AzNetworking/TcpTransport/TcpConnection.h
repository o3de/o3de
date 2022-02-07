/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzNetworking/DataStructures/ByteBuffer.h>
#include <AzNetworking/DataStructures/TimeoutQueue.h>
#include <AzNetworking/ConnectionLayer/IConnection.h>
#include <AzNetworking/ConnectionLayer/IConnectionListener.h>
#include <AzNetworking/ConnectionLayer/ConnectionMetrics.h>
#include <AzNetworking/TcpTransport/TcpSocket.h>
#include <AzNetworking/TcpTransport/TlsSocket.h>
#include <AzNetworking/TcpTransport/TcpRingBuffer.h>
#include <AzNetworking/TcpTransport/TcpPacketHeader.h>

namespace AzNetworking
{
    class TcpNetworkInterface;
    class ICompressor;

    // 20 byte IPv4 header + 20 byte TCP header
    static constexpr uint32_t TcpPacketHeaderSize = 20 + 20;

    //! @class TcpConnection
    //! @brief connection layer for TCP connection management.
    class TcpConnection final
        : public IConnection
    {
    public:

        //! Construct with an existing socket, used when accepting an incoming connection
        //! @param connectionId     connection identifier of this connection instance
        //! @param remoteAddress    IP address of the remote endpoint
        //! @param networkInterface TcpNetworkInterface that owns this connection instance
        //! @param socket           TCP socket to take ownership of and use for sending and receiving data
        //! @param timeoutId        timeout identifier of this connection instance
        TcpConnection
        (
            ConnectionId connectionId,
            const IpAddress& remoteAddress,
            TcpNetworkInterface& networkInterface,
            TcpSocket& socket,
            TimeoutId timeoutId
        );

        //! Construct a new socket with optional encryption, used when initiating a new connection
        //! @param connectionId     connection identifier of this connection instance
        //! @param remoteAddress    IP address of the remote endpoint
        //! @param networkInterface TcpNetworkInterface that owns this connection instance
        //! @param trustZone        for encrypted connections, the level of trust we associate with this connection (internal or external)
        //! @param useEncryption    if true connections will be made over TLS
        TcpConnection
        (
            ConnectionId connectionId,
            const IpAddress& remoteAddress,
            TcpNetworkInterface& networkInterface,
            TrustZone trustZone,
            bool useEncryption
        );

        ~TcpConnection() override;

        //! Returns the TcpSocket bound to this TcpConnection.
        //! @return the TcpSocket bound to this TcpConnection
        TcpSocket* GetTcpSocket() const;

        //! Sets the timeout identifier for this TcpConnection.
        //! @param timeoutId the timeout identifier to use for this TcpConnection
        void SetTimeoutId(TimeoutId timeoutId);

        //! Returns the timeout identifier for this TcpConnection.
        //! @return the timeout identifier for this TcpConnection
        TimeoutId GetTimeoutId() const;

        //! Returns true if this connection instance is in an open state, and is capable of actively sending and receiving packets.
        //! @return boolean true if this connection instance is in an open state
        bool IsOpen() const;

        //! Connects to the provided remote address.
        //! @return boolean true on success
        bool Connect();

        //! Handles any new outgoing network traffic.
        void UpdateSend();

        //! Handles any new incoming network traffic.
        //! @return boolean true if the socket is still active, false if it has been remotely terminated
        bool UpdateRecv();

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

        //! Sets the registered socket file descriptor for this TcpConnection in the associated ConnectionSet instance.
        //! @param registeredSocketFd the socket file descriptor for this TcpConnection in the associated ConnectionSet instance
        void SetRegisteredSocketFd(SocketFd registeredSocketFd);

        //! Returns the socket file descriptor for this TcpConnection in the associated ConnectionSet instance.
        //! @return the socket file descriptor for this TcpConnection in the associated ConnectionSet instance
        SocketFd GetRegisteredSocketFd() const;

    private:

        //! Transmits a packet to the connected connection.
        //! @param packetType     packet type of the buffer being transmitted
        //! @param payloadBuffer  packet buffer to transmit
        //! @param currentTimeMs current process time in milliseconds
        //! @return boolean true if the packet was transmitted (NOT AN INDICATION OF DELIVERY)
        bool SendPacketInternal(PacketType packetType, TcpPacketEncodingBuffer& payloadBuffer, AZ::TimeMs currentTimeMs);

        //! Receives a packet from the connected connection.
        //! @param outHeader      header of the received packet
        //! @param outBuffer      encoded buffer of the received packet
        //! @param currentTimeMs current process time in milliseconds
        //! @return boolean true if a packet has been received, false otherwise
        bool ReceivePacketInternal(TcpPacketHeader& outHeader, TcpPacketEncodingBuffer& outBuffer, AZ::TimeMs currentTimeMs);

        //! Decompresses an incoming packet data buffer.
        //! @param packetBuffer    the compressed packet buffer to decode
        //! @param packetSize      the size of the compressed packet buffer
        //! @param packetBufferOut the decoded data
        //! @return boolean true on success, false on failure
        bool DecompressPacket(const uint8_t* packetBuffer, AZStd::size_t packetSize, TcpPacketEncodingBuffer& packetBufferOut) const;

        //! Private copy operator, do not allow copying instances
        TcpConnection& operator=(const TcpConnection&) = delete;

        TcpNetworkInterface& m_networkInterface;
        AZStd::unique_ptr<TcpSocket> m_socket;
        AZStd::unique_ptr<ICompressor> m_compressor;

        TimeoutId       m_timeoutId;
        PacketId        m_lastSentPacketId = InvalidPacketId;
        ConnectionState m_state = ConnectionState::Disconnected;
        ConnectionRole  m_connectionRole = ConnectionRole::Connector;
        SocketFd        m_registeredSocketFd;

        static const uint32_t SendRingbufferSize = 1024 * 1024; // 1 MB send buffer
        TcpRingBuffer<SendRingbufferSize> m_sendRingbuffer;

        static const uint32_t RecvRingbufferSize = 1024 * 1024; // 1 MB recv buffer
        TcpRingBuffer<RecvRingbufferSize> m_recvRingbuffer;
    };
}

#include <AzNetworking/TcpTransport/TcpConnection.inl>
