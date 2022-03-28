/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Socket/AzSocket.h>

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define INVALID_SOCKET (-1)
#define closesocket(_s)         close(_s)
#define GetInternalSocketError  errno
using SOCKET = int;
using AZSOCKLEN = AZ::u32;

namespace AZ::AzSock
{
    AZ::s32 TranslateOSError(AZ::s32 oserror)
    {
        AZ::s32 error;

#define TRANSLATE(_from, _to) case (_from): error = static_cast<AZ::s32>(_to); break;

        switch (oserror)
        {
            TRANSLATE(0, AzSockError::eASE_NO_ERROR);
            TRANSLATE(EACCES, AzSockError::eASE_EACCES);
            TRANSLATE(EADDRINUSE, AzSockError::eASE_EADDRINUSE);
            TRANSLATE(EADDRNOTAVAIL, AzSockError::eASE_EADDRNOTAVAIL);
            TRANSLATE(EAFNOSUPPORT, AzSockError::eASE_EAFNOSUPPORT);
            TRANSLATE(EALREADY, AzSockError::eASE_EALREADY);
            TRANSLATE(EBADF, AzSockError::eASE_EBADF);
            TRANSLATE(ECONNABORTED, AzSockError::eASE_ECONNABORTED);
            TRANSLATE(ECONNREFUSED, AzSockError::eASE_ECONNREFUSED);
            TRANSLATE(ECONNRESET, AzSockError::eASE_ECONNRESET);
            TRANSLATE(EFAULT, AzSockError::eASE_EFAULT);
            TRANSLATE(EHOSTDOWN, AzSockError::eASE_EHOSTDOWN);
            TRANSLATE(EINPROGRESS, AzSockError::eASE_EINPROGRESS);
            TRANSLATE(EINTR, AzSockError::eASE_EINTR);
            TRANSLATE(EINVAL, AzSockError::eASE_EINVAL);
            TRANSLATE(EISCONN, AzSockError::eASE_EISCONN);
            TRANSLATE(EMFILE, AzSockError::eASE_EMFILE);
            TRANSLATE(EMSGSIZE, AzSockError::eASE_EMSGSIZE);
            TRANSLATE(ENETUNREACH, AzSockError::eASE_ENETUNREACH);
            TRANSLATE(ENOBUFS, AzSockError::eASE_ENOBUFS);
            TRANSLATE(ENOPROTOOPT, AzSockError::eASE_ENOPROTOOPT);
            TRANSLATE(ENOTCONN, AzSockError::eASE_ENOTCONN);
            TRANSLATE(EOPNOTSUPP, AzSockError::eASE_EOPNOTSUPP);
            TRANSLATE(EPROTONOSUPPORT, AzSockError::eASE_EAFNOSUPPORT);
            TRANSLATE(ETIMEDOUT, AzSockError::eASE_ETIMEDOUT);
            TRANSLATE(ETOOMANYREFS, AzSockError::eASE_ETOOMANYREFS);
            TRANSLATE(EWOULDBLOCK, AzSockError::eASE_EWOULDBLOCK);

        default:
            AZ_TracePrintf("AzSock", "AzSocket could not translate OS error code %x, treating as miscellaneous.\n", oserror);
            error = static_cast<AZ::s32>(AzSockError::eASE_MISC_ERROR);
            break;
        }

#undef TRANSLATE

        return error;
    }

    AZ::s32 TranslateSocketOption(AzSocketOption opt)
    {
        AZ::s32 value;

#define TRANSLATE(_from, _to) case (_from): value = (_to); break;

        switch (opt)
        {
            TRANSLATE(AzSocketOption::REUSEADDR, SO_REUSEADDR);
            TRANSLATE(AzSocketOption::KEEPALIVE, SO_KEEPALIVE);
            TRANSLATE(AzSocketOption::LINGER, SO_LINGER);

        default:
            AZ_TracePrintf("AzSock", "AzSocket option %x not yet supported", opt);
            value = 0;
            break;
        }

#undef TRANSLATE

        return value;
    }

    AZSOCKET HandleInvalidSocket(SOCKET sock)
    {
        AZSOCKET azsock = static_cast<AZSOCKET>(sock);
        if (sock == INVALID_SOCKET)
        {
            azsock = TranslateOSError(GetInternalSocketError);
        }
        return azsock;
    }

    AZ::s32 HandleSocketError(AZ::s32 socketError)
    {
        if (socketError == SOCKET_ERROR)
        {
            socketError = TranslateOSError(GetInternalSocketError);
        }
        return socketError;
    }

