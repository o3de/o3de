/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/TcpTransport/TcpNetworkInterface.h>
#include <AzNetworking/TcpTransport/TcpSocketManager.h>
#include <AzNetworking/AutoGen/CorePackets.AutoPackets.h>
#include <AzNetworking/Utilities/NetworkCommon.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ILogger.h>

namespace AzNetworking
{
#if AZ_TRAIT_USE_OPENSSL
    AZ_CVAR(bool, net_TcpUseEncryption, false, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Enable encryption on Tcp based connections");
#else
    static const bool net_TcpUseEncryption = false;
#endif

    TcpNetworkInterface::TcpNetworkInterface(AZ::Name name, IConnectionListener& connectionListener, TrustZone trustZone, TcpListenThread& listenThread)
        : m_name(name)
        , m_trustZone(trustZone)
        , m_connectionListener(connectionListener)
        , m_listenThread(listenThread)
    {
        ;
    }

    TcpNetworkInterface::~TcpNetworkInterface()
    {
        FlushQueuedRemoves();
        m_listenThread.StopListening(*this);
    }

    AZ::Name TcpNetworkInterface::GetName() const
    {
        return m_name;
    }

    ProtocolType TcpNetworkInterface::GetType() const
    {
        return ProtocolType::Tcp;
    }

    TrustZone TcpNetworkInterface::GetTrustZone() const
    {
        return m_trustZone;
    }

    uint16_t TcpNetworkInterface::GetPort() const
    {
        return m_port;
    }

    IConnectionSet& TcpNetworkInterface::GetConnectionSet()
    {
        return m_connectionSet;
    }

    IConnectionListener& TcpNetworkInterface::GetConnectionListener()
    {
        return m_connectionListener;
    }

    bool TcpNetworkInterface::Listen(uint16_t port)
    {
        m_port = port;
        return m_listenThread.Listen(*this);
    }

    ConnectionId TcpNetworkInterface::Connect(const IpAddress& remoteAddress)
    {
        const ConnectionId connectionId = m_connectionSet.GetNextConnectionId();
        AZStd::unique_ptr<TcpConnection> connection = AZStd::make_unique<TcpConnection>(connectionId, remoteAddress, *this, m_trustZone, net_TcpUseEncryption);
        AZ_Assert(connection->GetConnectionRole() == ConnectionRole::Connector, "Invalid role for connection");
        connection->Connect();

        TcpSocket* tcpSocket = connection->GetTcpSocket();
        if (tcpSocket == nullptr)
        {
            return InvalidConnectionId;
        }

        if (!(tcpSocket->IsOpen() && m_tcpSocketManager.AddSocket(tcpSocket->GetSocketFd())))
        {
            tcpSocket->Close();
            AZLOG_ERROR("Failed to bind new incoming connection to socket manager, failed fd: %d", static_cast<int32_t>(tcpSocket->GetSocketFd()));
            return InvalidConnectionId;
        }

        AZLOG_INFO("Adding new socket %d", static_cast<int32_t>(tcpSocket->GetSocketFd()));
        connection->SendReliablePacket(CorePackets::InitiateConnectionPacket());
        m_connectionListener.OnConnect(connection.get());
        m_connectionSet.AddConnection(AZStd::move(connection));
        return connectionId;
    }

    void TcpNetworkInterface::Update([[maybe_unused]] AZ::TimeMs deltaTimeMs)
    {
        const AZ::TimeMs startTimeMs = AZ::GetElapsedTimeMs();

        AcceptNewConnections();

        auto readCallback = [this, startTimeMs](SocketFd socketFd) { HandleConnectionRecv(socketFd, startTimeMs); };
        auto writeCallback = [this](SocketFd socketFd) { HandleConnectionSend(socketFd); };
        m_tcpSocketManager.ProcessEvents(AZ::Time::ZeroTimeMs, readCallback, writeCallback);

        FlushQueuedRemoves();

        // Update metrics
        GetMetrics().m_connectionCount = m_connectionSet.GetConnectionCount();
        GetMetrics().m_updateTimeMs += AZ::GetElapsedTimeMs() - startTimeMs;
    }

    bool TcpNetworkInterface::SendReliablePacket(ConnectionId connectionId, const IPacket& packet)
    {
        IConnection* connection = m_connectionSet.GetConnection(connectionId);
        if (connection == nullptr)
        {
            return false;
        }
        return connection->SendReliablePacket(packet);
    }

    PacketId TcpNetworkInterface::SendUnreliablePacket(ConnectionId connectionId, const IPacket& packet)
    {
        IConnection* connection = m_connectionSet.GetConnection(connectionId);
        if (connection == nullptr)
        {
            return InvalidPacketId;
        }
        return connection->SendUnreliablePacket(packet);
    }

    bool TcpNetworkInterface::WasPacketAcked(ConnectionId connectionId, PacketId packetId)
    {
        IConnection* connection = m_connectionSet.GetConnection(connectionId);
        if (connection == nullptr)
        {
            return false;
        }
        return connection->WasPacketAcked(packetId);
    }

    bool TcpNetworkInterface::StopListening()
    {
        m_port = 0;
        return m_listenThread.StopListening(*this);
    }

    bool TcpNetworkInterface::Disconnect(ConnectionId connectionId, DisconnectReason reason)
    {
        IConnection* connection = m_connectionSet.GetConnection(connectionId);
        if (connection == nullptr)
        {
            return false;
        }
        return connection->Disconnect(reason, TerminationEndpoint::Local);
    }

    void TcpNetworkInterface::SetTimeoutMs(AZ::TimeMs timeoutMs)
    {
        m_timeoutMs = timeoutMs;
    }

    AZ::TimeMs TcpNetworkInterface::GetTimeoutMs() const
    {
        return m_timeoutMs;
    }

    void TcpNetworkInterface::QueueNewConnection(const PendingConnection& pendingConnection)
    {
        m_pendingConnections.PushBackItem(pendingConnection);
    }

    bool TcpNetworkInterface::HandleConnectionRecv(SocketFd socketFd, [[maybe_unused]] AZ::TimeMs currentTimeMs)
    {
        TcpConnection* connection = m_connectionSet.GetConnection(socketFd);
        if (connection == nullptr)
        {
            return false;
        }
        const bool result = connection->UpdateRecv();
        if (!result)
        {
            connection->Disconnect(DisconnectReason::RemoteHostClosedConnection, TerminationEndpoint::Remote);
        }
        return result;
    }

    bool TcpNetworkInterface::HandleConnectionSend(SocketFd socketFd)
    {
        TcpConnection* connection = m_connectionSet.GetConnection(socketFd);
        if (connection == nullptr)
        {
            return false;
        }
        connection->UpdateSend();
        return true;
    }

    void TcpNetworkInterface::RequestDisconnect(TcpConnection* connection, DisconnectReason reason)
    {
        m_pendingRemoves.emplace_back(PendingRemove{ connection->GetRegisteredSocketFd(), reason });
    }

    void TcpNetworkInterface::AcceptNewConnections()
    {
        if (m_pendingConnections.Size() <= 0)
        {
            // Early out to avoid the deque below invoking a heap allocation
            // This is a performance optimization only, due to the expense of heap allocation calls on windows
            return;
        }

        AZ::ThreadSafeDeque<PendingConnection>::DequeType pendingConnections;
        m_pendingConnections.Swap(pendingConnections);

        for (auto pendingConnection : pendingConnections)
        {
            IpAddress remoteAddress = IpAddress(ByteOrder::Network, pendingConnection.m_remoteIpAddress, pendingConnection.m_remotePort);
            if (net_TcpUseEncryption)
            {
                TlsSocket newSocket = TlsSocket(pendingConnection.m_socketFd, m_trustZone);
                AddConnectionHelper(m_connectionSet.GetNextConnectionId(), remoteAddress, newSocket);
            }
            else
            {
                TcpSocket newSocket = TcpSocket(pendingConnection.m_socketFd);
                AddConnectionHelper(m_connectionSet.GetNextConnectionId(), remoteAddress, newSocket);
            }
        }
    }

    void TcpNetworkInterface::AddConnectionHelper(ConnectionId connectionId, const IpAddress& remoteAddress, TcpSocket& tcpSocket)
    {
        if (!(tcpSocket.IsOpen() && m_tcpSocketManager.AddSocket(tcpSocket.GetSocketFd())))
        {
            tcpSocket.Close();
            AZLOG_ERROR("Failed to bind new incoming connection to socket manager, failed fd: %d", static_cast<int32_t>(tcpSocket.GetSocketFd()));
            return;
        }
        AZLOG(NET_TcpTraffic, "Adding new socket %d", static_cast<int32_t>(tcpSocket.GetSocketFd()));
        AZStd::unique_ptr<TcpConnection> connection = AZStd::make_unique<TcpConnection>(connectionId, remoteAddress, *this, tcpSocket);
        AZ_Assert(connection->GetConnectionRole() == ConnectionRole::Acceptor, "Invalid role for connection");
        GetConnectionListener().OnConnect(connection.get());
        m_connectionSet.AddConnection(AZStd::move(connection));
    }

    void TcpNetworkInterface::FlushQueuedRemoves()
    {
        for (uint32_t i = 0; i < m_pendingRemoves.size(); ++i)
        {
            const SocketFd socketFd = m_pendingRemoves[i].m_socketFd;
            const DisconnectReason reason = m_pendingRemoves[i].m_reason;

            TcpConnection* connection = m_connectionSet.GetConnection(socketFd);
            if (connection == nullptr)
            {
                continue;
            }

            AZLOG_INFO("Removing socket %d due to %s", static_cast<int32_t>(socketFd), AZStd::string(ToString(reason)).c_str());
            m_tcpSocketManager.ClearSocket(socketFd);
            m_connectionSet.DeleteConnection(socketFd);
        }

        m_pendingRemoves.resize_no_construct(0);
    }

    TcpNetworkInterface::PendingConnection::PendingConnection(SocketFd socketFd, uint32_t remoteIpAddress, uint16_t remotePort, uint16_t listenPort)
        : m_socketFd(socketFd)
        , m_remoteIpAddress(remoteIpAddress)
        , m_remotePort(remotePort)
        , m_listenPort(listenPort)
    {
        ;
    }
}
