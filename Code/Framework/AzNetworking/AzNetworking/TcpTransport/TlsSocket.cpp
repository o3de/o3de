/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/TcpTransport/TlsSocket.h>
#include <AzCore/Console/ILogger.h>

#if AZ_TRAIT_USE_OPENSSL
#   include <openssl/ssl.h>
#   include <openssl/err.h>
#endif

namespace AzNetworking
{
    TlsSocket::TlsSocket(TrustZone trustZone)
        : TcpSocket()
        , m_sslContext(nullptr)
        , m_sslSocket(nullptr)
        , m_trustZone(trustZone)
    {
        ;
    }

    TlsSocket::TlsSocket(SocketFd socketFd, TrustZone trustZone)
        : TcpSocket(socketFd)
        , m_sslContext(nullptr)
        , m_sslSocket(nullptr)
        , m_trustZone(trustZone)
    {
        m_sslContext = CreateSslContext(SslContextType::TlsGeneric, trustZone);

        if (m_sslContext == nullptr)
        {
            AZLOG_ERROR("Listen call failed, SSL context creation failed");
            Close();
            return;
        }

        m_sslSocket = CreateSslForAccept(m_socketFd, m_sslContext);

        if (m_sslSocket == nullptr)
        {
            AZLOG_ERROR("Listen call failed, SSL socket wrapper creation failed");
            Close();
            return;
        }

        if (!(SetSocketNonBlocking(m_socketFd) && SetSocketNoDelay(m_socketFd)))
        {
            return;
        }
    }

    TlsSocket::~TlsSocket()
    {
        FreeSslContext(m_sslContext);
        AzNetworking::Close(m_sslSocket);
    }

    bool TlsSocket::IsEncrypted() const
    {
        return true;
    }

    TcpSocket* TlsSocket::CloneAndTakeOwnership()
    {
        TlsSocket* result = new TlsSocket(m_socketFd, m_trustZone);
        result->m_sslContext = m_sslContext;
        result->m_sslSocket = m_sslSocket;

        m_socketFd = InvalidSocketFd;
        m_sslContext = nullptr;
        m_sslSocket = nullptr;

        return result;
    }

    bool TlsSocket::Listen(uint16_t port)
    {
        Close();

        m_sslContext = CreateSslContext(SslContextType::TlsServer, m_trustZone);

        if (m_sslContext == nullptr)
        {
            AZLOG_ERROR("Listen call failed, SSL context creation failed");
            Close();
            return false;
        }

        if (!SocketCreateInternal())
        {
            Close();
            return false;
        }

        if (!BindSocketForListenInternal(port))
        {
            Close();
            return false;
        }

        m_sslSocket = CreateSslForAccept(GetSocketFd(), m_sslContext);

        if (m_sslSocket == nullptr)
        {
            AZLOG_ERROR("Listen call failed, SSL socket wrapper creation failed");
            Close();
            return false;
        }

        if (!(SetSocketNonBlocking(m_socketFd) && SetSocketNoDelay(m_socketFd)))
        {
            Close();
            return false;
        }

        return true;
    }

    bool TlsSocket::Connect(const IpAddress& address)
    {
        Close();

        m_sslContext = CreateSslContext(SslContextType::TlsClient, m_trustZone);

        if (m_sslContext == nullptr)
        {
            AZLOG_ERROR("Listen call failed, SSL context creation failed");
            Close();
            return false;
        }

        if (!SocketCreateInternal())
        {
            Close();
            return false;
        }

        if (!BindSocketForConnectInternal(address))
        {
            Close();
            return false;
        }

        m_sslSocket = CreateSslForConnect(GetSocketFd(), m_sslContext);

        if (m_sslSocket == nullptr)
        {
            AZLOG_ERROR("Listen call failed, SSL socket wrapper creation failed");
            Close();
            return false;
        }

        if (!(SetSocketNonBlocking(m_socketFd) && SetSocketNoDelay(m_socketFd)))
        {
            Close();
            return false;
        }

        return true;
    }

    void TlsSocket::Close()
    {
        FreeSslContext(m_sslContext);
        AzNetworking::Close(m_sslSocket);
        TcpSocket::Close();
    }

    int32_t TlsSocket::SendInternal([[maybe_unused]] const uint8_t* data, [[maybe_unused]] uint32_t size) const
    {
        if (m_sslSocket == nullptr)
        {
            AZLOG_ERROR("Trying to send on an open socketfd, but with a nullptr ssl socket wrapper!");
            return SocketOpResultErrorNoSsl;
        }
#if AZ_TRAIT_USE_OPENSSL
        const int32_t sentBytes = SSL_write(m_sslSocket, data, size);
        if (sentBytes < 0)
        {
            const int32_t sslError = SSL_get_error(m_sslSocket, sentBytes);
            if (SslErrorIsWouldBlock(sslError)) // Filter would block messages
            {
                return SocketOpResultSuccess;
            }
            const int32_t osError = GetLastNetworkError();
            AZLOG_ERROR("Failed to read from socket (%d:%s) (%d:%s)", sslError, ERR_error_string(sslError, nullptr), osError, GetNetworkErrorDesc(osError));
        }
        return sentBytes;
#else
        return 0;
#endif
    }

    int32_t TlsSocket::ReceiveInternal([[maybe_unused]] uint8_t* outData, [[maybe_unused]] uint32_t size) const
    {
        if (m_sslSocket == nullptr)
        {
            AZLOG_ERROR("Trying to receive on an open socketfd, but with a nullptr ssl socket wrapper!");
            return SocketOpResultErrorNoSsl;
        }
#if AZ_TRAIT_USE_OPENSSL
        int32_t receivedBytes = SSL_read(m_sslSocket, outData, size);
        if (receivedBytes < 0)
        {
            const int32_t sslError = SSL_get_error(m_sslSocket, receivedBytes);
            if (SslErrorIsWouldBlock(sslError)) // Filter would block messages
            {
                return SocketOpResultSuccess;
            }
            const int32_t osError = GetLastNetworkError();
            AZLOG_ERROR("Failed to read from socket (%d:%s) (%d:%s)", sslError, ERR_error_string(sslError, nullptr), osError, GetNetworkErrorDesc(osError));
        }
        else if (receivedBytes == 0)
        {
            // Clean disconnect, force the endpoint to disconnect and cleanup
            return SocketOpResultDisconnected;
        }
        return receivedBytes;
#else
        return 0;
#endif
    }
}
