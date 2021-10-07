/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GridMate/Carrier/SocketDriver.h>
#include <GridMate/Carrier/Utils.h>
#include <AzCore/Socket/AzSocket.h>

/**
 * Enable this define if you want to print all packets and addresses
 * to unbound connections. Unbound convention is when you don't have the full
 * address to receive or send data.
 */
//#define AZ_LOG_UNBOUND_SEND_RECEIVE

#include <GridMate/Containers/unordered_set.h>
#include <GridMate/Carrier/DriverEvents.h>

#include <AzCore/std/chrono/types.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/string/memorytoascii.h>

#include <AzCore/std/bind/bind.h>

namespace GridMate
{
    namespace Platform
    {
        bool IsValidSocket(GridMate::SocketDriverCommon::SocketType s);
        bool IsSocketError(AZ::s64 result);
        GridMate::SocketDriverCommon::SocketType GetInvalidSocket();
        int GetSocketError();
        timeval GetTimeValue(AZStd::chrono::microseconds timeOut);
        const char* GetSocketErrorString(int error, SocketErrorBuffer& array);
        GridMate::Driver::ResultCode SetSocketBlockingMode(GridMate::SocketDriverCommon::SocketType sock, bool blocking);
        GridMate::Driver::ResultCode SetFastSocketClose(GridMate::SocketDriverCommon::SocketType socket, bool isDatagram);
        void PrepareFamilyType(int ft, bool& isIpv6);
        int Bind(GridMate::SocketDriverCommon::SocketType socket, const sockaddr* addr, size_t addrlen);
        int GetAddressInfo(const char* node, const char* service, const struct addrinfo* hints, struct addrinfo** res);
        void FreeAddressInfo(struct addrinfo* res);
    }
}

#define AZ_SOCKET_WAKEUP_MSG_TYPE   char
#define AZ_SOCKET_WAKEUP_MSG_VALUE  'G'

namespace GridMate
{
    namespace SocketOperations
    {
        //=========================================================================
        // HostToNetLong
        // [3/23/2017]
        //=========================================================================
        AZ::u32 HostToNetLong(AZ::u32 hstLong)
        {
            return htonl(hstLong);
        }

        //=========================================================================
        // NetToHostLong
        // [3/23/2017]
        //=========================================================================
        AZ::u32 NetToHostLong(AZ::u32 netLong)
        {
            return ntohl(netLong);
        }

        //=========================================================================
        // HostToNetShort
        // [3/23/2017]
        //=========================================================================
        AZ::u16 HostToNetShort(AZ::u16 hstShort)
        {
            return htons(hstShort);
        }

        //=========================================================================
        // NetToHostShort
        // [3/23/2017]
        //=========================================================================
        AZ::u16 NetToHostShort(AZ::u16 netShort)
        {
            return ntohs(netShort);
        }

        //=========================================================================
        // CreateSocket
        // [4/4/2017]
        //=========================================================================
        SocketDriverCommon::SocketType CreateSocket(bool isDatagram, Driver::BSDSocketFamilyType familyType)
        {
            int addressFamily = (familyType == Driver::BSDSocketFamilyType::BSD_AF_INET6) ? AF_INET6 : AF_INET;
            int socketType = isDatagram ? SOCK_DGRAM : SOCK_STREAM;
            int protocol = isDatagram ? IPPROTO_UDP : IPPROTO_TCP;
            return socket(addressFamily, socketType, protocol);
        }

        //=========================================================================
        // SetSocketOptionValue
        // [3/23/2017]
        //=========================================================================
        Driver::ResultCode SetSocketOptionValue(SocketDriverCommon::SocketType sock, SocketOption option, const char* optval, AZ::s32 optlen)
        {
            AZ::s32 optionName;
            switch (option)
            {
            case SocketOption::NonBlockingIO:
                optionName = SO_NBIO;
                break;
            case SocketOption::ReuseAddress:
                optionName = SO_REUSEADDR;
                break;
            case SocketOption::KeepAlive:
                optionName = SO_KEEPALIVE;
                break;
            case SocketOption::Broadcast:
                optionName = SO_BROADCAST;
                break;
            case SocketOption::SendBuffer:
                optionName = SO_SNDBUF;
                break;
            case SocketOption::ReceiveBuffer:
                optionName = SO_RCVBUF;
                break;
            default:
                optionName = -1;
                AZ_Error("GridMate", false, "Unsupported socket option: %d with error:%d", option, Platform::GetSocketError());
                return Driver::EC_SOCKET_SOCK_OPT;
            }
            AZ::s64 sockResult = setsockopt(sock, SOL_SOCKET, optionName, optval, optlen);
            AZ_Error("GridMate", sockResult == 0, "Socket option: %d failed to set. Error:%d", option, Platform::GetSocketError());
            return (sockResult == 0) ? Driver::EC_OK : Driver::EC_SOCKET_SOCK_OPT;
        }

        //=========================================================================
        // SetSocketOptionBoolean
        // [3/23/2017]
        //=========================================================================
        Driver::ResultCode SetSocketOptionBoolean(SocketDriverCommon::SocketType sock, SocketOption option, bool enable)
        {
            bool value = enable;
            return SetSocketOptionValue(sock, option, reinterpret_cast<const char*>(value), sizeof(value));
        }

