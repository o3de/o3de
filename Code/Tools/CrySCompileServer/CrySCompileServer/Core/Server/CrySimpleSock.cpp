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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "CrySimpleSock.hpp"

#include <Core/StdTypes.hpp>
#include <Core/WindowsAPIImplementation.h>
#include <Core/Error.hpp>
#include <Core/STLHelper.hpp>

#include <AzCore/Debug/Trace.h>
#include <AzCore/std/parallel/atomic.h>
#include <algorithm>

#if defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_MAC)
#include <libkern/OSAtomic.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#else
#include <ws2tcpip.h>
typedef int socklen_t;
#endif

namespace
{
    enum ECrySimpleS_TYPE
    {
        ECrySimpleST_ROOT,
        ECrySimpleST_SERVER,
        ECrySimpleST_CLIENT,
        ECrySimpleST_INVALID,
    };

    static AZStd::atomic_long numberOfOpenSockets = {0};
    const int MAX_DATA_SIZE = 1024 * 1024; // Only allow 1 MB of data to come through. Lumberyard Game Engine has the same size constraint
    const size_t BLOCKSIZE = 4 * 1024;
    const size_t MAX_ERROR_MESSAGE_SIZE = 1024;
    const size_t MAX_HOSTNAME_BUFFER_SIZE = 1024;

    struct Ip4WhitelistAddress
    {
        Ip4WhitelistAddress() : m_address(0), m_mask(-1) { }
        // IP Address in network order to whitelist 
        uint32_t m_address;
        // Mask in network order to apply to connecting IP addresses
        uint32_t m_mask;
    };
}

struct CCrySimpleSock::Implementation
{

    Implementation(ECrySimpleS_TYPE type)
        : m_Type(type) { }

    void SetWhitelist(const std::vector<AZStd::string>& whiteList)
    {
        // Add in our local address so that we always allow connections from the local machine
        char hostNameBuffer[MAX_HOSTNAME_BUFFER_SIZE] = { 0 };
        gethostname(hostNameBuffer, MAX_HOSTNAME_BUFFER_SIZE);
        struct addrinfo* addressInfos{};
        struct addrinfo hints{};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        int addressInfoResultCode = getaddrinfo(hostNameBuffer, nullptr, &hints, &addressInfos);

        if (addressInfoResultCode == 0)
        {
            int i = 0;
            for (auto addressInfoIter = addressInfos; addressInfoIter != nullptr; addressInfoIter = addressInfoIter->ai_next)
            {
                Ip4WhitelistAddress whitelistAddress;
                whitelistAddress.m_address = static_cast<uint32_t>(reinterpret_cast<sockaddr_in*>(addressInfoIter->ai_addr)->sin_addr.s_addr);
                m_ipWhiteList.push_back(whitelistAddress);
                ++i;
            }
        }
        else
        {
            printf("Network error trying to get host computer local address. The host computer's local IP addresses will not be automatically whitelisted.");
        }

        for (const auto& address : whiteList)
        {
            Ip4WhitelistAddress whitelistAddress;
            AZStd::string::size_type maskLocation = address.rfind("/");
            if (maskLocation != AZStd::string::npos)
            {
                //x.x.x.x/0 is all addresses
                // For CIDR that specify the network mask, mask out the address that is 
                // supplied here once instead of everytime we check the address during 
                // accept calls. 
                int mask = atoi(address.substr(maskLocation+1).c_str());
                if (mask == 0)
                {
                    whitelistAddress.m_mask = 0;
                    whitelistAddress.m_address = 0;

                    static bool warnOnce = true;
                    if (warnOnce)
                    {
                        warnOnce = false;
                        printf("\nWARNING: Attempting to run the CrySCompileServer authorizing every IP. This is a security risk and not recommended.\nPlease use a more restrictive whitelist in the config.ini file by not using netmask 0.\n\n");
                    }
                }
                else
                {
                    whitelistAddress.m_mask ^= (1 << (32 - mask)) - 1;
                    whitelistAddress.m_mask = htonl(whitelistAddress.m_mask);
                    struct in_addr ipv4Address{};
                    if (inet_pton(AF_INET, address.substr(0, maskLocation).c_str(), &ipv4Address) == 1)
                    {
                        whitelistAddress.m_address = static_cast<uint32_t>(ipv4Address.s_addr);
                    }
                }
            }
            else
            {
                struct in_addr ipv4Address{};
                if (inet_pton(AF_INET, address.c_str(), &ipv4Address) == 1)
                {
                    whitelistAddress.m_address = static_cast<uint32_t>(ipv4Address.s_addr);
                }
            }

            m_ipWhiteList.push_back(whitelistAddress);
        }
    }

