/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        if (SSL_write(dtlsEndpoint.m_sslSocket, data, size) <= 0)
        {
            AZLOG_ERROR("DTLS: encrypting a %u byte packet failed; it is dropped", size);
            PrintSslErrorStack();
            return SocketOpResultError;
        }
        // One datagram must carry the whole record. Read out in part, the record would arrive cut and its
        // tail would start the next datagram, which the receiver could not decrypt either.
        const size_t encryptedSize = BIO_ctrl_pending(dtlsEndpoint.m_writeBio);
        if (encryptedSize > sizeof(encrpytedSendBuffer))
        {
            AZLOG_ERROR("DTLS: a %u byte packet encrypts to %u bytes, more than a %u byte datagram; it is dropped "
                "(net_SslInflationOverhead is too small for this cipher)",
                size, aznumeric_cast<uint32_t>(encryptedSize), aznumeric_cast<uint32_t>(sizeof(encrpytedSendBuffer)));
            (void)BIO_reset(dtlsEndpoint.m_writeBio);
            return SocketOpResultError;
        }
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