        //=========================================================================
        // EnableTCPNoDelay
        // [3/23/2017]
        //=========================================================================
        Driver::ResultCode EnableTCPNoDelay(SocketDriverCommon::SocketType sock, bool enable)
        {
            AZ::u32 val = enable ? 1 : 0;
            if (Platform::IsSocketError(setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&val), sizeof(val))))
            {
                return Driver::EC_SOCKET_SOCK_OPT;
            }
            return Driver::EC_OK;
        }

        //=========================================================================
        // SetSocketBlockingMode
        // [3/23/2017]
        //=========================================================================
        Driver::ResultCode SetSocketBlockingMode(SocketDriverCommon::SocketType sock, bool blocking)
        {
            return Platform::SetSocketBlockingMode(sock, blocking);
        }

        //=========================================================================
        // SetSocketLingerTime
        // [5/4/2017]
        //=========================================================================
        GridMate::Driver::ResultCode SetSocketLingerTime(SocketDriverCommon::SocketType sock, bool bDoLinger, AZ::u16 timeout)
        {
            // Indicates the state of the linger structure associated with a socket. If the l_onoff member of the linger
            // structure is nonzero, a socket remains open for a specified amount of time after a closesocket function call to
            // enable queued data to be sent. The amount of time, in seconds, to remain open is specified in the l_linger member
            // of the linger structure. This option is only valid for reliable, connection - oriented protocols.
            struct linger theLinger;
            theLinger.l_linger = static_cast<decltype(theLinger.l_linger)>(timeout);
            theLinger.l_onoff = static_cast<decltype(theLinger.l_onoff)>(bDoLinger);
            AZ::s64 sockResult = setsockopt(sock, SOL_SOCKET, SO_LINGER, reinterpret_cast<char*>(&theLinger), sizeof(theLinger));
            AZ_Error("GridMate", sockResult == 0, "Socket option: %d failed to set. Error:%d", SO_LINGER, Platform::GetSocketError());
            return (sockResult == 0) ? Driver::EC_OK : Driver::EC_SOCKET_SOCK_OPT;
        }

        //=========================================================================
        // CloseSocket
        // [3/23/2017]
        //=========================================================================
        Driver::ResultCode CloseSocket(SocketDriverCommon::SocketType sock)
        {
            if (Platform::IsValidSocket(sock))
            {
                if (!Platform::IsSocketError(closesocket(sock)))
                {
                    return Driver::EC_OK;
                }
            }
            return Driver::EC_SOCKET_CLOSE;
        }

        //=========================================================================
        // Send
        // [3/23/2017]
        //=========================================================================
        Driver::ResultCode Send(SocketDriverCommon::SocketType sock, const char* buf, AZ::u32 bufLen, AZ::u32& bytesSent)
        {
            bytesSent = 0;
            if (bufLen == 0)
            {
                // is an empty buffer?
                return Driver::EC_SEND;
            }
            else if ((0x80000000 & bufLen) != 0)
            {
                // is negative?
                return Driver::EC_SEND;
            }
            AZ::s32 result = send(sock, buf, static_cast<AZ::u32>(bufLen), AZ_TRAIT_GRIDMATE_MESSAGE_NO_SIGNAL);
            if (Platform::IsSocketError(result))
            {
                AZ::s32 err = Platform::GetSocketError();
                if (err != AZ_EWOULDBLOCK)
                {
                    AZ_TracePrintf("GridMate", "send() err:%d -> %s\n", err, AZ::AzSock::GetStringForError(err));
                    return Driver::EC_SEND;
                }
            }
            else
            {
                bytesSent = result;
            }
            return Driver::EC_OK;
        }

        //=========================================================================
        // Receive
        // [3/23/2017]
        //=========================================================================
        Driver::ResultCode Receive(SocketDriverCommon::SocketType sock, char* buf, AZ::u32& inOutlen)
        {
            if (inOutlen == 0)
            {
                // is an empty buffer?
                return Driver::EC_RECEIVE;
            }
            else if ((0x80000000 & inOutlen) != 0)
            {
                // is negative?
                return Driver::EC_RECEIVE;
            }

            AZ::s32 result = recv(sock, buf, static_cast<AZ::u32>(inOutlen), 0);
            if (Platform::IsSocketError(result))
            {
                inOutlen = 0;
                AZ::s32 err = Platform::GetSocketError();
                if (err != AZ_EWOULDBLOCK)
                {
                    AZ_TracePrintf("GridMate", "recv() err:%d -> %s\n", err, AZ::AzSock::GetStringForError(err));
                    return Driver::EC_RECEIVE;
                }
            }
            inOutlen = result;
            return Driver::EC_OK;
        }

        //=========================================================================
        // Bind
        // [3/23/2017]
        //=========================================================================
        Driver::ResultCode Bind(SocketDriverCommon::SocketType sock, const sockaddr* sockAddr, size_t sockAddrSize)
        {
            AZ::s32 ret = Platform::Bind(sock, sockAddr, sockAddrSize);
            if (Platform::IsSocketError(ret))
            {
                return Driver::EC_SOCKET_BIND;
            }
            return Driver::EC_OK;
        }

        //=========================================================================
        // Connect
        // [3/23/2017]
        //=========================================================================
        GridMate::Driver::ResultCode Connect(SocketDriverCommon::SocketType sock, const sockaddr* socketAddress, size_t sockAddrSize, ConnectionResult& outConnectionResult)
        {
            socklen_t addressSize = static_cast<socklen_t>(sockAddrSize);
            AZ::s64 err = connect(sock, socketAddress, addressSize);
            if (!Platform::IsSocketError(err))
            {
                outConnectionResult = ConnectionResult::Okay;
                return Driver::EC_OK;
            }
            else
            {
                // okay for non-blocking sockets... will take a while
                AZ::s64 extendedErr = Platform::GetSocketError();
                if (extendedErr == static_cast<AZ::s32>(AZ_EWOULDBLOCK))
                {
                    outConnectionResult = ConnectionResult::InProgress;
                    return Driver::EC_OK;
                }
                else if (extendedErr == static_cast<AZ::s32>(AZ_EINPROGRESS))
                {
                    outConnectionResult = ConnectionResult::InProgress;
                    return Driver::EC_OK;
                }
                else if (extendedErr == static_cast<AZ::s32>(AZ_EALREADY))
                {
                    outConnectionResult = ConnectionResult::InProgress;
                    return Driver::EC_OK;
                }
                else if (extendedErr == static_cast<AZ::s32>(AZ_ECONNREFUSED))
                {
                    outConnectionResult = ConnectionResult::Refused;
                }
                else if (extendedErr == static_cast<AZ::s32>(AZ_EISCONN))
                {
                    outConnectionResult = ConnectionResult::SocketConnected;
                }
                else if (extendedErr == static_cast<AZ::s32>(AZ_ENETUNREACH))
                {
                    outConnectionResult = ConnectionResult::NetworkUnreachable;
                }
                else if (extendedErr == static_cast<AZ::s32>(AZ_ETIMEDOUT))
                {
                    outConnectionResult = ConnectionResult::TimedOut;
                }
            }
            AZ_TracePrintf("GridMate", "Connect() error:%d\n", Platform::GetSocketError());
            return Driver::EC_SOCKET_CONNECT;
        }

        //=========================================================================
        // Connect
        // [3/23/2017]
        //=========================================================================
        Driver::ResultCode Connect(SocketDriverCommon::SocketType sock, const SocketDriverAddress& addr, ConnectionResult& outConnectionResult)
        {
            AZ::u32 addressSize;
            const sockaddr* socketAddress = reinterpret_cast<const sockaddr*>(addr.GetTargetAddress(addressSize));
            if (socketAddress == nullptr)
            {
                return Driver::EC_SOCKET_CONNECT;
            }
            return Connect(sock, socketAddress, addressSize, outConnectionResult);
        }

        //=========================================================================
        // Listen
        // [3/23/2017]
        //=========================================================================
        Driver::ResultCode Listen(SocketDriverCommon::SocketType sock, AZ::s32 backlog)
        {
            if (Platform::IsSocketError(listen(sock, backlog)))
            {
                return Driver::EC_SOCKET_LISTEN;
            }
            return Driver::EC_OK;
        }

        //=========================================================================
        // Accept
        // [3/23/2017]
        //=========================================================================
        Driver::ResultCode Accept(SocketDriverCommon::SocketType sock, sockaddr* outAddr, socklen_t& outAddrSize, SocketDriverCommon::SocketType& outSocket)
        {
            if (outAddrSize < static_cast<socklen_t>(sizeof(sockaddr_in)))
            {
                return Driver::EC_SOCKET_ACCEPT;
            }
            memset(outAddr, 0, outAddrSize);

            outSocket = accept(sock, outAddr, &outAddrSize);
            if (!Platform::IsValidSocket(outSocket))
            {
                // okay for non-blocking sockets... will take a while
                AZ::s64 extendedErr = Platform::GetSocketError();
                if (extendedErr == static_cast<AZ::s32>(AZ_EWOULDBLOCK))
                {
                    return Driver::EC_OK;
                }
                else if (extendedErr == static_cast<AZ::s32>(AZ_EINPROGRESS))
                {
                    return Driver::EC_OK;
                }
                return Driver::EC_SOCKET_ACCEPT;
            }
            return Driver::EC_OK;
        }

        //=========================================================================
        // GetTimeValue
        // [3/23/2017]
        //=========================================================================
        timeval GetTimeValue(AZStd::chrono::microseconds timeOut)
        {
            return Platform::GetTimeValue(timeOut);
        }

        //=========================================================================
        // IsWritable
        // [3/23/2017]
        //=========================================================================
        bool IsWritable(SocketDriverCommon::SocketType sock, AZStd::chrono::microseconds timeOut)
        {
            fd_set fdwrite;
            FD_ZERO(&fdwrite);
            FD_SET(sock, &fdwrite);
            timeval t = GetTimeValue(timeOut);
            int result = select(FD_SETSIZE, nullptr, &fdwrite, nullptr, &t);
            if (result > 0)
            {
                return true;
            }
            AZ_Warning("GridMate", result == 0, "Socket:%d select error %d\n", sock, Platform::GetSocketError());
            return false;
        }

        //=========================================================================
        // IsReceivePending
        // [3/23/2017]
        //=========================================================================
        bool IsReceivePending(SocketDriverCommon::SocketType sock, AZStd::chrono::microseconds timeOut)
        {
            fd_set fdread;
            FD_ZERO(&fdread);
            FD_SET(sock, &fdread);
            timeval t = GetTimeValue(timeOut);
            int result = select(FD_SETSIZE, &fdread, nullptr, nullptr, &t);
            if (result > 0)
            {
                return true;
            }
            AZ_Warning("GridMate", result == 0, "Socket:%d select error %d\n", sock, Platform::GetSocketError());
            return false;
        }
    }

    AZStd::size_t SocketDriverAddress::Hasher::operator()(const SocketDriverAddress& v) const
    {
        size_t hash = 0;
        switch (v.m_sockAddr.sin_family)
        {
            case AF_INET:
            {
                hash = v.m_sockAddr.sin_addr.s_addr ^ v.m_sockAddr.sin_port;
                break;
            }
            case AF_INET6:
            {
                union
                {
                    const void* m_voidPtr;
                    const size_t* m_hashPtr;
                } u;
                u.m_voidPtr = static_cast<const void*>(v.m_sockAddr6.sin6_addr.s6_addr);
                hash = *u.m_hashPtr ^ v.m_sockAddr6.sin6_port;
                break;
            }
        }
        return hash;
    }

    SocketDriverAddress::SocketDriverAddress()
        : DriverAddress(nullptr)
    {
        m_sockAddr.sin_family = AF_UNSPEC;
        m_sockAddr.sin_port = 0;
        m_sockAddr.sin_addr.s_addr = 0;
    }

    SocketDriverAddress::SocketDriverAddress(Driver* driver)
        : DriverAddress(driver)
    {
        m_sockAddr.sin_family = AF_UNSPEC;
        m_sockAddr.sin_port = 0;
        m_sockAddr.sin_addr.s_addr = 0;
    }

    SocketDriverAddress::SocketDriverAddress(Driver* driver, const sockaddr* addr)
        : DriverAddress(driver)
    {
        if (addr->sa_family == AF_INET6)
        {
            m_sockAddr6 = *reinterpret_cast<const sockaddr_in6*>(addr);
        }
        else
        {
            m_sockAddr = *reinterpret_cast<const sockaddr_in*>(addr);
        }
    }

    SocketDriverAddress::SocketDriverAddress(Driver* driver, const AZStd::string& ip, unsigned int port)
        : DriverAddress(driver)
    {
        AZ_Assert(!ip.empty(), "Invalid address string!");
        // resolve address
        {
            addrinfo  hints;
            memset(&hints, 0, sizeof(struct addrinfo));
            addrinfo* addrInfo;
            hints.ai_family = AF_UNSPEC;
            hints.ai_flags = AI_CANONNAME;
            char strPort[8];
            azsnprintf(strPort, AZ_ARRAY_SIZE(strPort), "%d", port); // TODO pass port as a string to avoid this call

            const char* address = ip.c_str();

            if (address && strlen(address) == 0) // Platform::GetAddressInfo() doesn't accept empty string
            {
                address = nullptr;
            }

            int error = Platform::GetAddressInfo(address, strPort, &hints, &addrInfo);
            if (error == 0)
            {
                if (addrInfo->ai_family == AF_INET)
                {
                    m_sockAddr = *reinterpret_cast<const sockaddr_in*>(addrInfo->ai_addr);
                }
                else if (addrInfo->ai_family == AF_INET6)
                {
                    m_sockAddr6 = *reinterpret_cast<const sockaddr_in6*>(addrInfo->ai_addr);
                }

                Platform::FreeAddressInfo(addrInfo);
            }
            else
            {
                SocketErrorBuffer buffer;
                AZ_UNUSED(buffer);
                AZ_Assert(false, "SocketDriver::ResolveAddress failed with error %s!", Platform::GetSocketErrorString(Platform::GetSocketError(), buffer));
            }
        }
    }

    bool SocketDriverAddress::operator==(const SocketDriverAddress& rhs) const
    {
        if (m_sockAddr.sin_family != rhs.m_sockAddr.sin_family)
        {
            return false;
        }

        if (m_sockAddr.sin_family == AF_INET6)
        {
            if (m_sockAddr6.sin6_port != rhs.m_sockAddr6.sin6_port)
            {
                return false;
            }

            const AZ::u64* addr = reinterpret_cast<const AZ::u64*>(m_sockAddr6.sin6_addr.s6_addr);
            const AZ::u64* rhsAddr = reinterpret_cast<const AZ::u64*>(rhs.m_sockAddr6.sin6_addr.s6_addr);
            if (addr[0] != rhsAddr[0] || addr[1] != rhsAddr[1])
            {
                return false;
            }
        }
        else
        {
            return m_sockAddr.sin_addr.s_addr == rhs.m_sockAddr.sin_addr.s_addr && m_sockAddr.sin_port == rhs.m_sockAddr.sin_port;
        }
        return true;
    }

    bool SocketDriverAddress::operator!=(const SocketDriverAddress& rhs) const
    {
        return !(*this == rhs);
    }

    AZStd::string SocketDriverAddress::ToString() const
    {
        char ip[64];
        unsigned short port;
        if (m_sockAddr.sin_family == AF_INET6)
        {
            inet_ntop(AF_INET6, const_cast<in6_addr*>(&m_sockAddr6.sin6_addr), ip, AZ_ARRAY_SIZE(ip));
            port = ntohs(m_sockAddr6.sin6_port);
        }
        else
        {
            inet_ntop(AF_INET, const_cast<in_addr*>(&m_sockAddr.sin_addr), ip, AZ_ARRAY_SIZE(ip));
            port = ntohs(m_sockAddr.sin_port);
        }

        return AZStd::string::format("%s|%d", ip, port);
    }

    AZStd::string SocketDriverAddress::ToAddress() const
    {
        return ToString();
    }

    AZStd::string SocketDriverAddress::GetIP() const
    {
        char ip[64];
        if (m_sockAddr.sin_family == AF_INET6)
        {
            inet_ntop(AF_INET6, const_cast<void*>(reinterpret_cast<const void*>(&m_sockAddr6.sin6_addr)), ip, AZ_ARRAY_SIZE(ip));
        }
        else
        {
            inet_ntop(AF_INET, const_cast<void*>(reinterpret_cast<const void*>(&m_sockAddr.sin_addr)), ip, AZ_ARRAY_SIZE(ip));
        }
        return AZStd::string(ip);
    }

    unsigned int SocketDriverAddress::GetPort() const
    {
        if (m_sockAddr.sin_family == AF_INET6)
        {
            return ntohs(m_sockAddr6.sin6_port);
        }
        else
        {
            return ntohs(m_sockAddr.sin_port);
        }
    }

    const void* SocketDriverAddress::GetTargetAddress(unsigned int& addressSize) const
    {
        if (m_sockAddr.sin_family == AF_INET6)
        {
            addressSize = sizeof(m_sockAddr6);
        }
        else
        {
            addressSize = sizeof(m_sockAddr);
        }
        return &m_sockAddr;
    }

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    // SocketAddressInfo
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////

    SocketAddressInfo::SocketAddressInfo()
        : m_addrInfo(nullptr)
    {
    }

    SocketAddressInfo::~SocketAddressInfo()
    {
        Reset();
    }

    void SocketAddressInfo::Reset()
    {
        if (m_addrInfo != nullptr)
        {
            Platform::FreeAddressInfo(m_addrInfo);
            m_addrInfo = nullptr;
        }
    }
    bool SocketAddressInfo::Resolve(char const* address, AZ::u16 port, Driver::BSDSocketFamilyType familyType, bool isDatagram, AdditionalOptionFlags flags)
    {
        AZ_Assert(familyType == Driver::BSDSocketFamilyType::BSD_AF_INET || familyType == Driver::BSDSocketFamilyType::BSD_AF_INET6, "Family type (familyType) can be IPV4 or IPV6 only!");
        Reset();

        char portStr[8];
        azsnprintf(portStr, AZ_ARRAY_SIZE(portStr), "%d", port);

        addrinfo  hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = (familyType == Driver::BSDSocketFamilyType::BSD_AF_INET6) ? AF_INET6 : AF_INET;
        hints.ai_socktype = isDatagram ? SOCK_DGRAM : SOCK_STREAM;
        hints.ai_flags = 0;

        AZ::u32 dwFlags = static_cast<AZ::u32>(flags);
        if ((dwFlags & static_cast<AZ::u32>(AdditionalOptionFlags::Passive)) == static_cast<AZ::u32>(AdditionalOptionFlags::Passive))
        {
            hints.ai_flags |= AI_PASSIVE;
        }
        if ((dwFlags & static_cast<AZ::u32>(AdditionalOptionFlags::NumericHost)) == static_cast<AZ::u32>(AdditionalOptionFlags::NumericHost))
        {
            hints.ai_flags |= AI_NUMERICHOST;
        }

        if (address && strlen(address) == 0)
        {
            address = nullptr;
        }

        int error = Platform::GetAddressInfo(address, portStr, &hints, &m_addrInfo);
        if (error != 0)
        {
            SocketErrorBuffer buffer;
            AZ_UNUSED(buffer);
            AZ_TracePrintf("GridMate", "SocketDriver::Initialize - Platform::GetAddressInfo() failed with code %s at port %d\n", Platform::GetSocketErrorString(Platform::GetSocketError(), buffer), port);
            return false;
        }
        return true;
    }

    AZ::u16 SocketAddressInfo::RetrieveSystemAssignedPort(SocketDriverCommon::SocketType socket) const
    {
        if (m_addrInfo == nullptr)
        {
            return 0;
        }
        socklen_t addrLen = static_cast<socklen_t>(m_addrInfo->ai_addrlen);
        if (getsockname(socket, m_addrInfo->ai_addr, &addrLen) == 0)
        {
            if (addrLen == sizeof(sockaddr_in6))
            {
                return reinterpret_cast<sockaddr_in6*>(m_addrInfo->ai_addr)->sin6_port;
            }
            else
            {
                return reinterpret_cast<sockaddr_in*>(m_addrInfo->ai_addr)->sin_port;
            }
        }
        return 0;
    }

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    // SocketDriverCommon
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////

    //=========================================================================
    // SocketDriverCommon
    // [10/14/2010]
    //=========================================================================
    SocketDriverCommon::SocketDriverCommon(bool isFullPackets, bool isCrossPlatform, bool isHighPerformance)
        : m_isFullPackets(isFullPackets)
        , m_isCrossPlatform(isCrossPlatform)
        , m_isIpv6(false)
        , m_isDatagram(true)
        , m_isHighPerformance(isHighPerformance)
    {
        m_port = 0;
        m_isStoppedWaitForData = false;
        m_socket = Platform::GetInvalidSocket();

#ifdef AZ_SOCKET_RIO_SUPPORT
        if (isHighPerformance && RIOPlatformSocketDriver::isSupported())
        {
            m_platformDriver = AZStd::make_unique<RIOPlatformSocketDriver>(*this, m_socket);
        }
        else
        {
            m_platformDriver = AZStd::make_unique<PlatformSocketDriver>(*this, m_socket);
        }
#else
        m_platformDriver = AZStd::make_unique<PlatformSocketDriver>(*this, m_socket);
#endif

    }

    //=========================================================================
    // ~SocketDriverCommon
    // [10/14/2010]
    //=========================================================================
    SocketDriverCommon::~SocketDriverCommon()
    {
    }

    //=========================================================================
    // GetMaxSendSize
    // [1/7/2011]
    //=========================================================================
    unsigned int
    SocketDriverCommon::GetMaxSendSize() const
    {
        unsigned int maxPacketSize = AZ_TRAIT_GRIDMATE_MAX_PACKET_SEND_SIZE;
        if(m_isCrossPlatform)
        {
            maxPacketSize = 1264;           // an obsolete platform has the lowest
        }
        else if (m_isFullPackets)
        {
            maxPacketSize = 65507;
        }
        return maxPacketSize - GetPacketOverheadSize();
    }

    //=========================================================================
    // GetMaxSendSize
    // [1/7/2011]
    //=========================================================================
    unsigned int
    SocketDriverCommon::GetPacketOverheadSize() const
    {
            return 8 /* standard UDP*/ + 20 /* min for IPv4 */;
    }

    //=========================================================================
    // CreateSocket
    // [2/25/2011]
    //=========================================================================
    SocketDriverCommon::SocketType
    SocketDriverCommon::CreateSocket(int af, int type, int protocol)
    {
        return m_platformDriver->CreateSocket(af, type, protocol);
    }

    //=========================================================================
    // BindSocket
    // [2/25/2011]
    //=========================================================================
    int
    SocketDriverCommon::BindSocket(const sockaddr* sockAddr, size_t sockAddrSize)
    {
        return Platform::Bind(m_socket, sockAddr, sockAddrSize);
    }

    //=========================================================================
    // SetSocketOptions
    // [2/25/2011]
    //=========================================================================
    Driver::ResultCode
    SocketDriverCommon::SetSocketOptions(bool isBroadcast, unsigned int receiveBufferSize, unsigned int sendBufferSize)
    {
        int sock_opt, sock_opt2 = 0;
        socklen_t size = sizeof(sock_opt2);

        int sockResult = 0;
        (void)sockResult;
#if defined(AZ_SOCKET_RIO_SUPPORT)
        if(! (m_isHighPerformance && RIOPlatformSocketDriver::isSupported()) )
        {
#endif
        // set nonblocking
        unsigned long sock_ctrl = 1;
        if (Platform::IsSocketError(ioctlsocket(m_socket, SO_NBIO, &sock_ctrl)))
        {
            closesocket(m_socket);
            int error = Platform::GetSocketError();
            (void)error;
            AZ_TracePrintf("GridMate", "SocketDriver::Initialize - ioctlsocket failed with code %d\n", error);
            return EC_SOCKET_MAKE_NONBLOCK;
        }
#if defined(AZ_SOCKET_RIO_SUPPORT)
        }
#endif

        // receive buffer size
        sock_opt = receiveBufferSize == 0 ? (1024 * 256) : receiveBufferSize;
        sockResult = setsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<char*>(&sock_opt), sizeof(sock_opt));
        AZ_Error("GridMate", sockResult == 0, "Failed to set receive buffer to %d size. Error: %d", sock_opt, Platform::GetSocketError());
        sockResult = getsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<char*>(&sock_opt2), &size);
        AZ_Error("GridMate", sockResult == 0, "Failed to get receive buffer to size. Error: %d", Platform::GetSocketError());
        AZ_Error("GridMate", sock_opt <= sock_opt2, "Failed to set receive buffer to %d size actual %d.", sock_opt, sock_opt2);

        // send buffer size
        sock_opt = sendBufferSize == 0 ? (1024 * 64) : sendBufferSize;
        sockResult = setsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<char*>(&sock_opt), sizeof(sock_opt));
        AZ_Error("GridMate", sockResult == 0, "Failed to set send buffer to %d size. Error: %d", sock_opt, Platform::GetSocketError());
        sockResult = getsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<char*>(&sock_opt2), &size);
        AZ_Error("GridMate", sockResult == 0, "Failed to get send buffer to size. Error: %d", Platform::GetSocketError());
        AZ_Error("GridMate", sock_opt <= sock_opt2, "Failed to set send buffer to %d size actual %d.", sock_opt, sock_opt2);

        // make sure we allow both ipv4 and ipv6 (we can make this optional)
        if (m_isIpv6)
        {
            sock_opt = 0;
            sockResult = setsockopt(m_socket, IPPROTO_IPV6, IPV6_V6ONLY, reinterpret_cast<char*>(&sock_opt), sizeof(sock_opt));
            AZ_Error("GridMate", sockResult == 0, "Failed to stop using ipv6 only. Error: %d", Platform::GetSocketError());

    #if AZ_TRAIT_OS_ALLOW_MULTICAST
            // we emulate broadcast support over ipv6 (todo enable multicast support with an address too)
            addrinfo hints;
            memset(&hints, 0, sizeof(hints));
            addrinfo* multicastInfo;
            hints.ai_family = AF_INET6;
            hints.ai_flags = AI_NUMERICHOST;
            sockResult = Platform::GetAddressInfo(Utils::GetBroadcastAddress(BSD_AF_INET6), nullptr, &hints, &multicastInfo);
            AZ_Error("GridMate", sockResult == 0, "Platform::GetAddressInfo() failed to get broadcast address. Error: %d", Platform::GetSocketError());

            ipv6_mreq multicastRequest;
            multicastRequest.ipv6mr_interface = 0;
            memcpy(&multicastRequest.ipv6mr_multiaddr, &reinterpret_cast<sockaddr_in6*>(multicastInfo->ai_addr)->sin6_addr, sizeof(multicastRequest.ipv6mr_multiaddr));

            Platform::FreeAddressInfo(multicastInfo);

            if (m_isDatagram)
            {
                sockResult = setsockopt(m_socket, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, reinterpret_cast<char*>(&multicastRequest), sizeof(multicastRequest));

                AZ_Error("GridMate", sockResult == 0, "Failed to IPV6_ADD_MEMBERSHIP. Error: %d", Platform::GetSocketError());
            }

    #endif
        }
        else
        {
            if (isBroadcast)
            {
                sock_opt = 1;
                sockResult = setsockopt(m_socket, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<char*>(&sock_opt), sizeof(sock_opt));
                AZ_Error("GridMate", sockResult == 0, "Failed to enable broadcast. Error: %d", Platform::GetSocketError());
            }
        }

        if (Platform::SetFastSocketClose(m_socket, m_isDatagram) != EC_OK)
        {
            return EC_SOCKET_SOCK_OPT;
        }
        return EC_OK;
    }

    //=========================================================================
    // Initialize
    // [10/14/2010]
    //=========================================================================
    Driver::ResultCode
    SocketDriverCommon::Initialize(int ft, const char* address, unsigned int port, bool isBroadcast, unsigned int receiveBufferSize, unsigned int sendBufferSize)
    {
        AZ_Assert(ft == BSD_AF_INET || ft == BSD_AF_INET6, "Family type (ft) can be IPV4 or IPV6 only!");
        Platform::PrepareFamilyType(ft, m_isIpv6);

        m_port = htons(static_cast<unsigned short>(port));
        char portStr[8];
        azsnprintf(portStr, AZ_ARRAY_SIZE(portStr), "%d", port);

        addrinfo* addrInfo;
        addrinfo  hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = m_isIpv6 ? AF_INET6 : AF_INET;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_flags = AI_PASSIVE;
        hints.ai_protocol = IPPROTO_UDP;

        if (address && strlen(address) == 0)
        {
            address = nullptr;
        }

        int error = Platform::GetAddressInfo(address, portStr, &hints, &addrInfo);
        if (error != 0)
        {
            SocketErrorBuffer buffer;
            AZ_UNUSED(buffer);
            AZ_Assert(false, "SocketDriver::Initialize - Platform::GetAddressInfo() failed with %s!", Platform::GetSocketErrorString(Platform::GetSocketError(), buffer));
            return EC_SOCKET_CREATE;
        }

        m_socket = CreateSocket(addrInfo->ai_family, addrInfo->ai_socktype, addrInfo->ai_protocol);

        if (Platform::IsValidSocket(m_socket))
        {
            ResultCode res = SetSocketOptions(isBroadcast, receiveBufferSize, sendBufferSize);
            if (res != EC_OK)
            {
                Platform::FreeAddressInfo(addrInfo);
                closesocket(m_socket);
                return res;
            }

            if (Platform::IsSocketError(BindSocket(addrInfo->ai_addr, addrInfo->ai_addrlen)))
            {
                int socketErr = Platform::GetSocketError();
                (void)socketErr;
                AZ_TracePrintf("GridMate", "SocketDriver::Initialize - bind failed with code %d at port %d\n", socketErr, port);
                Platform::FreeAddressInfo(addrInfo);
                closesocket(m_socket);
                return EC_SOCKET_BIND;
            }

            if (m_port == 0)  // if we use implicit bind, retrieve the system assigned port
            {
                socklen_t addrLen = static_cast<socklen_t>(addrInfo->ai_addrlen);
                if (getsockname(m_socket, addrInfo->ai_addr, &addrLen) == 0)
                {
                    if (addrLen == sizeof(sockaddr_in6))
                    {
                        m_port = reinterpret_cast<sockaddr_in6*>(addrInfo->ai_addr)->sin6_port;
                    }
                    else
                    {
                        m_port = reinterpret_cast<sockaddr_in*>(addrInfo->ai_addr)->sin_port;
                    }
                }

                AZ_Error("GridMate", m_port != 0, "Failed to implicitly assign port (getsockname faled with %d)!", Platform::GetSocketError());
                if (m_port == 0)
                {
                    int socketErr = Platform::GetSocketError();
                    (void)socketErr;
                    AZ_TracePrintf("GridMate", "SocketDriver::Initialize - getsockname failed with code %d at port %d\n", socketErr, port);
                    Platform::FreeAddressInfo(addrInfo);
                    closesocket(m_socket);
                    return EC_SOCKET_BIND;
                }
            }
        }
        else
        {
            m_port = 0;
            int socketErr = Platform::GetSocketError();
            (void)socketErr;
            AZ_TracePrintf("GridMate", "SocketDriver::Initialize - socket failed with code %d at port %d\n", socketErr, port);
            Platform::FreeAddressInfo(addrInfo);
            return EC_SOCKET_CREATE;
        }

        Platform::FreeAddressInfo(addrInfo);

        const ResultCode res = m_platformDriver->Initialize(receiveBufferSize, sendBufferSize);
        if (res != EC_OK)
        {
            closesocket(m_socket);
            m_socket = Platform::GetInvalidSocket();
            return res;
        }
        return EC_OK;
    }

    //=========================================================================
    // GetPort
    // [7/9/2012]
    //=========================================================================
    unsigned int
    SocketDriverCommon::GetPort() const
    {
        return ntohs(m_port);
    }

    //=========================================================================
    // Send
    // [10/14/2010]
    //=========================================================================
    Driver::ResultCode
    SocketDriverCommon::Send(const AZStd::intrusive_ptr<DriverAddress>& to, const char* data, unsigned int dataSize)
    {
        ResultCode rc = EC_OK;

        if (m_canSend)
        {
            AZ_Assert(to && data, "Invalid function input!");
            AZ_Assert(dataSize <= SocketDriverCommon::GetMaxSendSize(), "Size is too big to send! Must be less than %d bytes", SocketDriverCommon::GetMaxSendSize());

            unsigned int addressSize;
            const sockaddr* sockAddr = reinterpret_cast<const sockaddr*>(to->GetTargetAddress(addressSize));
            if (sockAddr == nullptr)
            {
#ifdef AZ_LOG_UNBOUND_SEND_RECEIVE
                AZ_TracePrintf("GridMate", "SocketDriver::Send - address %s is not bound. This is not an error if you support unbound connectins, but data was NOT send!\n", to->ToString().c_str());
#endif // AZ_LOG_UNBOUND_SEND_RECEIVE
                return /*EC_SEND_ADDRESS_NOT_BOUND*/ EC_OK;
            }

            rc = m_platformDriver->Send(sockAddr, addressSize, data, dataSize);
        }
        else
        {
            AZ_TracePrintf("GridMate", "SocketDriver::Send - Double Send for address %s\n", to->ToString().c_str());
            rc = EC_PLATFORM + 1;   // double send error
        }

        if(rc == EC_OK)
        {
            EBUS_EVENT_ID(this, DriverEventBus, OnDatagramSent, dataSize, to);
        }
        return rc;
    }

    //=========================================================================
    // Receive
    // [10/14/2010]
    //=========================================================================
    unsigned int
    SocketDriverCommon::Receive(char* data, unsigned int maxDataSize, AZStd::intrusive_ptr<DriverAddress>& from, ResultCode* resultCode)
    {
        AZ_Assert(data, "Invalid function input!");
        //AZ_Assert(maxDataSize>=GetMaxSendSize(),"Size is too small to receive data! Must be atleast %d bytes",GetMaxSendSize());
        union // or use sockaddr_storage
        {
            sockaddr_in  sockAddrIn;
            sockaddr_in6 sockAddrIn6;
        };
        sockaddr* sockAddr = reinterpret_cast<sockaddr*>(&sockAddrIn6);
        socklen_t sockAddrLen = sizeof(sockAddrIn6);
        from = nullptr;

        unsigned int recvd = m_platformDriver->Receive(data, maxDataSize, sockAddr, sockAddrLen, resultCode);

        if (recvd)
        {
            from = CreateDriverAddress(sockAddr);
            if (!from)  // if we did not assign an address, ignore the data.
            {
                recvd = 0;
#ifdef AZ_LOG_UNBOUND_SEND_RECEIVE
                char ip[64];
                unsigned short port;
                if (sockAddrLen == sizeof(sockAddrIn6))
                {
                    inet_ntop(AF_INET6, &sockAddrIn6.sin6_addr, ip, AZ_ARRAY_SIZE(ip));
                    port = htons(sockAddrIn6.sin6_port);
                }
                else
                {
                    inet_ntop(AF_INET, &sockAddrIn.sin_addr, ip, AZ_ARRAY_SIZE(ip));
                    port = htons(sockAddrIn.sin_port);
                }
                (void)port;
                AZ_TracePrintf("GridMate", "Data discarded from %s|%d\n", ip, port);
#endif // AZ_LOG_UNBOUND_SEND_RECEIVE
            }
            EBUS_EVENT_ID(this, DriverEventBus, OnDatagramReceived, recvd, from);
        }

        if (resultCode)
        {
            *resultCode = EC_OK;
        }

        return static_cast<unsigned int>(recvd);
    }

    //=========================================================================
    // WaitForData
    // [6/5/2012]
    //=========================================================================
    bool
    SocketDriverCommon::WaitForData(AZStd::chrono::microseconds timeOut)
    {
        return m_platformDriver->WaitForData(timeOut);
    }

    //=========================================================================
    // StopWaitForData
    // [6/5/2012]
    //=========================================================================
    void
    SocketDriverCommon::StopWaitForData()
    {
        m_platformDriver->StopWaitForData();
    }

    //=========================================================================
    // CreateSocketDriver
    // [3/4/2013]
    //=========================================================================
    AZStd::string
    SocketDriverCommon::IPPortToAddressString(const char* ip, unsigned int port)
    {
        AZ_Assert(ip != nullptr, "Invalid address!");
        return AZStd::string::format("%s|%d", ip, port);
    }

    //=========================================================================
    // CreateSocketDriver
    // [3/4/2013]
    //=========================================================================
    bool
    SocketDriverCommon::AddressStringToIPPort(const AZStd::string& address, AZStd::string& ip, unsigned int& port)
    {
        AZStd::size_t pos = address.find('|');
        AZ_Assert(pos != AZStd::string::npos, "Invalid driver address!");
        if (pos == AZStd::string::npos)
        {
            return false;
        }

        ip = AZStd::string(address.begin(), address.begin() + pos);
        port = AZStd::stoi(AZStd::string(address.begin() + pos + 1, address.end()));

        return true;
    }

    //=========================================================================
    // AddressFamilyType
    // [7/11/2013]
    //=========================================================================
    Driver::BSDSocketFamilyType
    SocketDriverCommon::AddressFamilyType(const AZStd::string& ip)
    {
        // TODO: We can/should use inet_ntop() to detect the family type
        AZStd::size_t pos = ip.find(".");
        if (pos != AZStd::string::npos)
        {
            return BSD_AF_INET;
        }
        pos = ip.find("::");
        if (pos != AZStd::string::npos)
        {
            return BSD_AF_INET6;
        }
        return BSD_AF_UNSPEC;
    }

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    // Socket Driver
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    //=========================================================================
    // CreateDriverAddress(const AZStd::string&)
    // [1/12/2011]
    //=========================================================================
    AZStd::intrusive_ptr<DriverAddress>
    SocketDriver::CreateDriverAddress(const AZStd::string& address)
    {
        AZStd::string ip;
        unsigned int port;
        if (!AddressToIPPort(address, ip, port))
        {
            return nullptr;
        }

        SocketDriverAddress drvAddr(this, ip, port);
        return &*m_addressMap.insert(drvAddr).first;
    }

    //=========================================================================
    // CreateDriverAddress(const sockaddr_in& addr)
    // [1/12/2011]
    //=========================================================================
    AZStd::intrusive_ptr<DriverAddress>
    SocketDriver::CreateDriverAddress(const sockaddr* sockAddr)
    {
        SocketDriverAddress drvAddr(this, sockAddr);
        return &*m_addressMap.insert(drvAddr).first;
    }

    //=========================================================================
    // DestroyDriverAddress
    // [1/12/2011]
    //=========================================================================
    void SocketDriver::DestroyDriverAddress(DriverAddress* address)
    {
        if (address)
        {
            AZ_Assert(address->GetDriver() == this, "The address %s doesn't belong to this driver!", address->ToString().c_str());
            SocketDriverAddress* socketAddress = static_cast<SocketDriverAddress*>(address);
            m_addressMap.erase(*socketAddress);
        }
    }

    namespace Utils
    {
        // \note function moved here to use addinfo when IPV6 is not in use, consider moving those definitions to a header file
        bool GetIpByHostName(int familyType, const char* hostName, AZStd::string& ip)
        {
            static const size_t kMaxLen = 64; // max length of ipv6 ip is 45 chars, so all ips should be able to fit in this buf
            char ipBuf[kMaxLen];

            addrinfo hints;
            memset(&hints, 0, sizeof(struct addrinfo));
            addrinfo* addrInfo;
            hints.ai_family = (familyType == Driver::BSD_AF_INET6) ? AF_INET6 : AF_INET;
            hints.ai_flags = AI_CANONNAME;

            int result = Platform::GetAddressInfo(hostName, nullptr, &hints, &addrInfo);
            if (result)
            {
                return false;
            }

            if (addrInfo->ai_family == AF_INET6)
            {
                ip = inet_ntop(hints.ai_family, &reinterpret_cast<sockaddr_in6*>(addrInfo->ai_addr)->sin6_addr, ipBuf, AZ_ARRAY_SIZE(ipBuf));
            }
            else
            {
                ip = inet_ntop(hints.ai_family, &reinterpret_cast<sockaddr_in*>(addrInfo->ai_addr)->sin_addr, ipBuf, AZ_ARRAY_SIZE(ipBuf));
            }

            Platform::FreeAddressInfo(addrInfo);
            return true;
        }
    } //namespace Utils

    SocketDriverCommon::PlatformSocketDriver::PlatformSocketDriver(SocketDriverCommon &parent, SocketType &socket)
    : m_parent(parent), m_socket(socket) {}

    SocketDriverCommon::PlatformSocketDriver::~PlatformSocketDriver()
    {
        if (Platform::IsValidSocket(m_socket))
        {
            closesocket(m_socket);
            m_socket = Platform::GetInvalidSocket();
        }
    }
    Driver::ResultCode SocketDriverCommon::PlatformSocketDriver::Initialize(unsigned int /*receiveBufferSize*/, unsigned int /*sendBufferSize*/)
    {
        return EC_OK;
    }

    SocketDriverCommon::SocketType SocketDriverCommon::PlatformSocketDriver::CreateSocket(int af, int type, int protocol)
    {
        return socket(af, type, protocol);
    }

    Driver::ResultCode SocketDriverCommon::PlatformSocketDriver::Send(const sockaddr* sockAddr, unsigned int addressSize, const char* data, unsigned int dataSize)
    {
        do
        {
            if (Platform::IsSocketError(sendto(m_socket, data, dataSize, 0, sockAddr, static_cast<socklen_t>(addressSize))))
            {
                int errorCode = Platform::GetSocketError();
                int wouldNotBlock = AZ_EWOULDBLOCK;
                // if a non blocking socket (todo add a quick check)
                if (errorCode != wouldNotBlock)   // it's ok if a non blocking socket can't get the command instantly
                {
                    AZ_Error("GridMate", false, "SocketDriver::Send - sendto failed with code %d!", errorCode);
                }
                else
                {
                    // If we run out of buffer just wait for some buffer to become available
                    fd_set fdwrite;
                    FD_ZERO(&fdwrite);
                    FD_SET(m_socket, &fdwrite);
                    select(FD_SETSIZE, nullptr, &fdwrite, nullptr, nullptr);
                    continue;
                }

                return EC_SEND;
            }
            else
            {
                break;
            }

        } while (true);

        return EC_OK;
    }

    unsigned int SocketDriverCommon::PlatformSocketDriver::Receive(char* data, unsigned maxDataSize, sockaddr* sockAddr, socklen_t sockAddrLen, ResultCode* resultCode)
    {
        AZ::s64 recvd;
        do
        {
            recvd = recvfrom(m_socket, data, maxDataSize, 0, sockAddr, &sockAddrLen);
            if (Platform::IsSocketError(recvd))
            {
                int error = Platform::GetSocketError();

                int wouldNotBlock = AZ_EWOULDBLOCK;

                // if a non blocking socket (todo add a quick check)
                if (error == wouldNotBlock)
                {
                    if (resultCode)
                    {
                        *resultCode = EC_OK;
                    }
                    return 0;  // this is normal for non blocking sockets
                }

                (void)error;
                AZ_TracePrintf("GridMate", "SocketDriver::Receive - recvfrom failed with code %d, dataSize=%d\n", error, maxDataSize);
                if (resultCode)
                {
                    *resultCode = EC_RECEIVE;
                }
                return 0;
            }
            if (recvd != sizeof(AZ_SOCKET_WAKEUP_MSG_TYPE) || *(AZ_SOCKET_WAKEUP_MSG_TYPE*)data != AZ_SOCKET_WAKEUP_MSG_VALUE)
            {
                break;  // internal wake up message
            }
        } while (true);

        return static_cast<unsigned int>(recvd);
    }

    bool SocketDriverCommon::PlatformSocketDriver::WaitForData(AZStd::chrono::microseconds timeOut)
    {
        // If we run out of buffer just wait for some buffer to become available
        fd_set fdread;
        FD_ZERO(&fdread);
        FD_SET(m_socket, &fdread);
        timeval t = Platform::GetTimeValue(timeOut);

        int result = select(FD_SETSIZE, &fdread, nullptr, nullptr, &t);
        if (result > 0)
        {
            m_parent.m_isStoppedWaitForData = true;
            return true;
        }
        AZ_Warning("GridMate", result >= 0, "Socket select error %d\n", Platform::GetSocketError());
        m_parent.m_isStoppedWaitForData = false;
        return false;
    }

    void SocketDriverCommon::PlatformSocketDriver::StopWaitForData()
    {
        // This is a little tricky we just send one byte of data on a loopback
        // so we unlock the select function. Data will be discarded.
        if (Platform::IsValidSocket(m_socket))
        {
            const AZ_SOCKET_WAKEUP_MSG_TYPE data = AZ_SOCKET_WAKEUP_MSG_VALUE;
            if (m_parent.m_isIpv6)
            {
                sockaddr_in6 sockAddr;
                sockAddr.sin6_family = AF_INET6;
                sockAddr.sin6_addr = in6addr_loopback;
                sockAddr.sin6_port = m_parent.m_port;
                sendto(m_socket, (const char*)&data, sizeof(data), 0, (const sockaddr*)&sockAddr, sizeof(sockAddr)); // if an error occurs we don't care as we will wake up anyway
            }
            else
            {
                sockaddr_in sockAddr;
                sockAddr.sin_family = AF_INET;
                sockAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
                sockAddr.sin_port = m_parent.m_port;
                sendto(m_socket, (const char*)&data, sizeof(data), 0, (const sockaddr*)&sockAddr, sizeof(sockAddr)); // if an error occurs we don't care as we will wake up anyway
            }
        }
    }

    bool SocketDriverCommon::PlatformSocketDriver::isSupported()
    {
        return true;    //Generic driver always supported
    };

