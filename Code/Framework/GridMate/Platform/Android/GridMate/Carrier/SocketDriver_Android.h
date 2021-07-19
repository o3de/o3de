
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <android/api-level.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#define closesocket(_s) close(_s)
#define AZ_EWOULDBLOCK EWOULDBLOCK
#define AZ_EINPROGRESS EINPROGRESS
#define AZ_ECONNREFUSED ECONNREFUSED
#define AZ_EALREADY EALREADY
#define AZ_EISCONN EISCONN
#define AZ_ENETUNREACH ENETUNREACH
#define AZ_ETIMEDOUT ETIMEDOUT
#define SO_NBIO FIONBIO
#define ioctlsocket  ioctl

namespace GridMate
{
    namespace Platform
    {
        using SocketType_Platform = int;
    }
}