    CCrySimpleSock* m_pInstance;

    const ECrySimpleS_TYPE  m_Type;
    SOCKET          m_Socket;
    uint16_t        m_Port;
#ifdef USE_WSAEVENTS
    WSAEVENT        m_Event;
#endif
    bool            m_WaitForShutdownEvent;
    bool            m_SwapEndian;

    bool            m_bHasReceivedData;
    bool            m_bHasSendData;

    tdDataVector    m_tempSendBuffer;

    std::vector<Ip4WhitelistAddress> m_ipWhiteList;
};

#if defined(AZ_PLATFORM_WINDOWS)
typedef BOOL (WINAPI * LPFN_DISCONNECTEX)(SOCKET, LPOVERLAPPED, DWORD, DWORD);
#define WSAID_DISCONNECTEX {0x7fda2e11, 0x8630, 0x436f, {0xa0, 0x31, 0xf5, 0x36, 0xa6, 0xee, 0xc1, 0x57} \
}
#endif

#ifdef USE_WSAEVENTS
CCrySimpleSock::CCrySimpleSock(SOCKET Sock, CCrySimpleSock* pInstance, WSAEVENT wsaEvent)
#else
CCrySimpleSock::CCrySimpleSock(SOCKET Sock, CCrySimpleSock * pInstance)
#endif
    : m_pImpl(new Implementation(ECrySimpleST_SERVER))
{
    #ifdef USE_WSAEVENTS
    m_pImpl->m_Event = wsaEvent;
    #endif

    m_pImpl->m_pInstance = pInstance;
    m_pImpl->m_Socket = Sock;
    m_pImpl->m_WaitForShutdownEvent = false;
    m_pImpl->m_bHasReceivedData = false;
    m_pImpl->m_bHasSendData = false;
    m_pImpl->m_Port = ~0;

    ++numberOfOpenSockets;

    InitClient();
}


CCrySimpleSock::CCrySimpleSock(const std::string& rServerName, uint16_t Port)
    : m_pImpl(new Implementation(ECrySimpleST_CLIENT))
{
    m_pImpl->m_pInstance = nullptr;
    m_pImpl->m_Socket = INVALID_SOCKET;
    m_pImpl->m_WaitForShutdownEvent = false;
    m_pImpl->m_bHasReceivedData = false;
    m_pImpl->m_bHasSendData = false;
    m_pImpl->m_Port = Port;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(Port);
    const char* pHostName = rServerName.c_str();
    bool IP = true;
    for (size_t a = 0, size = strlen(pHostName); a < size; a++)
    {
        IP &= (pHostName[a] >= '0' && pHostName[a] <= '9') || pHostName[a] == '.';
    }
    if (IP)
    {
        struct in_addr ipv4Address{};
        if (inet_pton(AF_INET, pHostName, &ipv4Address) == 1)
        {
            addr.sin_addr = ipv4Address;
        }
    }
    else
    {
        struct addrinfo* addressInfo{};
        struct addrinfo hints{};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        int addressInfoResultCode = getaddrinfo(pHostName, nullptr, &hints, &addressInfo);
        if (addressInfoResultCode != 9)
        {
            return;
        }
        addr = *reinterpret_cast<sockaddr_in*>(addressInfo->ai_addr);
    }

    m_pImpl->m_Socket = socket(AF_INET, SOCK_STREAM, 0);

    ++numberOfOpenSockets;

    int Err = connect(m_pImpl->m_Socket, (struct sockaddr*)&addr, sizeof addr);
    if (Err < 0)
    {
        m_pImpl->m_Socket = INVALID_SOCKET;
    }
}

CCrySimpleSock::~CCrySimpleSock()
{
    Release();
}

