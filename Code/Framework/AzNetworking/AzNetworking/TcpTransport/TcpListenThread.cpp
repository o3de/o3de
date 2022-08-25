/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/TcpTransport/TcpListenThread.h>
#include <AzNetworking/TcpTransport/TcpNetworkInterface.h>
#include <AzNetworking/TcpTransport/TcpSocketManager.h>
#include <AzCore/Console/ILogger.h>

namespace AzNetworking
{
    static constexpr AZ::TimeMs ListenThreadUpdateRateMs{ 10 };

    TcpListenThread::TcpListenThread()
        : TimedThread("AzNetworking::TcpListenThread", ListenThreadUpdateRateMs)
    {
        ;
    }

    TcpListenThread::~TcpListenThread()
    {
        Stop();
        Join();
    }

    bool TcpListenThread::Listen(TcpNetworkInterface& tcpNetworkInterface)
    {
        bool existsCheck = false;
        auto visitor = [&tcpNetworkInterface, &existsCheck](ListenPort& listenPort)
        {
            if (listenPort.m_tcpNetworkInterface == &tcpNetworkInterface)
            {
                existsCheck = true;
            }
        };
        m_listenPorts.Visit(visitor);
        if (existsCheck)
        {
            AZLOG_ERROR("Attempted to insert the same network interface twice");
            return false;
        }

        ++m_listenPortCount;
        ListenPort listenPort;
        listenPort.m_listenPort = tcpNetworkInterface.GetPort();
        listenPort.m_tcpNetworkInterface = &tcpNetworkInterface;
        m_listenPorts.PushBackItem(listenPort);
        AZLOG_INFO("TcpListenThread opening port: %d for incoming traffic", aznumeric_cast<int32_t>(listenPort.m_listenPort));

        // Start the listen thread if we have ports to listen on
        if (!IsRunning())
        {
            Start();
        }

        return true;
    }

    bool TcpListenThread::StopListening(TcpNetworkInterface& tcpNetworkInterface)
    {
        --m_listenPortCount;

        auto visitor = [&tcpNetworkInterface](ListenPort& listenPort)
        {
            if (listenPort.m_tcpNetworkInterface == &tcpNetworkInterface)
            {
                // This kills any ability to route new incoming connections to the network interface
                listenPort.m_tcpNetworkInterface = nullptr;
                listenPort.m_listenSocket.Close();
            }
        };
        m_listenPorts.Visit(visitor);

        // Stops the listen thread if there are no more listen sockets active
        if (IsRunning() && (m_listenPortCount == 0))
        {
            Stop();
            Join();
        }

        return true;
    }

    uint32_t TcpListenThread::GetSocketCount() const
    {
        return m_listenPortCount;
    }

    AZ::TimeMs TcpListenThread::GetUpdateTimeMs() const
    {
        return m_updateTimeMs;
    }

    void TcpListenThread::OnStart()
    {
        AZLOG_INFO("Starting TcpListenThread");
    }

    void TcpListenThread::OnStop()
    {
        AZLOG_INFO("Stopping TcpListenThread");
    }

    void TcpListenThread::OnUpdate(AZ::TimeMs updateRateMs)
    {
        // Don't proceed with any processing if our network state is not valid
        if (!EnsureSocketState())
        {
            return;
        }

        AZ::TimeMs startTimeMs = AZ::GetElapsedTimeMs();

        struct sockaddr_in newConnection;
        const int32_t connectionLength = aznumeric_cast<int32_t>(sizeof(newConnection));
        memset(&newConnection, 0, connectionLength);

        auto readCallback = [this, newConnection](SocketFd socketFd)
        {
            auto visitor = [this, newConnection, socketFd](ListenPort& listenPort)
            {
                if (listenPort.m_listenSocket.GetSocketFd() == socketFd)
                {
                    HandleSocketAccept((void*)&newConnection, connectionLength, listenPort);
                }
            };
            m_listenPorts.Visit(visitor);
        };
        auto writeCallback = [](SocketFd) {};
        m_tcpSocketManager.ProcessEvents(updateRateMs, readCallback, writeCallback);

        auto cleanupUnused = [](AZ::ThreadSafeDeque<ListenPort>::DequeType& deque)
        {
            AZStd::remove_if(deque.begin(), deque.end(), [](ListenPort& listenPort) { return listenPort.m_tcpNetworkInterface == nullptr; });
        };
        m_listenPorts.VisitDeque(cleanupUnused);

        m_updateTimeMs += AZ::GetElapsedTimeMs() - startTimeMs;
    }

    bool TcpListenThread::EnsureSocketState()
    {
        bool result = true;
        auto visitor = [this, &result](ListenPort& listenPort)
        {
            if (listenPort.m_tcpNetworkInterface && !listenPort.m_listenSocket.IsOpen())
            {
                if (!listenPort.m_listenSocket.Listen(listenPort.m_listenPort))
                {
                    listenPort.m_listenSocket.Close();
                    result = false;
                }
                else
                {
                    result &= m_tcpSocketManager.AddSocket(listenPort.m_listenSocket.GetSocketFd());
                }
            }
        };
        m_listenPorts.Visit(visitor);
        return result;
    }

    bool TcpListenThread::HandleSocketAccept(void* newConnection, int32_t newConnectionLength, ListenPort& listenPort)
    {
        struct sockaddr* newConnectionSockAddr = (struct sockaddr*)newConnection;
        const int32_t socketFdInt = aznumeric_cast<int32_t>(listenPort.m_listenSocket.GetSocketFd());

        socklen_t newConnectionLengthSocklen = aznumeric_cast<socklen_t>(newConnectionLength);
        const SocketFd newSocketFd = aznumeric_cast<SocketFd>(::accept(socketFdInt, newConnectionSockAddr, &newConnectionLengthSocklen));

        if (newSocketFd <= SocketFd{ 0 })
        {
            const int32_t error = GetLastNetworkError();
            AZLOG_WARN("Failed to accept incoming connection (%d:%s)", error, GetNetworkErrorDesc(error));
            return false;
        }

        // Hand new connection off to a worker thread
        struct sockaddr_in* newConnectionSockAddrIn = (struct sockaddr_in*)newConnection;
        TcpNetworkInterface::PendingConnection pendingConnection(
            newSocketFd,
            newConnectionSockAddrIn->sin_addr.s_addr,
            newConnectionSockAddrIn->sin_port,
            listenPort.m_listenPort
        );
        listenPort.m_tcpNetworkInterface->QueueNewConnection(pendingConnection);
        return true;
    }
}
