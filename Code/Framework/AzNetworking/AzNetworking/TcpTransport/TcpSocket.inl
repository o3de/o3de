/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AzNetworking
{
    inline bool TcpSocket::IsOpen() const
    {
        return (m_socketFd > SocketFd{ 0 });
    }

    inline void TcpSocket::SetSocketFd(SocketFd socketFd)
    {
        m_socketFd = socketFd;
    }

    inline SocketFd TcpSocket::GetSocketFd() const
    {
        return m_socketFd;
    }
}
