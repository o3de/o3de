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