CCrySimpleSock::CCrySimpleSock(uint16_t Port, const std::vector<AZStd::string>& ipWhiteList)
    : m_pImpl(new Implementation(ECrySimpleST_ROOT))
{
    m_pImpl->m_pInstance = nullptr;
    m_pImpl->m_WaitForShutdownEvent = false;
    m_pImpl->m_bHasReceivedData = false;
    m_pImpl->m_bHasSendData = false;
    m_pImpl->m_Port = Port;

#ifdef _MSC_VER
    WSADATA Data;
    m_pImpl->m_Socket   =   INVALID_SOCKET;
    if (WSAStartup(MAKEWORD(2, 0), &Data))
    {
        CrySimple_ERROR("Could not init root socket");
        return;
    }
#endif
    m_pImpl->SetWhitelist(ipWhiteList);

    m_pImpl->m_Socket = socket(AF_INET, SOCK_STREAM, 0);
    if (INVALID_SOCKET == m_pImpl->m_Socket)
    {
        CrySimple_ERROR("Could not initialize basic server due to invalid socket");
        return;
    }

#if defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_MAC)
    int arg = 1;
    setsockopt(m_pImpl->m_Socket, SOL_SOCKET, SO_KEEPALIVE, &arg, sizeof arg);
    arg = 1;
    setsockopt(m_pImpl->m_Socket, SOL_SOCKET, SO_REUSEADDR, &arg, sizeof arg);
#endif

    sockaddr_in SockAddr;
    memset(&SockAddr, 0, sizeof(sockaddr_in));
    SockAddr.sin_family =   PF_INET;
    SockAddr.sin_port       =   htons(Port);
    if (bind(m_pImpl->m_Socket, (sockaddr*)&SockAddr, sizeof(sockaddr_in)) == SOCKET_ERROR)
    {
#if defined(AZ_PLATFORM_WINDOWS)
        AZ_Warning(0, false, "bind failed with error = %d", WSAGetLastError());
#else
        shutdown(m_pImpl->m_Socket, SHUT_RDWR);
#endif
        closesocket(m_pImpl->m_Socket);
        CrySimple_ERROR("Could not bind server socket. This can happen if there is another process running already that is using this port or antivirus software/firewall is blocking the port.\n");
        return;
    }

    ++numberOfOpenSockets;
}

void CCrySimpleSock::Listen()
{
    listen(m_pImpl->m_Socket, SOMAXCONN);
}

void CCrySimpleSock::InitClient()
{
}

void CCrySimpleSock::Release()
{
    if (m_pImpl->m_Socket != INVALID_SOCKET)
    {
        // check if we have received and sended data but ignore that for the HTTP server
        if ((!m_pImpl->m_bHasSendData || !m_pImpl->m_bHasReceivedData) && (!m_pImpl->m_pInstance || m_pImpl->m_pInstance->m_pImpl->m_Port != 80))
        {
            char acTmp[MAX_ERROR_MESSAGE_SIZE];
            azsprintf(acTmp, "ERROR : closing socket without both receiving and sending data: receive: %d send: %d",
                m_pImpl->m_bHasReceivedData, m_pImpl->m_bHasSendData);
            CRYSIMPLE_LOG(acTmp);
        }

#ifdef USE_WSAEVENTS
        if (m_pImpl->m_WaitForShutdownEvent)
        {
            // wait until client has shutdown its socket
            DWORD nReturnCode = WSAWaitForMultipleEvents(1, &m_pImpl->m_Event,
                    FALSE, INFINITE, FALSE);
            if ((nReturnCode != WSA_WAIT_FAILED) && (nReturnCode != WSA_WAIT_TIMEOUT))
            {
                WSANETWORKEVENTS NetworkEvents;
                WSAEnumNetworkEvents(m_pImpl->m_Socket, m_pImpl->m_Event,   &NetworkEvents);
                if (NetworkEvents.lNetworkEvents & FD_CLOSE)
                {
                    int iErrorCode = NetworkEvents.iErrorCode[FD_CLOSE_BIT];
                    if (iErrorCode != 0)
                    {
                        // error shutting down
                    }
                }
            }
        }

        // shutdown the server side of the connection since no more data will be sent
        shutdown(m_pImpl->m_Socket, SHUT_RDWR);
        closesocket(m_pImpl->m_Socket);
#endif

#if defined(AZ_PLATFORM_WINDOWS)
        LPFN_DISCONNECTEX pDisconnectEx = NULL;
        DWORD Bytes;
        GUID guidDisconnectEx = WSAID_DISCONNECTEX;
        WSAIoctl(m_pImpl->m_Socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidDisconnectEx,
            sizeof(GUID), &pDisconnectEx, sizeof(pDisconnectEx), &Bytes, NULL, NULL);
        pDisconnectEx(m_pImpl->m_Socket, NULL, 0, 0); // retrieve this function pointer with WSAIoctl(WSAID_DISCONNECTEX).
#else
        shutdown(m_pImpl->m_Socket, SHUT_RDWR);
#endif
        closesocket(m_pImpl->m_Socket);
        m_pImpl->m_Socket = INVALID_SOCKET;
        --numberOfOpenSockets;
    }

#if defined(AZ_PLATFORM_WINDOWS)
    switch (m_pImpl->m_Type)
    {
    case    ECrySimpleST_ROOT:
        WSACleanup();
        break;
    case    ECrySimpleST_SERVER:     // Intentionally fall through
    case    ECrySimpleST_CLIENT:
        break;
    default:
        CrySimple_ERROR("unknown SocketType Released");
    }
#endif
}

