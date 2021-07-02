/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/Framework/INetworkInterface.h>
#include <AzNetworking/UdpTransport/DtlsSocket.h>
#include <AzCore/Console/ILogger.h>

#if AZ_TRAIT_USE_OPENSSL
#   include <openssl/ssl.h>
#   include <openssl/err.h>
#endif

namespace AzNetworking
{
    DtlsSocket::~DtlsSocket()
    {
        FreeSslContext(m_sslContext);
    }

    bool DtlsSocket::IsEncrypted() const
    {
        return true;
    }

    DtlsEndpoint::ConnectResult DtlsSocket::ConnectDtlsEndpoint(DtlsEndpoint& dtlsEndpoint, const IpAddress& address, UdpPacketEncodingBuffer& outDtlsData) const
    {
        return dtlsEndpoint.Connect(*this, address, outDtlsData);
    }

    DtlsEndpoint::ConnectResult DtlsSocket::AcceptDtlsEndpoint(DtlsEndpoint& dtlsEndpoint, const IpAddress& address) const
    {
        return dtlsEndpoint.Accept(*this, address);
    }

    bool DtlsSocket::Open(uint16_t port, CanAcceptConnections canAccept, TrustZone trustZone)
    {
        Close();

        const SslContextType contextType = (canAccept == UdpSocket::CanAcceptConnections::True) ? SslContextType::DtlsGeneric : SslContextType::DtlsClient;
        m_sslContext = CreateSslContext(contextType, trustZone);

        if (m_sslContext == nullptr)
        {
            AZLOG_ERROR("SSL context creation call failed");
            PrintSslErrorStack();
            Close();
            return false;
        }

        if (!UdpSocket::Open(port, canAccept, trustZone))
        {
            AZLOG_ERROR("UDP socket creation failed");
            PrintSslErrorStack();
            Close();
            return false;
        }

        return true;
    }

    void DtlsSocket::Close()
    {
        FreeSslContext(m_sslContext);
        UdpSocket::Close();
    }

    int32_t DtlsSocket::SendInternal(const IpAddress& address, const uint8_t* data, uint32_t size, bool encrypt, DtlsEndpoint& dtlsEndpoint) const
    {
        if (!encrypt)
        {
            // If the packet has requested to remain unencrypted then just send directly
            return UdpSocket::SendInternal(address, data, size, encrypt, dtlsEndpoint);
        }

        if (dtlsEndpoint.m_sslSocket == nullptr)
        {
            AZLOG_ERROR("Trying to send on an open socketfd, but with a nullptr ssl socket wrapper!");
            return SocketOpResultErrorNoSsl;
        }

#if AZ_TRAIT_USE_OPENSSL
        uint8_t encrpytedSendBuffer[MaxUdpTransmissionUnit];
        // Write out the packet we were requested to send
        const int32_t sentBytesRaw = SSL_write(dtlsEndpoint.m_sslSocket, data, size);
        const int32_t sentBytesEnc = BIO_read(dtlsEndpoint.m_writeBio, encrpytedSendBuffer, sizeof(encrpytedSendBuffer));

        // Track encryption metrics
        m_sentBytesEncryptionInflation += aznumeric_cast<uint32_t>(sentBytesEnc - aznumeric_cast<int32_t>(size));
        m_sentPacketsEncrypted++;

        return UdpSocket::SendInternal(address, encrpytedSendBuffer, sentBytesEnc, encrypt, dtlsEndpoint);
#else
        return 0;
#endif
    }
}
