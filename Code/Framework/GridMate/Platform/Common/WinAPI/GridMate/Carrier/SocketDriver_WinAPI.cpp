/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/std/chrono/types.h>
#include <AzCore/std/containers/array.h>
#include <GridMate/Carrier/SocketDriver.h>

#include <GridMate_Traits_Platform.h>
#include <GridMate/Carrier/Driver.h>
#include <AzCore/std/base.h>

#include <WinSock2.h>
#include <ws2tcpip.h>
#include <Mswsock.h>

namespace GridMate
{
    namespace Platform
    {
        int Bind(GridMate::SocketDriverCommon::SocketType socket, const sockaddr* addr, size_t addrlen)
        {
            return ::bind(socket, addr, static_cast<socklen_t>(addrlen));
        }

        int GetAddressInfo(const char* node, const char* service, const struct addrinfo* hints, struct addrinfo** res)
        {
            return ::getaddrinfo(node, service, hints, res);
        }

        void FreeAddressInfo(struct addrinfo* res)
        {
            ::freeaddrinfo(res);
        }

        bool IsValidSocket(GridMate::SocketDriverCommon::SocketType s)
        {
            return s != INVALID_SOCKET;
        }

        GridMate::SocketDriverCommon::SocketType GetInvalidSocket()
        {
            return INVALID_SOCKET;
        }

        bool IsSocketError(AZ::s64 result)
        {
            return result == SOCKET_ERROR;
        }

        int GetSocketError()
        {
            return WSAGetLastError();
        }

        timeval GetTimeValue(AZStd::chrono::microseconds timeOut)
        {
            timeval t;
            t.tv_sec = static_cast<long>(timeOut.count() / 1000000);
            t.tv_usec = static_cast<long>(timeOut.count() % 1000000);
            return t;
        }

        const char* GetSocketErrorString(int error, SocketErrorBuffer& array)
        {
            sprintf_s(array.data(), array.size() - 1, "%d", error);
            return array.data();
        }

        GridMate::Driver::ResultCode SetSocketBlockingMode(GridMate::SocketDriverCommon::SocketType sock, bool blocking)
        {
            AZ::s64 result = SOCKET_ERROR;
            u_long val = blocking ? 0 : 1;
            result = ioctlsocket(sock, FIONBIO, &val);
            if (IsSocketError(result))
            {
                return GridMate::Driver::EC_SOCKET_MAKE_NONBLOCK;
            }
            return GridMate::Driver::EC_OK;
        }

        GridMate::Driver::ResultCode SetFastSocketClose(GridMate::SocketDriverCommon::SocketType socket, bool isDatagram)
        {
            // faster socket close
            linger l;
            l.l_onoff = 0;
            l.l_linger = 0;
            setsockopt(socket, SOL_SOCKET, SO_LINGER, reinterpret_cast<char*>(&l), sizeof(l));

            // MS Transport Provider IOCTL to control
            // reporting PORT_UNREACHABLE messages
            // on UDP sockets via recv/WSARecv/etc.
            // Path TRUE in input buffer to enable (default if supported),
            // FALSE to disable.
#if !defined(SIO_UDP_CONNRESET)
#define SIO_UDP_CONNRESET   _WSAIOW(IOC_VENDOR, 12)
#endif
            if (isDatagram)
            {
                // disable new behavior using
                // IOCTL: SIO_UDP_CONNRESET
                DWORD dwBytesReturned = 0;
                BOOL isReportPortUnreachable = FALSE;
                if (WSAIoctl(socket, SIO_UDP_CONNRESET, &isReportPortUnreachable, sizeof(isReportPortUnreachable), NULL, 0, &dwBytesReturned, NULL, NULL) == SOCKET_ERROR)
                {
                    closesocket(socket);
                    int error = GetSocketError();
                    (void)error;
                    AZ_TracePrintf("GridMate", "SocketDriver::Initialize - WSAIoctl failed with code %d\n", error);
                    return GridMate::Driver::EC_SOCKET_SOCK_OPT;
                }
            }
            return GridMate::Driver::EC_OK;
        }

        void PrepareFamilyType(int ft, bool& isIpv6)
        {
            isIpv6 = (ft == GridMate::Driver::BSD_AF_INET6);
        }
    }
}