CCrySimpleSock* CCrySimpleSock::Accept()
{
    if (m_pImpl->m_Type != ECrySimpleST_ROOT)
    {
        CrySimple_ERROR("called Accept on non root socket");
        return nullptr;
    }

    while (true)
    {
        sockaddr_in connectingAddress;
        int addressSize = sizeof(connectingAddress);
        SOCKET Sock = accept(m_pImpl->m_Socket, reinterpret_cast<sockaddr*>(&connectingAddress), reinterpret_cast<socklen_t*>(&addressSize));
        if (Sock == INVALID_SOCKET)
        {
#if defined(AZ_PLATFORM_MAC)
            switch (errno)
            {
            case EINTR:
                // OS X tends to get interupt calls on every other accept call
                // so just ignore this particular error and try the accept call
                // again.
                continue;
            default:
                // Do nothing - all other errors are "real" and we should exit
                break;
            }
#endif
            AZ_Warning(0, false, "Errno = %d", WSAGetLastError());
            CrySimple_ERROR("Accept recived invalid socket");
            return nullptr;
        }

        bool allowConnection = false;

        for (const auto& ip4WhitelistAddress : m_pImpl->m_ipWhiteList)
        {
            if ((connectingAddress.sin_addr.s_addr & ip4WhitelistAddress.m_mask) == (ip4WhitelistAddress.m_address))
            {
                allowConnection = true;
                break;
            }
        }

        if (!allowConnection)
        {
            constexpr size_t ipAddressBufferSize = 17;
            char ipAddressBuffer[ipAddressBufferSize]{};
            inet_ntop(AF_INET, &connectingAddress.sin_addr, ipAddressBuffer, ipAddressBufferSize);
            printf("Warning: unauthorized IP %s trying to connect. If this IP is authorized please add it to the whitelist in the config.ini file\n", ipAddressBuffer);
            closesocket(Sock);
            continue;
        }

        int arg = 1;
        setsockopt(Sock, SOL_SOCKET, SO_REUSEADDR, (char*)&arg, sizeof arg);

        /*
        // keep socket open for another 2 seconds until data has been fully send
        LINGER linger;
        int len = sizeof(LINGER);
        linger.l_onoff = 1;
        linger.l_linger = 2;
        setsockopt(Sock, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof linger);
        */

#ifdef USE_WSAEVENTS
        WSAEVENT wsaEvent = WSACreateEvent();
        if (wsaEvent == WSA_INVALID_EVENT)
        {
            closesocket(Sock);
            int Error = WSAGetLastError();
            CrySimple_ERROR("Couldn't create wsa event");
            return nullptr;
        }

        int Status = WSAEventSelect(Sock, wsaEvent, FD_CLOSE);
        if (Status == SOCKET_ERROR)
        {
            closesocket(Sock);
            int Error = WSAGetLastError();
            CrySimple_ERROR("Couldn't create wsa event");
            return nullptr;
        }

        return new CCrySimpleSock(Sock, this, wsaEvent);
#else
        return new CCrySimpleSock(Sock, this);
#endif
    }
    
    return nullptr;
}

union CrySimpleRecvSize
{
    uint8_t     m_Data8[8];
    uint64_t    m_Data64;
};

static const int MAX_TIME_TO_WAIT = 10000;

