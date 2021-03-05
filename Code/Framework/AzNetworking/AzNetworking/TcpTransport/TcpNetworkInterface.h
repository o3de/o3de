/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
        bool Disconnect(ConnectionId connectionId, DisconnectReason reason) override;
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

        struct ConnectionTimeoutFunctor final
            : public ITimeoutHandler
        {
            ConnectionTimeoutFunctor(TcpNetworkInterface& networkInterface);
            TimeoutResult HandleTimeout(TimeoutQueue::TimeoutItem& item) override;
        private:
            AZ_DISABLE_COPY_MOVE(ConnectionTimeoutFunctor);
            TcpNetworkInterface& m_networkInterface;
        };

        struct PendingRemove
        {
            SocketFd m_socketFd;
            DisconnectReason m_reason;
        };

        AZ::Name m_name;
        TrustZone m_trustZone;
        uint16_t m_port = 0;
        IConnectionListener& m_connectionListener;
        TcpConnectionSet m_connectionSet;
        TcpSocketManager m_tcpSocketManager;
        AZ::ThreadSafeDeque<PendingConnection> m_pendingConnections;
        AZStd::vector<PendingRemove> m_pendingRemoves;
        TimeoutQueue m_connectionTimeoutQueue;
        TcpListenThread& m_listenThread;

        friend class TcpConnection; // For access to private RequestDisconnect() method
    };
}
