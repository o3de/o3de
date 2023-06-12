/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/Utilities/NetworkCommon.h>
#include <AzCore/PlatformIncl.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ILogger.h>

namespace AzNetworking
{
    namespace Platform
    {
        bool SocketLayerInit()
        {
            WSADATA wsaData;
            const WORD wVersionRequested = MAKEWORD(2, 2);
            const int32_t error = WSAStartup(wVersionRequested, &wsaData);

            if (error != NO_ERROR)
            {
                AZLOG_ERROR("WSAStartup failed with error code %d", error);
                return false;
            }

            AZLOG_DEBUG("Network layer initialized");
            return true;
        }

        bool SocketLayerShutdown()
        {
            WSACleanup();
            AZLOG_DEBUG("Network layer shut down");
            return true;
        }

        bool SetSocketNonBlocking(SocketFd socketFd)
        {
            // Set non-blocking
            DWORD nonBlocking = 1;

            if (ioctlsocket(int32_t(socketFd), FIONBIO, &nonBlocking) != SocketOpResultSuccess)
            {
                const int32_t error = GetLastNetworkError();
                AZLOG_ERROR("Failed to set non-blocking for socket (%d:%s)", error, GetNetworkErrorDesc(error));
                return false;
            }

            return true;
        }

        void CloseSocket(SocketFd socketFd)
        {
            closesocket(int32_t(socketFd));
        }

        int32_t GetLastNetworkError()
        {
            return WSAGetLastError();
        }

        bool ErrorIsWouldBlock(int32_t errorCode)
        {
            return errorCode == WSAEWOULDBLOCK;
        }

        bool ErrorIsForciblyClosed(int32_t errorCode, bool& ignoreError)
        {
            static const int32_t WindowsForciblyClosedError = 10054; // 'Existing connection was forcibly closed by the remote host...'
            if (errorCode == WindowsForciblyClosedError) // Filter WindowsForciblyClosedError messages on windows
            {
                if (auto console = AZ::Interface<AZ::IConsole>::Get(); console)
                {
                    console->GetCvarValue("net_UdpIgnoreWin10054", ignoreError);
                }
                return true;
            }
            return false;
        }

        const char* GetNetworkErrorDesc(int32_t errorCode)
        {
            static AZ_THREAD_LOCAL char buffer[1024];
    #if AZ_TRAIT_OS_IS_HOST_OS_PLATFORM
            FormatMessageA
            (
                FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                nullptr,
                errorCode,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                buffer,
                sizeof(buffer),
                nullptr
            );
            // Strip newlines from FormatMessageA output
            const size_t messageLen = strnlen_s(buffer, sizeof(buffer));
            if ((messageLen > 2) && (buffer[messageLen - 2] == '\r' || buffer[messageLen - 2] == '\n'))
            {
                buffer[messageLen - 2] = '\0';
            }
    #else
            strerror_s(buffer, sizeof(buffer), errorCode);
    #endif
            return buffer;
        }
    }
}