int CCrySimpleSock::Recv(char* acData, int len, int flags)
{
    int recived = SOCKET_ERROR;
    int waitingtime = 0;
    while (recived < 0)
    {
        recived = recv(m_pImpl->m_Socket, acData, len, flags);
        if (recived == SOCKET_ERROR)
        {
            int WSAError = WSAGetLastError();
#if defined(AZ_PLATFORM_WINDOWS)
            if (WSAError == WSAEWOULDBLOCK)
            {
                // are we out of time
                if (waitingtime > MAX_TIME_TO_WAIT)
                {
                    char acTmp[MAX_ERROR_MESSAGE_SIZE];
                    azsprintf(acTmp, "Error while receiving size of data - Timeout on blocking. (Error Code: %i)", WSAError);
                    CrySimple_ERROR(acTmp);

                    return recived;
                }

                waitingtime += 5;

                // sleep a bit and try again
                Sleep(5);
            }
            else
#endif
            {
                char acTmp[MAX_ERROR_MESSAGE_SIZE];
                azsprintf(acTmp, "Error while receiving size of data - Network error. (Error Code: %i)", WSAError);
                CrySimple_ERROR(acTmp);

                return recived;
            }
        }
    }

    return recived;
}

bool CCrySimpleSock::Recv(std::vector<uint8_t>& rVec)
{
    CrySimpleRecvSize size;

    int received = Recv(reinterpret_cast<char*>(&size.m_Data8[0]), 8, 0);
    if (received != 8)
    {
#if defined(AZ_PLATFORM_WINDOWS)
        int WSAError = WSAGetLastError();
#else
        int WSAError = errno;
#endif
        char acTmp[MAX_ERROR_MESSAGE_SIZE];
        azsprintf(acTmp, "Error while receiving size of data - Invalid size (Error Code: %i)", WSAError);
        CrySimple_ERROR(acTmp);
        return false;
    }

    if (size.m_Data64 == 0)
    {
        int WSAError = WSAGetLastError();
        char acTmp[MAX_ERROR_MESSAGE_SIZE];
        azsprintf(acTmp, "Error while receiving size of data - Size of zero (Error Code: %i)", WSAError);
        CrySimple_ERROR(acTmp);

        return false;
    }

    if (size.m_Data64 > MAX_DATA_SIZE)
    {
        int WSAError = WSAGetLastError();
        char acTmp[MAX_ERROR_MESSAGE_SIZE];
        azsprintf(acTmp, "Error while receiving size of data - Size is greater than max support data size.");
        CrySimple_ERROR(acTmp);

        return false;
    }

    m_pImpl->m_SwapEndian   =   (size.m_Data64 >> 32) != 0;
    if (m_pImpl->m_SwapEndian)
    {
        CSTLHelper::EndianSwizzleU64(size.m_Data64);
    }

    rVec.clear();
    rVec.resize(static_cast<size_t>(size.m_Data64));

    for (uint32_t a = 0; a < size.m_Data64; )
    {
        int read = Recv(reinterpret_cast<char*>(&rVec[a]), static_cast<int>(size.m_Data64) - a, 0);
        if (read <= 0)
        {
            int WSAError = WSAGetLastError();
            char acTmp[MAX_ERROR_MESSAGE_SIZE];
            azsprintf(acTmp, "Error while receiving tcp-data (size: %d - Error Code: %i)", static_cast<int>(size.m_Data64), WSAError);
            CrySimple_ERROR(acTmp);
            return false;
        }
        a += read;
    }

    m_pImpl->m_bHasReceivedData = true;

    return true;
}

bool CCrySimpleSock::RecvResult()
{
    CrySimpleRecvSize size;
    if (recv(m_pImpl->m_Socket, reinterpret_cast<char*>(&size.m_Data8[0]), 8, 0) != 8)
    {
        CrySimple_ERROR("Error while receiving result");
        return false;
    }

    return size.m_Data64 > 0;
}


void CCrySimpleSock::Forward(const std::vector<uint8_t>& rVecIn)
{
    tdDataVector& rVec = m_pImpl->m_tempSendBuffer;
    rVec.resize(rVecIn.size() + 8);
    CrySimpleRecvSize& rHeader  =   *(CrySimpleRecvSize*)(&rVec[0]);
    rHeader.m_Data64    = (uint32_t)rVecIn.size();
    memcpy(&rVec[8], &rVecIn[0], rVecIn.size());

    CrySimpleRecvSize size;
    size.m_Data64 = static_cast<int>(rVec.size());
    for (uint64_t a = 0; a < size.m_Data64; a += BLOCKSIZE)
    {
        char* pData     =   reinterpret_cast<char*>(&rVec[(size_t)a]);
        int nSendRes    = send(m_pImpl->m_Socket, pData, std::min<int>(static_cast<int>(size.m_Data64 - a), BLOCKSIZE), 0);
        if (nSendRes == SOCKET_ERROR)
        {
            int nLastSendError = WSAGetLastError();
            logmessage("Socket send(forward) error: %d", nLastSendError);
        }
    }
}


