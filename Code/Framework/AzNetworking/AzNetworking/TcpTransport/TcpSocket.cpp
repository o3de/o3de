/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/AzNetworking_Traits_Platform.h>
#include <AzNetworking/TcpTransport/TcpSocket.h>
#include <AzNetworking/Utilities/Endian.h>
#include <AzNetworking/Utilities/NetworkIncludes.h>
#include <AzCore/Console/ILogger.h>

namespace AzNetworking
{
    TcpSocket::TcpSocket()
        : m_socketFd(InvalidSocketFd)
    {
        ;
    }

    TcpSocket::TcpSocket(SocketFd socketFd)
        : m_socketFd(socketFd)
    {
        if (m_socketFd != InvalidSocketFd)
        {
            if (!(SetSocketNonBlocking(m_socketFd) && SetSocketNoDelay(m_socketFd)))
            {
                Close();
            }
        }
    }

    TcpSocket::~TcpSocket()
    {
        Close();
    }

    bool TcpSocket::IsEncrypted() const
    {
        return false;
    }

    TcpSocket* TcpSocket::CloneAndTakeOwnership()
    {
        TcpSocket* result = new TcpSocket(m_socketFd);
        m_socketFd = InvalidSocketFd;
        return result;
    }

    bool TcpSocket::Listen(uint16_t port)
    {
        Close();

        if (!SocketCreateInternal()
         || !BindSocketForListenInternal(port)
         || !(SetSocketNonBlocking(m_socketFd) && SetSocketNoDelay(m_socketFd)))
        {
            Close();
            return false;
        }

        return true;
    }

    bool TcpSocket::Connect(const IpAddress& address, uint16_t localPort)
    {
        Close();

        if (!SocketCreateInternal()
         || !BindSocketForConnectInternal(address, localPort)
         || !(SetSocketNonBlocking(m_socketFd) && SetSocketNoDelay(m_socketFd)))
        {
            Close();
            return false;
        }

        return true;
    }

    void TcpSocket::Close()
    {
        CloseSocket(m_socketFd);
        m_socketFd = InvalidSocketFd;
    }

    int32_t TcpSocket::Send(const uint8_t* data, uint32_t size) const
    {
        AZ_Assert(size > 0, "Invalid data size for send");
        AZ_Assert(data != nullptr, "NULL data pointer passed to send");
        if (!IsOpen())
        {
            return SocketOpResultErrorNotOpen;
        }
        return SendInternal(data, size);
    }

    int32_t TcpSocket::Receive(uint8_t* outData, uint32_t size) const
    {
        AZ_Assert(size > 0, "Invalid data size for receive");
        AZ_Assert(outData != nullptr, "NULL data pointer passed to receive");
        if (!IsOpen())
        {
            return SocketOpResultErrorNotOpen;
        }
        return ReceiveInternal(outData, size);
    }

    int32_t TcpSocket::SendInternal(const uint8_t* data, uint32_t size) const
    {
        const int32_t sentBytes = static_cast<int32_t>(send(aznumeric_cast<int32_t>(m_socketFd), (const char*)data, size, 0));

        if (sentBytes < 0)
        {
            const int32_t error = GetLastNetworkError();
            if (ErrorIsWouldBlock(error)) // Filter would block messages
            {
                return 0;
            }
            AZLOG_WARN("Failed to write to socket (%d:%s)", error, GetNetworkErrorDesc(error));
        }

        return sentBytes;
    }

    int32_t TcpSocket::ReceiveInternal(uint8_t* outData, uint32_t size) const
    {
        const int32_t receivedBytes = static_cast<int32_t>(recv(aznumeric_cast<int32_t>(m_socketFd), (char*)outData, (int32_t)size, 0));

        if (receivedBytes < 0)
        {
            const int32_t error = GetLastNetworkError();
            if (ErrorIsWouldBlock(error)) // Filter would block messages
            {
                return 0;
            }
            AZLOG_WARN("Failed to read from socket (%d:%s)", error, GetNetworkErrorDesc(error));
        }
        else if (receivedBytes == 0)
        {
            // Clean disconnect, force the endpoint to disconnect and cleanup
            return SocketOpResultDisconnected;
        }

        return receivedBytes;
    }

    bool TcpSocket::BindSocketForListenInternal(uint16_t port)
    {
        // Handle binding
        sockaddr_in hints;
        hints.sin_family = AF_INET;
        hints.sin_addr.s_addr = INADDR_ANY;
        hints.sin_port = htons(port);

        if (::bind(aznumeric_cast<int32_t>(m_socketFd), (const sockaddr*)&hints, sizeof(hints)) != 0)
        {
            const int32_t error = GetLastNetworkError();
            AZLOG_WARN("Failed to bind TCP socket to port %u (%d:%s)", uint32_t(port), error, GetNetworkErrorDesc(error));
            return false;
        }

        if (::listen(aznumeric_cast<int32_t>(m_socketFd), SOMAXCONN) != 0)
        {
            const int32_t error = GetLastNetworkError();
            AZLOG_WARN("Failed to listen on socket (%d:%s)", error, GetNetworkErrorDesc(error));
            return false;
        }

        return true;
    }

    bool TcpSocket::BindSocketForConnectInternal(const IpAddress& address, uint16_t localPort)
    {
        // If a specific local port was specified, attempt to use that port
        if (localPort != 0)
        {
            sockaddr_in hints;
            memset(&hints, 0, sizeof(hints));
            hints.sin_family = AF_INET;
            hints.sin_port = htons(localPort);

            if (::bind(aznumeric_cast<int32_t>(m_socketFd), (const sockaddr*)&hints, sizeof(hints)) != 0)
            {
                const int32_t error = GetLastNetworkError();
                AZLOG_WARN("Failed to bind TCP socket to port %u (%d:%s)", uint32_t(localPort), error, GetNetworkErrorDesc(error));
                return false;
            }
        }

        // Initiate connection to the requested remote endpoint
        {
            struct sockaddr_in dest;
            memset(&dest, 0, sizeof(dest));

            dest.sin_family = AF_INET;
            dest.sin_addr.s_addr = address.GetAddress(ByteOrder::Network);
            dest.sin_port = address.GetPort(ByteOrder::Network);

            if (::connect(aznumeric_cast<int32_t>(m_socketFd), (struct sockaddr*)&dest, sizeof(dest)) != 0)
            {
                const int32_t error = GetLastNetworkError();
                AZLOG(NET_TcpTraffic, "Failed to connect to remote endpoint (%s) (%d:%s)", address.GetString().c_str(), error, GetNetworkErrorDesc(error));
                return false;
            }
        }

        return true;
    }

    bool TcpSocket::SocketCreateInternal()
    {
        AZ_Assert(!IsOpen(), "Open called on an active socket");

        if (IsOpen())
        {
            return false;
        }

        // Open the socket
        {
            m_socketFd = (SocketFd)::socket(AF_INET, SOCK_STREAM, 0);

            if (!IsOpen())
            {
                const int32_t error = GetLastNetworkError();
                AZLOG_WARN("Failed to create socket (%d:%s)", error, GetNetworkErrorDesc(error));
                m_socketFd = InvalidSocketFd;
                return false;
            }
        }

        return true;
    }
}