#ifdef AZ_SOCKET_RIO_SUPPORT
    SocketDriverCommon::RIOPlatformSocketDriver::RIOPlatformSocketDriver(SocketDriverCommon &parent, SocketType &socket)
        : PlatformSocketDriver(parent, socket)
        , m_workerBufferCount(0)
    { }

    SocketDriverCommon::RIOPlatformSocketDriver::~RIOPlatformSocketDriver()
    {
        const auto DeregisterAndFreeBuffer = [&](RIO_BUFFERID& id, char* rawBuffer)
        {
            m_RIO_FN_TABLE.RIODeregisterBuffer(id);
            FreeRIOBuffer(rawBuffer);
        };

        if (m_isInitialized)
        {
            //Worker thread
            m_workersQuit = true;
            m_triggerWorkerSend.notify_all();
            if (m_workerSendThread.joinable())
            {
                m_workerSendThread.join();
            }

            if (Platform::IsValidSocket(m_socket))
            {
                closesocket(m_socket);
                m_socket = Platform::GetInvalidSocket();
            }

            //Completion Queues
            m_RIO_FN_TABLE.RIOCloseCompletionQueue(m_RIORecvQueue);
            m_RIO_FN_TABLE.RIOCloseCompletionQueue(m_RIOSendQueue);

            //Events
            for (auto& ev : m_events)
            {
                WSACloseEvent(ev);
            }

            //Buffers
            //Note: each set of RIO buffers share the same buffer ID so only deregister the first
            DeregisterAndFreeBuffer(m_RIORecvBuffer[0].BufferId, m_rawRecvBuffer);
            DeregisterAndFreeBuffer(m_RIORecvAddressBuffer[0].BufferId, m_rawRecvAddressBuffer);
            DeregisterAndFreeBuffer(m_RIOSendBuffer[0].BufferId, m_rawSendBuffer);
            DeregisterAndFreeBuffer(m_RIOSendAddressBuffer[0].BufferId, m_rawSendAddressBuffer);
        }
    }

    bool SocketDriverCommon::RIOPlatformSocketDriver::isSupported()
    {
        //Requires Windows 8 / Server 2012 or newer
        if(!IsWindows8OrGreater())
        {
            const auto err = GetLastError();
            if(err != ERROR_OLD_WIN_VERSION)
            {
                AZ_Error("GridMate", false, "Failed to Verify OS Version: %d", err);
            }

            AZ_TracePrintf("GridMate", "RIO not supported on this platform\n");
            return false;
        }
        return true;
    }

    SocketDriverCommon::SocketType SocketDriverCommon::RIOPlatformSocketDriver::CreateSocket(int af, int type, int protocol)
    {
        SocketType s = ::WSASocketW(af, type, protocol, NULL, 0, WSA_FLAG_REGISTERED_IO);

        AZ_Error("GridMate", s != INVALID_SOCKET, "Invalid create socket\n");

        return s;
    }

    Driver::ResultCode SocketDriverCommon::RIOPlatformSocketDriver::Initialize(unsigned int receiveBufferSize, unsigned int sendBufferSize)
    {
        if(m_isInitialized)
        {
            AZ_Error("GridMate", !m_isInitialized, "PlatformSocketDriver double Initialize!\n");
            return EC_SOCKET_CREATE;
        }

        AZ_TracePrintf("GridMate", "SocketDriver RIO (%p) starting up.\n", this);

        //
        // We have to make a system call here and we need to know our page size. Needs to happen
        // before call to AllocRIOBuffer.
        //

        SYSTEM_INFO systemInfo;
        ::GetSystemInfo(&systemInfo);
        m_pageSize = systemInfo.dwPageSize;

        GUID functionTableId = WSAID_MULTIPLE_RIO;
        DWORD dwBytes = 0;
        if (sendBufferSize)
        {
            m_RIOSendBufferCount = sendBufferSize / m_RIOBufferSize + ((sendBufferSize%m_RIOBufferSize) ? 1 : 0);
        }
        if (receiveBufferSize)
        {
            m_RIORecvBufferCount = receiveBufferSize / m_RIOBufferSize + ((receiveBufferSize%m_RIOBufferSize) ? 1 : 0);
        }

        //runtime check
        if (0 != WSAIoctl( m_socket, SIO_GET_MULTIPLE_EXTENSION_FUNCTION_POINTER, &functionTableId,
            sizeof(GUID), (void**)&m_RIO_FN_TABLE,  sizeof(m_RIO_FN_TABLE), &dwBytes, 0, 0))
        {
            AZ_Error("GridMate", false, "Could not initialize RIO: %u\n", GridMate::Platform::GetSocketError());
            return EC_SOCKET_CREATE;
        }
        else
        {
            //RIO
            const ULONG maxOutstandingReceive = m_RIORecvBufferCount;
            const ULONG maxReceiveDataBuffers = 1; //Must be 1.
            const ULONG maxOutstandingSend = m_RIOSendBufferCount;
            const ULONG maxSendDataBuffers = 1; //Must be 1.

            void *pContext = 0;

            if ((m_events[WakeupOnSend] = WSACreateEvent()) == WSA_INVALID_EVENT)
            {
                AZ_Error("GridMate", false, "Failed WSACreateEvent(): %u\n", GridMate::Platform::GetSocketError());
                return EC_SOCKET_CREATE;
            }

            if ((m_events[ReceiveEvent] = WSACreateEvent()) == WSA_INVALID_EVENT)
            {
                AZ_Error("GridMate", false, "Failed WSACreateEvent(): %u\n", GridMate::Platform::GetSocketError());
                return EC_SOCKET_CREATE;
            }
            RIO_NOTIFICATION_COMPLETION typeRecv;
            typeRecv.Type = RIO_EVENT_COMPLETION;
            typeRecv.Event.EventHandle = m_events[ReceiveEvent];
            typeRecv.Event.NotifyReset = TRUE; //causes the event to be automatically reset by the RIONotify function when the notification occurs
            m_RIORecvQueue = m_RIO_FN_TABLE.RIOCreateCompletionQueue(maxOutstandingReceive, &typeRecv);
            if (m_RIORecvQueue == RIO_INVALID_CQ)
            {
                AZ_Error("GridMate", false, "Could not RIOCreateCompletionQueue: %u\n", GridMate::Platform::GetSocketError());
                return EC_SOCKET_CREATE;
            }

            if ((m_events[SendEvent] = WSACreateEvent()) == WSA_INVALID_EVENT)
            {
                AZ_Error("GridMate", false, "Failed WSACreateEvent(): %u\n", GridMate::Platform::GetSocketError());
                return EC_SOCKET_CREATE;
            }
            RIO_NOTIFICATION_COMPLETION typeSend;
            typeSend.Type = RIO_EVENT_COMPLETION;
            typeSend.Event.EventHandle = m_events[SendEvent];
            typeSend.Event.NotifyReset = TRUE; //causes the event to be automatically reset by the RIONotify function when the notification occurs
            m_RIOSendQueue = m_RIO_FN_TABLE.RIOCreateCompletionQueue(maxOutstandingSend, &typeSend);
            if (m_RIOSendQueue == RIO_INVALID_CQ)
            {
                AZ_Error("GridMate", false, "Could not RIOCreateCompletionQueue: %u\n", GridMate::Platform::GetSocketError());
                return EC_SOCKET_CREATE;
            }

            m_requestQueue = m_RIO_FN_TABLE.RIOCreateRequestQueue( m_socket, maxOutstandingReceive,
                maxReceiveDataBuffers, maxOutstandingSend, maxSendDataBuffers, m_RIORecvQueue,  m_RIOSendQueue, pContext);
            if (m_requestQueue == RIO_INVALID_RQ)
            {
                AZ_Error("GridMate", m_requestQueue != NULL, "Could not RIOCreateRequestQueue: %u\n", GridMate::Platform::GetSocketError());
                return EC_SOCKET_CREATE;
            }

            //Setup buffers
            AZ::u64 recvAllocated = 0, recvAddrsAllocated = 0;
            AZ::u64 sendAllocated = 0, sendAddrsAllocated = 0;
            const DWORD bufferSize = m_RIOBufferSize;
            RIO_BUFFERID recvBufferId, recvAddressBufferId;

            //Setup Recv raw buffer and RIO record
            if (nullptr == (m_rawRecvBuffer = AllocRIOBuffer(bufferSize, m_RIORecvBufferCount, &recvAllocated)))
            {
                AZ_Error("GridMate", false, "Could not allocate buffer: %u\n", GridMate::Platform::GetSocketError());
                return EC_SOCKET_CREATE;
            }
            if (RIO_INVALID_BUFFERID == (recvBufferId = m_RIO_FN_TABLE.RIORegisterBuffer(m_rawRecvBuffer, bufferSize * m_RIORecvBufferCount)))
            {
                AZ_Error("GridMate", false, "Could not register buffer: %u\n", GridMate::Platform::GetSocketError());
                return EC_SOCKET_CREATE;
            }

            //Setup Recv address raw buffer and RIO record
            if (nullptr == (m_rawRecvAddressBuffer = AllocRIOBuffer(sizeof(SOCKADDR_INET), m_RIORecvBufferCount, &recvAddrsAllocated)))
            {
                AZ_Error("GridMate", false, "Could not allocate buffer: %u\n", GridMate::Platform::GetSocketError());
                return EC_SOCKET_CREATE;
            }
            if (RIO_INVALID_BUFFERID == (recvAddressBufferId = m_RIO_FN_TABLE.RIORegisterBuffer(m_rawRecvAddressBuffer, sizeof(SOCKADDR_INET) * m_RIORecvBufferCount)))
            {
                AZ_Error("GridMate", false, "Could not register buffer: %u\n", GridMate::Platform::GetSocketError());
                return EC_SOCKET_CREATE;
            }

            //Init RIO Recv buffers
            m_RIORecvBuffer.reserve(m_RIORecvBufferCount);
            m_RIORecvAddressBuffer.reserve(m_RIORecvBufferCount);
            for (int i = 0; i < m_RIORecvBufferCount; ++i)
            {
                char *pBuffer = m_rawRecvAddressBuffer + i * bufferSize;
                RIO_BUF buf;

                buf.BufferId    = recvBufferId;
                buf.Offset      = i * bufferSize;
                buf.Length      = bufferSize;

                m_RIORecvBuffer.push_back(buf);

                buf.BufferId    = recvAddressBufferId;
                buf.Offset      = i * sizeof(SOCKADDR_INET);
                buf.Length      = sizeof(SOCKADDR_INET);

                m_RIORecvAddressBuffer.push_back(buf);

                //Start Receive Handler
                if (false == m_RIO_FN_TABLE.RIOReceiveEx(m_requestQueue, &m_RIORecvBuffer[i], 1, NULL, &m_RIORecvAddressBuffer[i], NULL, NULL, 0, pBuffer))
                {
                    AZ_Error("GridMate", false, "Could not RIOReceive: %u\n", GridMate::Platform::GetSocketError());
                    return EC_SOCKET_CREATE;
                }
            }

            RIO_BUFFERID sendBufferId, sendAddressBufferId;

            //setup send raw buffer and RIO record
            if (nullptr == (m_rawSendBuffer = AllocRIOBuffer(bufferSize, m_RIOSendBufferCount, &sendAllocated)))
            {
                AZ_Error("GridMate", false, "Could not allocate buffer: %u", GridMate::Platform::GetSocketError());
                return EC_SOCKET_CREATE;
            }
            if (RIO_INVALID_BUFFERID == (sendBufferId = m_RIO_FN_TABLE.RIORegisterBuffer(m_rawSendBuffer, m_RIOSendBufferCount * bufferSize)))
            {
                AZ_Error("GridMate", false, "Could not register buffer: %u\n", GridMate::Platform::GetSocketError());
                return EC_SOCKET_CREATE;
            }

            //setup send address raw buffer and RIO record
            if (nullptr == (m_rawSendAddressBuffer = AllocRIOBuffer(sizeof(SOCKADDR_INET), m_RIOSendBufferCount, &sendAddrsAllocated)))
            {
                AZ_Error("GridMate", false, "Could not allocate send address buffer: %u\n", GridMate::Platform::GetSocketError());
                return EC_SOCKET_CREATE;
            }

            if (RIO_INVALID_BUFFERID == (sendAddressBufferId = m_RIO_FN_TABLE.RIORegisterBuffer(m_rawSendAddressBuffer, m_RIOSendBufferCount * sizeof(SOCKADDR_INET))))
            {
                AZ_Error("GridMate", false, "Could not register buffer: %u\n", GridMate::Platform::GetSocketError());
                return EC_SOCKET_CREATE;
            }

            //Init RIO Send buffers
            m_RIOSendBuffer.reserve(m_RIOSendBufferCount);
            m_RIOSendAddressBuffer.reserve(m_RIOSendBufferCount);
            for (int i = 0; i < m_RIOSendBufferCount; ++i)
            {
                RIO_BUF buf;
                buf.BufferId    = sendBufferId;
                buf.Offset      = i * bufferSize;
                buf.Length      = bufferSize;

                m_RIOSendBuffer.push_back(buf);

                buf.BufferId    = sendAddressBufferId;
                buf.Offset      = i * sizeof(SOCKADDR_INET);
                buf.Length      = sizeof(SOCKADDR_INET);

                m_RIOSendAddressBuffer.push_back(buf);
            }
        }

        //worker packet send thread
        AZStd::thread_desc workerSendThreadDesc;
        workerSendThreadDesc.m_name = "GridMate-Carrier Packet Send Thread";
        m_workerSendThread = AZStd::thread(workerSendThreadDesc, AZStd::bind(&SocketDriverCommon::RIOPlatformSocketDriver::WorkerSendThread, this));
        if (m_workerSendThread.get_id() == AZStd::native_thread_invalid_id)
        {
            AZ_Error("GridMate", false, "Could not create worker thread.");
            return EC_SOCKET_CREATE;
        }

        AZ_TracePrintf("GridMate", "SocketDriver RIO (%p) startup successful.\n", this);
        m_isInitialized = true;
        return EC_OK;
    }

    void SocketDriverCommon::RIOPlatformSocketDriver::WorkerSendThread()
    {
        const auto workerHasDatagramsToSend = [&] { return m_workerBufferCount > 0 || m_workersQuit;  };
        while(!m_workersQuit)
        {
            {
                AZStd::unique_lock<AZStd::mutex> l(m_WorkerSendMutex);
                m_triggerWorkerSend.wait(l, workerHasDatagramsToSend);
                //m_workerBufferCount is the only shared atomic so release the lock
            }

            while (workerHasDatagramsToSend() && !m_workersQuit)
            {
                for (;;)
                {
                    static const int bufferCount = 1;
                    if (!m_RIO_FN_TABLE.RIOSendEx(m_requestQueue, &m_RIOSendBuffer[m_workerNextSendBuffer],
                        bufferCount, NULL, &m_RIOSendAddressBuffer[m_workerNextSendBuffer], NULL, NULL, 0, 0))
                    {
                        const DWORD lastError = GridMate::Platform::GetSocketError();
                        if (lastError == WSAENOBUFS)
                        {
                            continue;   //spin until free
                        }
                        else if (lastError == WSA_IO_PENDING)
                        {
                            break;
                        }

                        const RIO_BUF *rioBuf = &m_RIOSendBuffer[m_workerNextSendBuffer];
                        const void *data = reinterpret_cast<const void*>(&m_rawSendBuffer[rioBuf->Offset]);
                        (void)data;
                        const SOCKADDR_INET *adrs = reinterpret_cast<const SOCKADDR_INET*>(&m_rawSendAddressBuffer[m_RIOSendAddressBuffer[m_workerNextSendBuffer].Offset]);
                        (void)adrs;

                        AZ_TracePrintf("SocketDriver-RIO",
                            "RIOSendEX failed! Buffer/Length/Offset:%u/%u/%u WSAError=%u\n"
                            "Adrs: %u.%u.%u.%u:%u\n"
                            "%s\n",
                            m_workerNextSendBuffer, rioBuf->Length, rioBuf->Offset, lastError,
                            adrs->Ipv4.sin_addr.S_un.S_un_b.s_b1, adrs->Ipv4.sin_addr.S_un.S_un_b.s_b3, adrs->Ipv4.sin_addr.S_un.S_un_b.s_b3, adrs->Ipv4.sin_addr.S_un.S_un_b.s_b4, adrs->Ipv4.sin_port,
                            AZStd::MemoryToASCII::ToString(data, rioBuf->Length, m_RIOBufferSize).c_str());
                        break;
                    }
                    else
                    {
                        AZStd::lock_guard<AZStd::mutex> l(m_RIOSendQueueMutex);
                        m_RIO_FN_TABLE.RIONotify(m_RIOSendQueue);
                        break;
                    }
                }
                --m_workerBufferCount;
                if (++m_workerNextSendBuffer == m_RIOSendBufferCount)
                {
                    m_workerNextSendBuffer = 0;
                }
            }
        }
    }
    Driver::ResultCode SocketDriverCommon::RIOPlatformSocketDriver::Send(const sockaddr* sockAddr, unsigned int /*addressSize*/, const char* data, unsigned int dataSize)
    {
        if( dataSize > m_RIOBufferSize)
        {
            AZ_TracePrintf("GridMateSecure", "Buffer too large to send! Size=%u\n", dataSize);
            return EC_BUFFER_TOOLARGE;
        }

        memcpy(m_rawSendAddressBuffer + m_RIONextSendBuffer * sizeof(SOCKADDR_INET), reinterpret_cast<const char*>(sockAddr), /* addressSize */sizeof(SOCKADDR_INET));
        memcpy(m_rawSendBuffer + m_RIONextSendBuffer * m_RIOBufferSize, data, dataSize);
        m_RIOSendBuffer[m_RIONextSendBuffer].Length = dataSize;

        ++m_workerBufferCount;      //update shared atomic
        ++m_RIOSendBuffersInUse;
        ++m_RIONextSendBuffer;
        if (m_RIONextSendBuffer == m_RIOSendBufferCount)
        {
            m_RIONextSendBuffer = 0;
        }

        if (m_RIOSendBuffersInUse == m_RIOSendBufferCount)
        {
            m_parent.m_canSend = false;  //wait for completion
        }

        m_triggerWorkerSend.notify_one();   //signal worker thread

        return EC_OK;
    }
    unsigned int SocketDriverCommon::RIOPlatformSocketDriver::Receive(char* data, unsigned maxDataSize, sockaddr* sockAddr, socklen_t sockAddrLen, ResultCode* resultCode)
    {
        AZ::s64 recvd = 0;
        static const int bufferCount = 1;
        do
        {
            (void)maxDataSize;
            (void)sockAddrLen;
            recvd = 0;

            RIORESULT result;
            const int resultsRequested = 1;
            memset(reinterpret_cast<void*>(&result), 0, sizeof(result));
            ULONG numResults = m_RIO_FN_TABLE.RIODequeueCompletion(m_RIORecvQueue, &result, resultsRequested);

            AZ_Error("GridMate", RIO_CORRUPT_CQ != numResults, "RIO Queue corrupted during RIODequeueCompletion()");

            if (numResults == 0)
            {
                if (resultCode)
                {
                    *resultCode = EC_OK;
                }
                m_RIO_FN_TABLE.RIONotify(m_RIORecvQueue);

                return 0;
            }
            if (numResults != resultsRequested)
            {
                AZ_Error("GridMate", resultsRequested == numResults, "Too many results returned: %d/%d", numResults, resultsRequested);
            }
            recvd = result.BytesTransferred;
            AZ_Error("GridMate", recvd <= maxDataSize, "Recvd too many bytes %d > %d\n", recvd, maxDataSize);

            memcpy(data, m_rawRecvBuffer + m_RIONextRecvBuffer * m_RIOBufferSize, recvd);
            memcpy(sockAddr, m_rawRecvAddressBuffer + m_RIONextRecvBuffer * sizeof(SOCKADDR_INET), m_RIORecvAddressBuffer[m_RIONextRecvBuffer].Length); //TODO length check?

            //Resetup to handle a receive event
            if (false == m_RIO_FN_TABLE.RIOReceiveEx(m_requestQueue, &m_RIORecvBuffer[m_RIONextRecvBuffer],
                bufferCount, NULL, &m_RIORecvAddressBuffer[m_RIONextRecvBuffer], NULL, NULL, 0, 0))
            {
                AZ_Error("GridMate", false, "Could not RIOReceive: %u\n", GridMate::Platform::GetSocketError());
            }

            if (recvd)
            {
                m_RIONextRecvBuffer++;  //move to the next buffer
                if (m_RIONextRecvBuffer == m_RIORecvBufferCount)
                {
                    m_RIONextRecvBuffer = 0;
                }
                break;
            }
        } while (true);

        return static_cast<unsigned int>(recvd);
    }
    bool SocketDriverCommon::RIOPlatformSocketDriver::WaitForData(AZStd::chrono::microseconds timeOut)
    {
        DWORD Index = 0;
        const auto start = AZStd::chrono::system_clock::now();
        auto remainingWaitTime = timeOut.count() / 1000;

        const auto isWakeOnSend = [](DWORD Index) { return WakeupOnSend == Index - WSA_WAIT_EVENT_0; };
        const auto isSend = [](DWORD Index) { return SendEvent == Index - WSA_WAIT_EVENT_0; };
        const auto isReceive = [](DWORD Index) { return ReceiveEvent == Index - WSA_WAIT_EVENT_0; };
        const auto isTimeout = [](DWORD Index) { return WSA_WAIT_TIMEOUT == Index; };
        const auto isFailed = [](DWORD Index) { return WSA_WAIT_FAILED == Index; };
        const auto resetSignalEvent = [&](DWORD Index)
        {
            if (!WSAResetEvent(m_events[Index - WSA_WAIT_EVENT_0]))
            {
                AZ_Assert(false, "WSAResetEvent failed with error = %d\n", GridMate::Platform::GetSocketError());
            }
        };

        bool loop = true;
        do
        {
            Index = WSAWaitForMultipleEvents(NumberOfEvents, m_events, FALSE, static_cast<DWORD>(remainingWaitTime), FALSE);
            loop = isWakeOnSend(Index) && !m_parent.m_canSend;
            if (loop)
            {
                resetSignalEvent(Index);
                const auto duration = AZStd::chrono::milliseconds(AZStd::chrono::system_clock::now() - start).count();
                if (duration < remainingWaitTime)
                {
                    remainingWaitTime -= duration;
                }
                else
                {
                    break;
                }
            }
        } while (loop);

        bool rtrn = false;

        if (isWakeOnSend(Index))
        {
            rtrn = m_parent.m_canSend;   //Send if we can otherwise perform a receive loop
        }
        else if (isSend(Index))
        {
            RIORESULT* result = new RIORESULT[m_RIOSendBufferCount];
            //memset(reinterpret_cast<void*>(&result), 0, sizeof(result));
            ULONG numResults;
            {
                AZStd::lock_guard<AZStd::mutex> l(m_RIOSendQueueMutex);
                numResults = m_RIO_FN_TABLE.RIODequeueCompletion(m_RIOSendQueue, result, m_RIOSendBufferCount);
            }
            if (1 > numResults)    //cleanup completion queue
            {
                AZ_Assert(false, "dequeue failed");
            }
            m_RIOSendBuffersInUse -= numResults;
            m_parent.m_canSend = true;
            rtrn = true;
            delete [] result;
        }
        else if (isReceive(Index))
        {
            rtrn = false;
        }
        else if (isTimeout(Index))
        {
            m_parent.m_isStoppedWaitForData = false;
            return false;
        }
        else if (isFailed(Index))
        {
            AZ_Assert(false, "WSAWaitForMultipleEvents failed with error = %d\n", GridMate::Platform::GetSocketError());
            return false;
        }
        else
        {
            AZ_Assert(false, "Unsupported WSAWaitForMultipleEvents() return %d", Index);
        }

        m_parent.m_isStoppedWaitForData = true; //Did not timeout

        resetSignalEvent(Index);

        return rtrn;
    }

    void SocketDriverCommon::RIOPlatformSocketDriver::StopWaitForData()
    {
        if (!SetEvent(m_events[WakeupOnSend])) //Wake thread
        {
            AZ_Assert(false, "SetEvent failed with error = %d\n", GridMate::Platform::GetSocketError());
        }
    }

    char *SocketDriverCommon::RIOPlatformSocketDriver::AllocRIOBuffer(AZ::u64 bufferSize, AZ::u64 numBuffers, AZ::u64* amountAllocated /*=nullptr*/)
    {
        // calculate how much memory we are really asking for, and this must be page aligned.
        AZ::u64 totalBufferSize = RoundUp(bufferSize * numBuffers, m_pageSize);

        if (amountAllocated != nullptr)
        {
            *amountAllocated = totalBufferSize;
        }

        // By using VirtualAlloc, we guarantee that our memory will be paged aligned.
        return reinterpret_cast<char *>(::VirtualAlloc(nullptr, totalBufferSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    }

    bool SocketDriverCommon::RIOPlatformSocketDriver::FreeRIOBuffer(char *buffer)
    {
        bool success = false;

        if (buffer != nullptr)
        {
            success = (::VirtualFree(buffer, 0, MEM_RELEASE) == TRUE);
        }

        return success;
    }
#endif
}
