/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/Utilities/NetworkCommon.h>
#include <AzNetworking/Utilities/NetworkIncludes.h>
#include <AzCore/Console/ILogger.h>

namespace AzNetworking
{
    namespace Platform
    {
        bool SocketLayerInit();
        bool SocketLayerShutdown();
        bool SetSocketNonBlocking(SocketFd socketFd);
        void CloseSocket(SocketFd socketFd);
        int32_t GetLastNetworkError();
        bool ErrorIsWouldBlock(int32_t errorCode);
        bool ErrorIsForciblyClosed(int32_t errorCode, bool& ignoreError);
        const char* GetNetworkErrorDesc(int32_t errorCode);
    }

    DisconnectReason GetDisconnectReasonForSocketResult(int32_t socketResult)
    {
        switch (socketResult)
        {
        case SocketOpResultError:
            return DisconnectReason::Unknown;
        case SocketOpResultDisconnected:
            return DisconnectReason::RemoteHostClosedConnection;
        case SocketOpResultErrorNotOpen:
            return DisconnectReason::TransportError;
        case SocketOpResultErrorNoSsl:
            return DisconnectReason::SslFailure;
        }

        return DisconnectReason::MAX;
    }

    bool SocketLayerInit()
    {
        return Platform::SocketLayerInit();
    }

    bool SocketLayerShutdown()
    {
        return Platform::SocketLayerShutdown();
    }

    bool SetSocketNonBlocking(SocketFd socketFd)
    {
        return Platform::SetSocketNonBlocking(socketFd);
    }

    bool SetSocketNoDelay(SocketFd socketFd)
    {
        // Disable flow control
        int flag = 1;

        if (setsockopt(int32_t(socketFd), IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int)) != SocketOpResultSuccess)
        {
            const int32_t error = GetLastNetworkError();
            AZLOG_ERROR("Failed to disable flow control for socket (%d:%s)", error, GetNetworkErrorDesc(error));
            return false;
        }

        return true;
    }

    bool SetSocketBufferSizes(SocketFd socketFd, int32_t sendSize, int32_t recvSize)
    {
        if (setsockopt(int32_t(socketFd), SOL_SOCKET, SO_SNDBUF, (const char *)&sendSize, sizeof(sendSize)) != SocketOpResultSuccess)
        {
            const int32_t error = GetLastNetworkError();
            AZLOG_ERROR("Failed to set socket receive buffer size for socket (%d:%s)", error, GetNetworkErrorDesc(error));
            return false;
        }

        if (setsockopt(int32_t(socketFd), SOL_SOCKET, SO_RCVBUF, (const char *)&recvSize, sizeof(recvSize)) != SocketOpResultSuccess)
        {
            const int32_t error = GetLastNetworkError();
            AZLOG_ERROR("Failed to set socket receive buffer size for socket (%d:%s)", error, GetNetworkErrorDesc(error));
            return false;
        }

        return true;
    }

    void CloseSocket(SocketFd socketFd)
    {
        if (int32_t(socketFd) <= 0)
        {
            return;
        }

        Platform::CloseSocket(socketFd);
    }

    int32_t GetLastNetworkError()
    {
        return Platform::GetLastNetworkError();
    }

    bool ErrorIsWouldBlock(int32_t errorCode)
    {
        return Platform::ErrorIsWouldBlock(errorCode);
    }

    bool ErrorIsForciblyClosed(int32_t errorCode, bool& ignoreError)
    {
        return Platform::ErrorIsForciblyClosed(errorCode, ignoreError);
    }

    const char* GetNetworkErrorDesc(int32_t errorCode)
    {
        return Platform::GetNetworkErrorDesc(errorCode);
    }
}
