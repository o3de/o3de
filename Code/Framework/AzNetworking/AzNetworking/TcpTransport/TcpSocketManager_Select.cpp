/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/TcpTransport/TcpSocketManager.h>
#include <AzCore/Console/ILogger.h>

#if AZ_TRAIT_USE_SOCKET_SERVER_SELECT

namespace AzNetworking
{
    TcpSocketManager::TcpSocketManager()
    {
        FD_ZERO(&m_sourceFdSet);
        FD_ZERO(&m_readerFdSet);
        FD_ZERO(&m_writerFdSet);
    }

    TcpSocketManager::~TcpSocketManager() = default;

    bool TcpSocketManager::AddSocket(SocketFd socketFd)
    {
        const int32_t rawFd = static_cast<int32_t>(socketFd);
        if (rawFd <= 0 || rawFd >= FD_SETSIZE)
        {
            return false;
        }

        FD_SET(rawFd, &m_sourceFdSet);
        m_maxFd = AZStd::max<SocketFd>(m_maxFd, socketFd);
        AddSocketHelper(socketFd);
        return true;
    }

    bool TcpSocketManager::ClearSocket(SocketFd socketFd)
    {
        const int32_t rawFd = static_cast<int32_t>(socketFd);
        if (rawFd > 0 && rawFd < FD_SETSIZE)
        {
            FD_CLR(rawFd, &m_sourceFdSet);
        }
        ClearSocketHelper(socketFd);
        m_maxFd = SocketFd{ 0 };
        for (SocketFd activeFd : m_socketFds)
        {
            m_maxFd = AZStd::max<SocketFd>(m_maxFd, activeFd);
        }
        return true;
    }

    void TcpSocketManager::ProcessEvents(AZ::TimeMs maxBlockMs, const SocketEventCallback& readCallback, const SocketEventCallback& writeCallback)
    {
        if (static_cast<int32_t>(m_maxFd) <= 0 || m_socketFds.empty())
        {
            // There are no available sockets to process
            return;
        }

        m_readerFdSet = m_sourceFdSet;
        m_writerFdSet = m_sourceFdSet;

        const int64_t maxMs = AZStd::max<int64_t>(0, static_cast<int64_t>(maxBlockMs));
        struct timeval tv;
        tv.tv_sec = static_cast<decltype(tv.tv_sec)>(maxMs / 1000);
        tv.tv_usec = static_cast<decltype(tv.tv_usec)>((maxMs % 1000) * 1000);
        const int32_t selectResult = ::select(static_cast<int32_t>(m_maxFd) + 1, &m_readerFdSet, &m_writerFdSet, nullptr, &tv);
        if (selectResult < 0)
        {
            const int32_t error = GetLastNetworkError();
            AZLOG_ERROR("select returned an error (%d:%s)", error, GetNetworkErrorDesc(error));
        }
        if (selectResult <= 0)
        {
            return;
        }

        for (auto socketFd : m_socketFds)
        {
            // Sockets with pending data awaiting receipt
            if (FD_ISSET(static_cast<int32_t>(socketFd), &m_readerFdSet))
            {
                readCallback(socketFd);
            }

            // Sockets with free space for sending data
            if (FD_ISSET(static_cast<int32_t>(socketFd), &m_writerFdSet))
            {
                writeCallback(socketFd);
            }
        }
    }
}

#endif
