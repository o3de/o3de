/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzNetworking/TcpTransport/TcpSocket.h>
#include <AzNetworking/TcpTransport/TcpSocketManager.h>
#include <AzNetworking/Framework/INetworkInterface.h>
#include <AzNetworking/Utilities/TimedThread.h>
#include <AzCore/Threading/ThreadSafeDeque.h>

namespace AzNetworking
{
    class TcpNetworkInterface;

    //! @class TcpListenThread
    //! @brief A class for managing a TCP listen socket and accepting new incoming connections.
    class TcpListenThread final
        : public TimedThread
    {
    public:

        TcpListenThread();
        ~TcpListenThread() override;

        //! Opens a new listen socket capable of accepting incoming connections for the provided TcpNetworkInterface.
        //! @param tcpNetworkInterface the TcpNetworkInterface being opened to incoming connections
        //! @return boolean true if the operation was successful, false if it failed
        bool Listen(TcpNetworkInterface& tcpNetworkInterface);

        //! Stops listening for incoming connections for the provided TcpNetworkInterface.
        //! @param tcpNetworkInterface the TcpNetworkInterface being closed to new incoming connections
        //! @return boolean true if the operation was successful, false if it failed
        bool StopListening(TcpNetworkInterface& tcpNetworkInterface);

        //! Returns the number of active listen ports bound to this thread.
        //! @return the number of active listen ports bound to this thread
        uint32_t GetSocketCount() const;

        //! Gets the total elapsed time spent updating the background thread in milliseconds
        //! @return the total elapsed time spent updating the background thread in milliseconds
        AZ::TimeMs GetUpdateTimeMs() const;

    private:

        AZ_DISABLE_COPY_MOVE(TcpListenThread);

        struct ListenPort
        {
            TcpSocket m_listenSocket;
            TcpNetworkInterface* m_tcpNetworkInterface = nullptr;
            uint16_t m_listenPort;
        };

        void OnStart() override;
        void OnStop() override;
        void OnUpdate(AZ::TimeMs updateRateMs) override;

        bool EnsureSocketState();
        bool HandleSocketAccept(void* newConnection, int32_t newConnectionLength, ListenPort& listenPort);

        uint32_t m_listenPortCount = 0;
        TcpSocketManager m_tcpSocketManager;
        AZ::ThreadSafeDeque<ListenPort> m_listenPorts;
        AZ::TimeMs m_updateTimeMs = AZ::Time::ZeroTimeMs;
    };
}