    const char* GetStringForError(AZ::s32 errorNumber)
    {
        AzSockError errorCode = AzSockError(errorNumber);

#define CASE_RETSTRING(errorEnum) case errorEnum: { return #errorEnum; }

        switch (errorCode)
        {
            CASE_RETSTRING(AzSockError::eASE_NO_ERROR);
            CASE_RETSTRING(AzSockError::eASE_SOCKET_INVALID);
            CASE_RETSTRING(AzSockError::eASE_EACCES);
            CASE_RETSTRING(AzSockError::eASE_EADDRINUSE);
            CASE_RETSTRING(AzSockError::eASE_EADDRNOTAVAIL);
            CASE_RETSTRING(AzSockError::eASE_EAFNOSUPPORT);
            CASE_RETSTRING(AzSockError::eASE_EALREADY);
            CASE_RETSTRING(AzSockError::eASE_EBADF);
            CASE_RETSTRING(AzSockError::eASE_ECONNABORTED);
            CASE_RETSTRING(AzSockError::eASE_ECONNREFUSED);
            CASE_RETSTRING(AzSockError::eASE_ECONNRESET);
            CASE_RETSTRING(AzSockError::eASE_EFAULT);
            CASE_RETSTRING(AzSockError::eASE_EHOSTDOWN);
            CASE_RETSTRING(AzSockError::eASE_EINPROGRESS);
            CASE_RETSTRING(AzSockError::eASE_EINTR);
            CASE_RETSTRING(AzSockError::eASE_EINVAL);
            CASE_RETSTRING(AzSockError::eASE_EISCONN);
            CASE_RETSTRING(AzSockError::eASE_EMFILE);
            CASE_RETSTRING(AzSockError::eASE_EMSGSIZE);
            CASE_RETSTRING(AzSockError::eASE_ENETUNREACH);
            CASE_RETSTRING(AzSockError::eASE_ENOBUFS);
            CASE_RETSTRING(AzSockError::eASE_ENOPROTOOPT);
            CASE_RETSTRING(AzSockError::eASE_ENOTCONN);
            CASE_RETSTRING(AzSockError::eASE_ENOTINITIALISED);
            CASE_RETSTRING(AzSockError::eASE_EOPNOTSUPP);
            CASE_RETSTRING(AzSockError::eASE_EPIPE);
            CASE_RETSTRING(AzSockError::eASE_EPROTONOSUPPORT);
            CASE_RETSTRING(AzSockError::eASE_ETIMEDOUT);
            CASE_RETSTRING(AzSockError::eASE_ETOOMANYREFS);
            CASE_RETSTRING(AzSockError::eASE_EWOULDBLOCK);
            CASE_RETSTRING(AzSockError::eASE_EWOULDBLOCK_CONN);
            CASE_RETSTRING(AzSockError::eASE_MISC_ERROR);
        }

#undef CASE_RETSTRING

        return "(invalid)";
    }

    AZ::u32 HostToNetLong(AZ::u32 hstLong)
    {
        return htonl(hstLong);
    }

    AZ::u32 NetToHostLong(AZ::u32 netLong)
    {
        return ntohl(netLong);
    }

    AZ::u16 HostToNetShort(AZ::u16 hstShort)
    {
        return htons(hstShort);
    }

    AZ::u16 NetToHostShort(AZ::u16 netShort)
    {
        return ntohs(netShort);
    }

    AZ::s32 GetHostName(AZStd::string& hostname)
    {
        AZ::s32 result = 0;
        hostname.clear();
        char name[256];
        result = HandleSocketError(gethostname(name, AZ_ARRAY_SIZE(name)));
        if (result == static_cast<AZ::s32>(AzSockError::eASE_NO_ERROR))
        {
            hostname = name;
        }
        return result;
    }

    AZSOCKET Socket()
    {
        return Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    }

    AZSOCKET Socket(AZ::s32 af, AZ::s32 type, AZ::s32 protocol)
    {
        return HandleInvalidSocket(socket(af, type, protocol));
    }

    AZ::s32 SetSockOpt(AZSOCKET sock, AZ::s32 level, AZ::s32 optname, const char* optval, AZ::s32 optlen)
    {
        AZSOCKLEN length(optlen);
        return HandleSocketError(setsockopt(sock, level, optname, optval, length));
    }

    AZ::s32 SetSocketOption(AZSOCKET sock, AzSocketOption opt, bool enable)
    {
        AZ::u32 val = enable ? 1 : 0;
        return SetSockOpt(sock, SOL_SOCKET, TranslateSocketOption(opt), reinterpret_cast<const char*>(&val), sizeof(val));
    }

