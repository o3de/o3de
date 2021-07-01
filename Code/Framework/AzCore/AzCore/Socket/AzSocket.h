/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Socket/AzSocket_Platform.h>
#include <AzCore/PlatformIncl.h>

#include <AzCore/Socket/AzSocket_fwd.h>
#include <AzCore/std/string/string.h>

typedef fd_set  AZFD_SET;
typedef timeval AZTIMEVAL;

namespace AZ
{
    namespace AzSock
    {
        //! All socket errors should be -ve since a 0 or +ve result indicates success
        enum class AzSockError : AZ::s32
        {
            eASE_NO_ERROR           = 0,        // No error reported

            eASE_SOCKET_INVALID     = -1,       // Invalid socket

            eASE_EACCES             = -2,       // Tried to establish a connection to an invalid address (such as a broadcast address)
            eASE_EADDRINUSE         = -3,       // Specified address already in use
            eASE_EADDRNOTAVAIL      = -4,       // Invalid address was specified
            eASE_EAFNOSUPPORT       = -5,       // Invalid socket type (or invalid protocol family)
            eASE_EALREADY           = -6,       // Connection is being established (if the function is called again in non-blocking state)
            eASE_EBADF              = -7,       // Invalid socket number specified
            eASE_ECONNABORTED       = -8,       // Connection was aborted
            eASE_ECONNREFUSED       = -9,       // Connection refused by destination
            eASE_ECONNRESET         = -10,       // Connection was reset (TCP only)
            eASE_EFAULT             = -11,      // Invalid socket number specified
            eASE_EHOSTDOWN          = -12,      // Other end is down and unreachable
            eASE_EINPROGRESS        = -13,      // Action is already in progress (when non-blocking)
            eASE_EINTR              = -14,      // A blocking socket call was cancelled
            eASE_EINVAL             = -15,      // Invalid argument or function call
            eASE_EISCONN            = -16,      // Specified connection is already established
            eASE_EMFILE             = -17,      // No more socket descriptors available
            eASE_EMSGSIZE           = -18,      // Message size is too large
            eASE_ENETUNREACH        = -19,      // Destination is unreachable
            eASE_ENOBUFS            = -20,      // Insufficient working memory
            eASE_ENOPROTOOPT        = -21,      // Invalid combination of 'level' and 'optname'
            eASE_ENOTCONN           = -22,      // Specified connection is not established
            eASE_ENOTINITIALISED    = -23,      // Socket layer not initialised (e.g. need to call WSAStartup() on Windows)
            eASE_EOPNOTSUPP         = -24,      // Socket type cannot accept connections
            eASE_EPIPE              = -25,      // The writing side of the socket has already been closed
            eASE_EPROTONOSUPPORT    = -26,      // Invalid protocol family
            eASE_ETIMEDOUT          = -27,      // TCP resend timeout occurred
            eASE_ETOOMANYREFS       = -28,      // Too many multicast addresses specified
            eASE_EWOULDBLOCK        = -29,      // Time out occurred when attempting to perform action
            eASE_EWOULDBLOCK_CONN   = -30,      // Only applicable to connect() - a non-blocking connection has been attempted and is in progress

            eASE_MISC_ERROR         = -1000
        };

        enum class AzSocketOption : AZ::s32
        {
            REUSEADDR               = 0x04,
            KEEPALIVE               = 0x08,
            LINGER                  = 0x20,
        };

        const char* GetStringForError(AZ::s32 errorNumber);

        AZ::u32 HostToNetLong(AZ::u32 hstLong);
        AZ::u32 NetToHostLong(AZ::u32 netLong);
        AZ::u16 HostToNetShort(AZ::u16 hstShort);
        AZ::u16 NetToHostShort(AZ::u16 netShort);
        AZ::s32 GetHostName(AZStd::string& hostname);

        AZSOCKET Socket();
        AZSOCKET Socket(AZ::s32 af, AZ::s32 type, AZ::s32 protocol);
        AZ::s32 SetSockOpt(AZSOCKET sock, AZ::s32 level, AZ::s32 optname, const char* optval, AZ::s32 optlen);
        AZ::s32 SetSocketOption(AZSOCKET sock, AzSocketOption opt, bool enable);
        AZ::s32 EnableTCPNoDelay(AZSOCKET sock, bool enable);
        AZ::s32 SetSocketBlockingMode(AZSOCKET sock, bool blocking);
        AZ::s32 CloseSocket(AZSOCKET sock);
        AZ::s32 Shutdown(AZSOCKET sock, AZ::s32 how);
        AZ::s32 GetSockName(AZSOCKET sock, AzSocketAddress& addr);
        AZ::s32 Connect(AZSOCKET sock, const AzSocketAddress& addr);
        AZ::s32 Listen(AZSOCKET sock, AZ::s32 backlog);
        AZSOCKET Accept(AZSOCKET sock, AzSocketAddress& addr);
        AZ::s32 Send(AZSOCKET sock, const char* buf, AZ::s32 len, AZ::s32 flags);
        AZ::s32 Recv(AZSOCKET sock, char* buf, AZ::s32 len, AZ::s32 flags);
        AZ::s32 Bind(AZSOCKET sock, const AzSocketAddress& addr);

        AZ::s32 Select(AZSOCKET sock, AZFD_SET* readfds, AZFD_SET* writefds, AZFD_SET* exceptfds, AZTIMEVAL* timeout);
        AZ::s32 IsRecvPending(AZSOCKET sock, AZTIMEVAL* timeout);
        AZ::s32 WaitForWritableSocket(AZSOCKET sock, AZTIMEVAL* timeout);

        //! Required to call startup before using most other socket calls
        //!  This really only is necessary on windows, but may be relevant for other platforms soon
        AZ::s32 Startup();

        //! Required to call cleanup exactly as many times as startup
        AZ::s32 Cleanup();

        AZ_FORCE_INLINE bool IsAzSocketValid(const AZSOCKET& socket)
        {
            return socket >= static_cast<AZ::s32>(AzSockError::eASE_NO_ERROR);
        }

        AZ_FORCE_INLINE bool SocketErrorOccured(AZ::s32 error)
        {
            return error < static_cast<AZ::s32>(AzSockError::eASE_NO_ERROR);
        }

        bool ResolveAddress(const AZStd::string& ip, AZ::u16 port, AZSOCKADDR_IN& sockAddr);

        class AzSocketAddress
        {
        public:
            AzSocketAddress();

            AzSocketAddress& operator=(const AZSOCKADDR& addr);

            bool operator==(const AzSocketAddress& rhs) const;
            AZ_FORCE_INLINE bool operator!=(const AzSocketAddress& rhs) const
            {
                return !(*this == rhs);
            }

            const AZSOCKADDR* GetTargetAddress() const;

            AZStd::string GetIP() const;
            AZStd::string GetAddress() const;

            AZ::u16 GetAddrPort() const;
            void SetAddrPort(AZ::u16 port);

            bool SetAddress(const AZStd::string& ip, AZ::u16 port);
            bool SetAddress(AZ::u32 ip, AZ::u16 port);

        protected:
            void Reset();

        private:
            AZSOCKADDR_IN m_sockAddr;
        };
    }
}
