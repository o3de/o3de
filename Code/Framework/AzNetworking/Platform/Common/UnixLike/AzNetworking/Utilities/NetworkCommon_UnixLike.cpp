/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/Utilities/NetworkCommon.h>
#include <AzCore/Console/ILogger.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <string.h>

namespace AzNetworking
{
    namespace Platform
    {
        bool SocketLayerInit()
        {
            // nothing to do
            AZLOG_INFO("Network layer initialized");
            return true;
        }

        bool SocketLayerShutdown()
        {
            // nothing to do
            AZLOG_INFO("Network layer shut down");
            return true;
        }

        bool SetSocketNonBlocking(SocketFd socketFd)
        {
            // Set non-blocking
            int nonBlocking = 1;

            if (fcntl(int32_t(socketFd), F_SETFL, O_NONBLOCK, nonBlocking) < 0)
            {
                const int32_t error = GetLastNetworkError();
                AZLOG_ERROR("Failed to set non-blocking for socket (%d:%s)", error, GetNetworkErrorDesc(error));
                return false;
            }

            return true;
        }

        void CloseSocket(SocketFd socketFd)
        {
            close(int32_t(socketFd));
        }

        int32_t GetLastNetworkError()
        {
            return errno;
        }

        bool ErrorIsWouldBlock(int32_t errorCode)
        {
            return errorCode == EWOULDBLOCK;
        }

        bool ErrorIsForciblyClosed(int32_t, bool&)
        {
            return false;
        }

        const char* GetNetworkErrorDesc(int32_t errorCode)
        {
            static AZ_THREAD_LOCAL char buffer[1024];
            [[maybe_unused]] auto strErrorResult = strerror_r(errorCode, buffer, sizeof(buffer));
            return buffer;
        }
    }
}
