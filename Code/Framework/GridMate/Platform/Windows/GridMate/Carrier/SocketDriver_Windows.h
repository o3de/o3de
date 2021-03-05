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
#pragma once

#include <GridMate_Traits_Platform.h>

#include <AzCore/std/base.h>

#include <WinSock2.h>
#include <ws2tcpip.h>
#include <Mswsock.h>
#ifdef WSAID_MULTIPLE_RIO
    #define AZ_SOCKET_RIO_SUPPORT
#endif

struct sockaddr_in;
struct sockaddr;
struct addrinfo;

AZ_PUSH_DISABLE_WARNING(4800, "-Wunknown-warning-option")
#include <VersionHelpers.h>
AZ_POP_DISABLE_WARNING
#define SO_NBIO          FIONBIO
#define AZ_EWOULDBLOCK   WSAEWOULDBLOCK
#define AZ_EINPROGRESS   WSAEINPROGRESS
#define AZ_ECONNREFUSED  WSAECONNREFUSED
#define AZ_EALREADY      WSAEALREADY
#define AZ_EISCONN       WSAEISCONN
#define AZ_ENETUNREACH   WSAENETUNREACH
#define AZ_ETIMEDOUT     WSAETIMEDOUT

namespace GridMate
{
    namespace Platform
    {
        using SocketType_Platform = AZStd::size_t;
    }
}
