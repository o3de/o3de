/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AzNetworking
{
    inline TcpSocket* TcpConnection::GetTcpSocket() const
    {
        return m_socket.get();
    }

    inline void TcpConnection::SetTimeoutId(TimeoutId timeoutId)
    {
        m_timeoutId = timeoutId;
    }

    inline TimeoutId TcpConnection::GetTimeoutId() const
    {
        return m_timeoutId;
    }

    inline bool TcpConnection::IsOpen() const
    {
        return m_socket->IsOpen();
    }

    inline void TcpConnection::SetRegisteredSocketFd(SocketFd registeredSocketFd)
    {
        m_registeredSocketFd = registeredSocketFd;
    }

    inline SocketFd TcpConnection::GetRegisteredSocketFd() const
    {
        return m_registeredSocketFd;
    }
}
