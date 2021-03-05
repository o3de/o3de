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

#ifndef __CRYSIMPLESOCK__
#define __CRYSIMPLESOCK__

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

#include <Core/Common.h>
#include <Core/STLHelper.hpp>

#if defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_MAC)
typedef int SOCKET;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <errno.h>
#define closesocket close
#else
#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS // till we swtich to in inet_pton and getaddrinfo
#endif
#include <WinSock2.h>
#endif

#include <vector>
#include <memory>

//#define USE_WSAEVENTS

enum EProtocolVersion
{
    EPV_V001,
    EPV_V002,
    EPV_V0021,
    EPV_V0022,
    EPV_V0023,
};

class CCrySimpleSock
{
public:


#ifdef USE_WSAEVENTS
    CCrySimpleSock(SOCKET Sock, CCrySimpleSock* pInstance, WSAEVENT wsaEvent);
#else
    CCrySimpleSock(SOCKET Sock, CCrySimpleSock* pInstance);
#endif
    CCrySimpleSock(const CCrySimpleSock&);

    CCrySimpleSock(const std::string& rServerName, uint16_t Port);
    CCrySimpleSock(uint16_t Port, const std::vector<AZStd::string>& ipWhiteList);

    ~CCrySimpleSock();

    void    InitClient();
    void    Release();

    int     Recv(char* acData, int len, int flags);

    void    Listen();


    CCrySimpleSock* Accept();

    bool            Recv(std::vector<uint8_t>& rVec);
    bool            RecvResult();

    bool            Backward(std::vector<uint8_t>& rVec);
    void            Send(const std::vector<uint8_t>& rVec, size_t State, EProtocolVersion Version);
    void            Forward(const std::vector<uint8_t>& rVec);

    //used for HTML
    void            Send(const std::string& rData);

    uint32_t        PeerIP();

    bool            Valid() const;

    void            WaitForShutDownEvent(bool bValue);

    static long GetOpenSockets();
    
    
private:
    struct Implementation;
    std::unique_ptr<Implementation> m_pImpl;
};

#endif
