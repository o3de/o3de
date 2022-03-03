/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzNetworking/TcpTransport/TcpPacketHeader.h>
#include <AzNetworking/TcpTransport/TcpConnectionSet.h>
#include <AzNetworking/TcpTransport/TcpListenThread.h>
#include <AzNetworking/ConnectionLayer/IConnection.h>
#include <AzNetworking/Framework/INetworkInterface.h>
#include <AzCore/Threading/ThreadSafeDeque.h>

namespace AzNetworking
{
    class IConnectionListener;

    //! @class TcpNetworkInterface
    //! @brief This class implements a TCP network interface.
    //!
    //! TcpNetworkInterface is an implementation of AzNetworking::INetworkInterface.
    //! Unlike UDP, TCP implements a variety of transport features such as congestion
    //! avoidance, flow control, and reliability. These features are valuable, but TCP
    //! offers minimal configuration of them. This is why UdpNetworkInterface offers
    //! similar features, but with greater flexibility in configuration. If your project doesn't
    //! require the low latency of UDP, consider using TCP.
    //! 
    //! ## Packet structure
    //! 
    //! * Flags - A bitfield a receiving endpoint can quickly inspect to learn about configuration of a packet
    //! * Header - Details the type of packet and other information related to reliability
    //! * Payload - The actual serialized content of the packet
    //! 
    //! For more information, read [Networking Packets](http://o3de.org/docs/user-guide/networking/packets) in the O3DE documentation.
    //! 
    //! ## Reliability
    //! 
    //! TCP packets can only be sent reliably. This is a feature of TCP itself.
    //! 
    //! ## Fragmentation
    //! 
    //! TCP implements fragmentation under the hood. Consumers of TCP packets will never
    //! need to worry about reconstructing the contents over multiple transmissions.
    //! 
    //! ## Compression
    //! 
    //! Compression here refers to content insensitive compression using libraries like
    //! LZ4. If enabled, the target payload is run through the compressor and replaces
    //! the original payload if it's in fact smaller. To tell if compression is enabled
    //! on a given packet, we operate on a bit in the packet's Flags. The Sender writes
    //! this bit while the Receiver checks it to see if a packet needs to be
    //! decompressed.
    //! 
    //! ## Encryption
    //! 
    //! AzNetworking uses the [OpenSSL](https://www.openssl.org/) library to implement TLS encryption. If enabled,
    //! the O3DE network layer handles the OpenSSL handshake under the hood using provided certificates.
    class TcpNetworkInterface final
        : public INetworkInterface
    {
    public:

        //! @struct PendingConnection
        //! @brief helper structure for transferring new pending connections from the listen thread to network interface.
        struct PendingConnection
        {
            PendingConnection(SocketFd socketFd, uint32_t remoteIpAddress, uint16_t remotePort, uint16_t listenPort);
            SocketFd   m_socketFd;
            uint32_t   m_remoteIpAddress;
            uint16_t   m_remotePort;
            uint16_t   m_listenPort;
        };

        //! Constructor.
        //! @param name               the name of this network interface instance.
        //! @param connectionListener reference to the connection listener responsible for handling all connection events
        //! @param trustZone          the trust level assigned to this network interface, server to server or client to server
        //! @param listenThread       the listen thread to bind to this network interface
        TcpNetworkInterface(AZ::Name name, IConnectionListener& connectionListener, TrustZone trustZone, TcpListenThread& listenThread);
        ~TcpNetworkInterface() override;

        //! INetworkInterface interface.
        //! @{
        AZ::Name GetName() const override;
        ProtocolType GetType() const override;
        TrustZone GetTrustZone() const override;
        uint16_t GetPort() const override;
        IConnectionSet& GetConnectionSet() override;
        IConnectionListener& GetConnectionListener() override;
        bool Listen(uint16_t port) override;
        ConnectionId Connect(const IpAddress& remoteAddress) override;
        void Update(AZ::TimeMs deltaTimeMs) override;
        bool SendReliablePacket(ConnectionId connectionId, const IPacket& packet) override;
        PacketId SendUnreliablePacket(ConnectionId connectionId, const IPacket& packet) override;
        bool WasPacketAcked(ConnectionId connectionId, PacketId packetId) override;
        bool StopListening() override;
        bool Disconnect(ConnectionId connectionId, DisconnectReason reason) override;
        void SetTimeoutMs(AZ::TimeMs timeoutMs) override;
        AZ::TimeMs GetTimeoutMs() const override;
        //! @}

        //! Queues a new incoming connection for this network interface.
        //! @param pendingConnection info on the new incoming connection
        void QueueNewConnection(const PendingConnection& pendingConnection);

    private:

        //! Performs connection receive updates for a single socket.
        //! @param socketFd      socket descriptor with new incoming data
        //! @param currentTimeMs current time in milliseconds for metrics management
        bool HandleConnectionRecv(SocketFd socketFd, AZ::TimeMs currentTimeMs);

        //! Performs connection send updates for a single socket.
        //! @param socketFd socket descriptor to send data to
        bool HandleConnectionSend(SocketFd socketFd);

        //! Internal helper to cleanly remove a connection from the network interface.
        //! @param connection pointer to the connection to disconnect
        //! @param reason     reason for the disconnect
        void RequestDisconnect(TcpConnection* connection, DisconnectReason reason);

        //! Internal method to activate all pending connections.
        void AcceptNewConnections();

        //! Method that correctly adds a new connection to the network interface.
        //! @param connectionId  connection id of the new connection
        //! @param remoteAddress address of the remote endpoint
        //! @param tcpSocket     underlying TCP socket connected to the remote endpoint
        void AddConnectionHelper(ConnectionId connectionId, const IpAddress& remoteAddress, TcpSocket& tcpSocket);

        //! Deletes all connections queued for removal from the network interface.
        void FlushQueuedRemoves();

        AZ_DISABLE_COPY_MOVE(TcpNetworkInterface);

        struct PendingRemove
        {
            SocketFd m_socketFd;
            DisconnectReason m_reason;
        };

        AZ::Name m_name;
        TrustZone m_trustZone;
        uint16_t m_port = 0;
        AZ::TimeMs m_timeoutMs = AZ::Time::ZeroTimeMs;
        IConnectionListener& m_connectionListener;
        TcpConnectionSet m_connectionSet;
        TcpSocketManager m_tcpSocketManager;
        AZ::ThreadSafeDeque<PendingConnection> m_pendingConnections;
        AZStd::vector<PendingRemove> m_pendingRemoves;
        TcpListenThread& m_listenThread;

        friend class TcpConnection; // For access to private RequestDisconnect() method
    };
}
