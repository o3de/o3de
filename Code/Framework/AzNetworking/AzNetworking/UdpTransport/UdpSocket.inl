/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AzNetworking
{
    inline bool UdpSocket::IsOpen() const
    {
        return (m_socketFd > SocketFd{ 0 });
    }

    inline SocketFd UdpSocket::GetSocketFd() const
    {
        return m_socketFd;
    }

    inline uint32_t UdpSocket::GetSentPackets() const
    {
        return m_sentPackets;
    }

    inline uint32_t UdpSocket::GetSentBytes() const
    {
        return m_sentBytes;
    }

    inline uint32_t UdpSocket::GetSentPacketsEncrypted() const
    {
        return m_sentPacketsEncrypted;
    }

    inline uint32_t UdpSocket::GetSentBytesEncryptionInflation() const
    {
        return m_sentBytesEncryptionInflation;
    }

    inline uint32_t UdpSocket::GetRecvPackets() const
    {
        return m_recvPackets;
    }

    inline uint32_t UdpSocket::GetRecvBytes() const
    {
        return m_recvBytes;
    }
}
