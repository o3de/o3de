/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Platform.h>
#include <AzCore/Time/ITime.h>
#include <AzNetworking/AzNetworking_Traits_Platform.h>
#include <AzNetworking/Utilities/NetworkCommon.h>
#include <AzNetworking/Utilities/NetworkIncludes.h>

#if AZ_TRAIT_USE_SOCKET_SERVER_EPOLL
#   include <sys/epoll.h>
#endif

namespace AzNetworking
{
    //! @class TcpSocketManager
    //! @brief internal helper implementation that manages basic details related to handling large numbers of TCP sockets efficiently.
    class TcpSocketManager
    {
    public:

        using SocketEventCallback = AZStd::function<void(SocketFd)>;

        TcpSocketManager();

        //! Adds the provided socket to the internal socket management mechanism.
        //! @param socketFd the socket file descriptor to add
        //! @return boolean true on success, false otherwise
        bool AddSocket(SocketFd socketFd);

        //! Removes the requested socket from the internal socket management mechanism.
        //! @param socketFd the socket file descriptor to remove
        //! @return boolean true on success, false otherwise
        bool ClearSocket(SocketFd socketFd);

        //! Processes any pending events for the set of sockets currently managed by this instance.
        //! @param maxBlockMs    the maximum milliseconds to block while gathering events
        //! @param readCallback  functor to invoke if a socket has pending data to read
        //! @param writeCallback functor to invoke if a socket is ready for writing
        void ProcessEvents(AZ::TimeMs maxBlockMs, const SocketEventCallback& readCallback, const SocketEventCallback& writeCallback);

    private:

        //! Internal helper for adding a socketFd to the socket manager
        //! @param socketFd the socket file descriptor to add
        void AddSocketHelper(SocketFd socketFd);

        //! Internal helper for removing a socketFd from the socket manager
        //! @param socketFd the socket file descriptor to remove
        void ClearSocketHelper(SocketFd socketFd);

        AZ_DISABLE_COPY_MOVE(TcpSocketManager);

#if AZ_TRAIT_USE_SOCKET_SERVER_EPOLL
        SocketFd m_epollFd = InvalidSocketFd;
#elif AZ_TRAIT_USE_SOCKET_SERVER_SELECT
        fd_set   m_sourceFdSet;
        fd_set   m_readerFdSet;
        fd_set   m_writerFdSet;
        SocketFd m_maxFd = SocketFd{ 0 };
#endif

        AZStd::vector<SocketFd> m_socketFds;
    };

    inline void TcpSocketManager::AddSocketHelper(SocketFd socketFd)
    {
        auto element = AZStd::find(m_socketFds.begin(), m_socketFds.end(), socketFd);
        if (element == m_socketFds.end())
        {
            m_socketFds.push_back(socketFd);
        }
    }

    inline void TcpSocketManager::ClearSocketHelper(SocketFd socketFd)
    {
        auto element = AZStd::find(m_socketFds.begin(), m_socketFds.end(), socketFd);
        if (element != m_socketFds.end())
        {
            m_socketFds.erase(element);
        }
    }
}
