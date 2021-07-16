/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/TcpTransport/TcpSocketManager.h>
#include <AzCore/Console/ILogger.h>

#if !AZ_TRAIT_USE_SOCKET_SERVER_EPOLL && !AZ_TRAIT_USE_SOCKET_SERVER_SELECT

namespace AzNetworking
{
    TcpSocketManager::TcpSocketManager()
    {
        ;
    }

    bool TcpSocketManager::AddSocket(SocketFd socketFd)
    {
        AddSocketHelper(socketFd);
        return true;
    }

    bool TcpSocketManager::ClearSocket(SocketFd socketFd)
    {
        ClearSocketHelper(socketFd);
        return true;
    }

    void TcpSocketManager::ProcessEvents([[maybe_unused]]AZ::TimeMs maxBlockMs, const SocketEventCallback& readCallback, const SocketEventCallback& writeCallback)
    {
        // No edge triggering, just brute force iterate all socketFds and invoke the callbacks
        for (auto socketFd : m_socketFds)
        {
            readCallback(socketFd);
            writeCallback(socketFd);
        }
    }
}

#endif