bool CCrySimpleSock::Backward(std::vector<uint8_t>& rVec)
{
    uint32_t size;
    if (recv(m_pImpl->m_Socket, reinterpret_cast<char*>(&size), 4, 0) != 4)
    {
        CrySimple_ERROR("Error while receiving size of data");
        return false;
    }

    rVec.clear();
    rVec.resize(static_cast<size_t>(size));

    for (uint32_t a = 0; a < size; )
    {
        int read = recv(m_pImpl->m_Socket, reinterpret_cast<char*>(&rVec[a]), size - a, 0);
        if (read <= 0)
        {
            CrySimple_ERROR("Error while receiving tcp-data");
            return false;
        }
        a += read;
    }
    return true;
}

void CCrySimpleSock::Send(const std::vector<uint8_t>& rVecIn, size_t state, EProtocolVersion version)
{
    const size_t offset = version == EPV_V001 ? 4 : 5;
    tdDataVector& rVec = m_pImpl->m_tempSendBuffer;
    rVec.resize(rVecIn.size() + offset);
    if (rVecIn.size())
    {
        *(uint32_t*)(&rVec[0]) = (uint32_t)rVecIn.size();
        memcpy(&rVec[offset], &rVecIn[0], rVecIn.size());
    }

    if (version >= EPV_V002)
    {
        rVec[4] =   static_cast<uint8_t>(state);
    }

    if (m_pImpl->m_SwapEndian)
    {
        CSTLHelper::EndianSwizzleU32(*(uint32_t*)&rVec[0]);
    }

    // send can fail, you must retry unsent parts.
    size_t remainingBytes = rVec.size();
    const char* pData = reinterpret_cast<char*>(rVec.data());

    while (remainingBytes != 0)
    {
        size_t sendThisRound = remainingBytes;
        if (sendThisRound > BLOCKSIZE)
        {
            sendThisRound = BLOCKSIZE;
        }

        int bytesActuallySent = send(m_pImpl->m_Socket, pData, static_cast<int>(sendThisRound), 0);
        if (bytesActuallySent < 0)
        {
            int nLastSendError = WSAGetLastError();
            logmessage("Socket send error: %d", nLastSendError);
            m_pImpl->m_bHasSendData = true;
            return;
        }
        size_t actuallySent = static_cast<size_t>(bytesActuallySent);
        
        remainingBytes -= actuallySent;
        pData += actuallySent;
    }

    m_pImpl->m_bHasSendData = true;
}


void CCrySimpleSock::Send(const std::string& rData)
{
    const size_t S = rData.size();

    for (uint64_t a = 0; a < S; a += BLOCKSIZE)
    {
        const char* pData = &rData.c_str()[a];
        const int nSendRes = send(m_pImpl->m_Socket, pData, std::min<int>(static_cast<int>(S - a), BLOCKSIZE), 0);
        if (nSendRes == SOCKET_ERROR)
        {
            int nLastSendError = WSAGetLastError();
            logmessage("Socket send error: %d", nLastSendError);
        }
        else
        {
            m_pImpl->m_bHasSendData = true;
        }
    }
}

uint32_t CCrySimpleSock::PeerIP()
{
    struct sockaddr_in addr;
#if defined(AZ_PLATFORM_WINDOWS)
    int addr_size = sizeof(sockaddr_in);
#else
    socklen_t addr_size = sizeof(sockaddr_in);
#endif
    int nRes = getpeername(m_pImpl->m_Socket, (sockaddr*) &addr, &addr_size);
    if (nRes == SOCKET_ERROR)
    {
        int nError = WSAGetLastError();
        logmessage("Socket getpeername error: %d", nError);
        return 0;
    }
#if defined(AZ_PLATFORM_WINDOWS)
    return addr.sin_addr.S_un.S_addr;
#else
    return addr.sin_addr.s_addr;
#endif
}

bool CCrySimpleSock::Valid() const
{
    return m_pImpl->m_Socket != INVALID_SOCKET;
}

void CCrySimpleSock::WaitForShutDownEvent(bool bValue)
{
    m_pImpl->m_WaitForShutdownEvent = bValue;
}

long CCrySimpleSock::GetOpenSockets()
{
    return numberOfOpenSockets;
}

