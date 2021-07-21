/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/TcpTransport/TcpSocketManager.h>
#include <AzCore/Console/ILogger.h>

#if AZ_TRAIT_USE_SOCKET_SERVER_EPOLL

namespace AzNetworking
{
    static constexpr uint32_t MaxEpollEvents = 256;

    TcpSocketManager::TcpSocketManager()
        // Don't propagate fd's to child processes, not that we should ever be spawning children
        : m_epollFd(static_cast<SocketFd>(epoll_create1(EPOLL_CLOEXEC)))
    {
        if (m_epollFd == InvalidSocketFd)
        {
            const int32_t error = GetLastNetworkError();
            AZLOG_ERROR("Failed to create epollFd, terminating application (%d:%s)", error, GetNetworkErrorDesc(error));
            AZ_Assert(false, "Failed to create epollFd, terminating application");
            exit(EXIT_FAILURE);
        }
    }

    bool TcpSocketManager::AddSocket(SocketFd socketFd)
    {
        if (socketFd < SocketFd{ 0 })
        {
            return false;
        }

        struct epoll_event fdEvents;
        fdEvents.events = EPOLLIN | EPOLLOUT | EPOLLET;
        fdEvents.data.fd = static_cast<int32_t>(socketFd);

        if (epoll_ctl(static_cast<int32_t>(m_epollFd), EPOLL_CTL_ADD, static_cast<int32_t>(socketFd), &fdEvents) < 0)
        {
            const int32_t error = GetLastNetworkError();
            AZLOG_ERROR("Call to epoll_ctl to bind socket failed (%d:%s)", error, GetNetworkErrorDesc(error));
            return false;
        }

        AddSocketHelper(socketFd);
        return true;
    }

    bool TcpSocketManager::ClearSocket(SocketFd socketFd)
    {
        ClearSocketHelper(socketFd);
        return true;
    }

    void TcpSocketManager::ProcessEvents(AZ::TimeMs maxBlockMs, const SocketEventCallback& readCallback, const SocketEventCallback& writeCallback)
    {
        struct epoll_event socketEvents[MaxEpollEvents];
        const int32_t numEpollEvents = epoll_wait(static_cast<int32_t>(m_epollFd), socketEvents, MaxEpollEvents, -1);
        if (numEpollEvents < 0)
        {
            const int32_t error = GetLastNetworkError();
            AZLOG_ERROR("epoll_wait returned an error (%d:%s)", error, GetNetworkErrorDesc(error));
        }

        if (numEpollEvents > 0)
        {
            for (int32_t event = 0; event < numEpollEvents; ++event)
            {
                const SocketFd socketFd = static_cast<SocketFd>(socketEvents[event].data.fd);
                if (socketEvents[event].events & EPOLLIN)
                {
                    readCallback(socketFd);
                }

                if (socketEvents[event].events & EPOLLOUT)
                {
                    writeCallback(socketFd);
                }
            }
        }
    }
}

#endif