    AZ::s32 EnableTCPNoDelay(AZSOCKET sock, bool enable)
    {
        AZ::u32 val = enable ? 1 : 0;
        return SetSockOpt(sock, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&val), sizeof(val));
    }

    AZ::s32 SetSocketBlockingMode(AZSOCKET sock, bool blocking)
    {
        AZ::s32 flags = ::fcntl(sock, F_GETFL);
        flags &= ~O_NONBLOCK;
        flags |= (blocking ? 0 : O_NONBLOCK);
        return ::fcntl(sock, F_SETFL, flags);
    }

    AZ::s32 CloseSocket(AZSOCKET sock)
    {
        return HandleSocketError(closesocket(sock));
    }

    AZ::s32 Shutdown(AZSOCKET sock, AZ::s32 how)
    {
        return HandleSocketError(shutdown(sock, how));
    }

    AZ::s32 GetSockName(AZSOCKET sock, AzSocketAddress& addr)
    {
        AZSOCKADDR sAddr;
        AZSOCKLEN sAddrLen = sizeof(AZSOCKADDR);
        memset(&sAddr, 0, sAddrLen);
        AZ::s32 result = HandleSocketError(getsockname(sock, &sAddr, &sAddrLen));
        addr = sAddr;
        return result;
    }

    AZ::s32 Connect(AZSOCKET sock, const AzSocketAddress& addr)
    {
        AZ::s32 err = HandleSocketError(connect(sock, addr.GetTargetAddress(), sizeof(AZSOCKADDR_IN)));
        if (err == static_cast<AZ::s32>(AzSockError::eASE_EINPROGRESS))
        {
            err = static_cast<AZ::s32>(AzSockError::eASE_EWOULDBLOCK_CONN);
        }
        return err;
    }

    AZ::s32 Listen(AZSOCKET sock, AZ::s32 backlog)
    {
        return HandleSocketError(listen(sock, backlog));
    }

    AZSOCKET Accept(AZSOCKET sock, AzSocketAddress& addr)
    {
        AZSOCKADDR sAddr;
        AZSOCKLEN sAddrLen = sizeof(AZSOCKADDR);
        memset(&sAddr, 0, sAddrLen);
        AZSOCKET outSock = HandleInvalidSocket(accept(sock, &sAddr, &sAddrLen));
        addr = sAddr;
        return outSock;
    }

    AZ::s32 Send(AZSOCKET sock, const char* buf, AZ::s32 len, AZ::s32 flags)
    {
        AZ::s32 msgNoSignal = MSG_NOSIGNAL;
        return HandleSocketError(send(sock, buf, len, flags | msgNoSignal));
    }

    AZ::s32 Recv(AZSOCKET sock, char* buf, AZ::s32 len, AZ::s32 flags)
    {
        return HandleSocketError(recv(sock, buf, len, flags));
    }

    AZ::s32 Bind(AZSOCKET sock, const AzSocketAddress& addr)
    {
        return HandleSocketError(bind(sock, addr.GetTargetAddress(), sizeof(AZSOCKADDR_IN)));
    }

    AZ::s32 Select(AZSOCKET sock, AZFD_SET* readfdsock, AZFD_SET* writefdsock, AZFD_SET* exceptfdsock, AZTIMEVAL* timeout)
    {
        return HandleSocketError(::select(sock + 1, readfdsock, writefdsock, exceptfdsock, timeout));
    }

    AZ::s32 IsRecvPending(AZSOCKET sock, AZTIMEVAL* timeout)
    {
        AZFD_SET readSet;
        FD_ZERO(&readSet);
        FD_SET(sock, &readSet);

        AZ::s32 ret = Select(sock, &readSet, nullptr, nullptr, timeout);
        if (ret >= 0)
        {
            ret = FD_ISSET(sock, &readSet);
            if (ret != 0)
            {
                ret = 1;
            }
        }

        return ret;
    }

    AZ::s32 WaitForWritableSocket(AZSOCKET sock, AZTIMEVAL* timeout)
    {
        AZFD_SET writeSet;
        FD_ZERO(&writeSet);
        FD_SET(sock, &writeSet);

        AZ::s32 ret = Select(sock, nullptr, &writeSet, nullptr, timeout);
        if (ret >= 0)
        {
            ret = FD_ISSET(sock, &writeSet);
            if (ret != 0)
            {
                ret = 1;
            }
        }

        return ret;
    }

    AZ::s32 Startup()
    {
        return static_cast<AZ::s32>(AzSockError::eASE_NO_ERROR);
    }

    AZ::s32 Cleanup()
    {
        return static_cast<AZ::s32>(AzSockError::eASE_NO_ERROR);
    }

    bool ResolveAddress(const AZStd::string& ip, AZ::u16 port, AZSOCKADDR_IN& socketAddress)
    {
        bool foundAddr = false;
        addrinfo hints;
        memset(&hints, 0, sizeof(addrinfo));
        addrinfo* addrInfo;
        hints.ai_family = AF_INET;
        hints.ai_flags = AI_CANONNAME;
        char strPort[8];
        azsnprintf(strPort, AZ_ARRAY_SIZE(strPort), "%d", port);

        const char* address = ip.c_str();
        if (address && strlen(address) == 0) // getaddrinfo doesn't accept empty string
        {
            address = nullptr;
        }

        AZ::s32 err = HandleSocketError(getaddrinfo(address, strPort, &hints, &addrInfo));
        if (err == 0) // eASE_NO_ERROR
        {
            if (addrInfo->ai_family == AF_INET)
            {
                socketAddress = *reinterpret_cast<const AZSOCKADDR_IN*>(addrInfo->ai_addr);
                foundAddr = true;
            }

            freeaddrinfo(addrInfo);
        }
        else
        {
            AZ_Assert(false, "AzSocketAddress could not resolve address %s with port %d. (reason - %s)", ip.c_str(), port, GetStringForError(err));
        }
        return foundAddr;
    }
} // namespace AZ::AzSock
