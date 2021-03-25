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
