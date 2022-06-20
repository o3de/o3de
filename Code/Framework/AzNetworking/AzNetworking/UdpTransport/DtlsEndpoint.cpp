/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/UdpTransport/DtlsEndpoint.h>
#include <AzNetworking/UdpTransport/DtlsSocket.h>
#include <AzNetworking/UdpTransport/UdpConnection.h>
#include <AzNetworking/Utilities/EncryptionCommon.h>
#include <AzNetworking/Utilities/NetworkIncludes.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ILogger.h>

#if AZ_TRAIT_USE_OPENSSL
#   include <openssl/ssl.h>
#   include <openssl/err.h>
#endif

namespace AzNetworking
{
    AZ_CVAR(bool, net_UseDtlsCookies, false, nullptr, AZ::ConsoleFunctorFlags::Null, "Enables DTLS cookie exchange during the connection handshake");

    DtlsEndpoint::DtlsEndpoint()
        : m_state(HandshakeState::None)
        , m_sslSocket(nullptr)
        , m_readBio(nullptr)
        , m_writeBio(nullptr)
    {
        ;
    }

    DtlsEndpoint::~DtlsEndpoint()
    {
        Close(m_sslSocket); // Note this also closes any attached BIO instances
        m_readBio = nullptr;
        m_writeBio = nullptr;
    }

    DtlsEndpoint::ConnectResult DtlsEndpoint::Connect(const DtlsSocket& socket, const IpAddress& address, [[maybe_unused]] UdpPacketEncodingBuffer& outDtlsData)
    {
        const ConnectResult result = ConstructEndpointInternal(socket, address);
#if AZ_TRAIT_USE_OPENSSL
        if (result != ConnectResult::Failed)
        {
            // This SSL should be configured to initiate connections
            SSL_set_connect_state(m_sslSocket);
            m_state = HandshakeState::Connecting;
            return PerformHandshakeInternal(outDtlsData);
        }
#endif
        return result;
    }

    DtlsEndpoint::ConnectResult DtlsEndpoint::Accept(const DtlsSocket& socket, const IpAddress& address)
    {
        const ConnectResult result = ConstructEndpointInternal(socket, address);
#if AZ_TRAIT_USE_OPENSSL
        if (result != ConnectResult::Failed)
        {
            // This SSL should be configured to accept connections
            SSL_set_accept_state(m_sslSocket);
            m_state = HandshakeState::Accepting;
            return ConnectResult::Pending;
        }
#endif
        return result;
    }

    bool DtlsEndpoint::IsConnecting() const
    {
        return ((m_state == HandshakeState::Connecting)
             || (m_state == HandshakeState::Accepting)
             || (m_state == HandshakeState::Failed)); // In all cases caller should call CompleteHandshake() next and check the return value
    }

    DtlsEndpoint::ConnectResult DtlsEndpoint::ProcessHandshakeData([[maybe_unused]] UdpConnection& connection, [[maybe_unused]] const UdpPacketEncodingBuffer& dtlsData)
    {
        ConnectResult result = ConnectResult::Failed;
#if AZ_TRAIT_USE_OPENSSL
        UdpPacketEncodingBuffer outDtlsData;
        if (dtlsData.GetSize() > 0)
        {
            const uint8_t* encryptedData = dtlsData.GetBuffer();
            const uint32_t encryptedSize = static_cast<uint32_t>(dtlsData.GetSize());
            BIO_write(m_readBio, encryptedData, encryptedSize);
        }
        DtlsEndpoint::HandshakeState prevState = m_state;
        result = PerformHandshakeInternal(outDtlsData);

        // Pass along any remaining handshake data
        // If we're the connecting endpoint and the handshake is complete, both sides are encrypted and this isn't necessary so skip it
        bool continueHandshake = prevState != DtlsEndpoint::HandshakeState::Connecting || m_state != DtlsEndpoint::HandshakeState::Complete;
        if (outDtlsData.GetSize() > 0 && continueHandshake)
        {
            CorePackets::ConnectionHandshakePacket handshakePacket = CorePackets::ConnectionHandshakePacket();
            handshakePacket.SetHandshakeBuffer(outDtlsData);
            // SSL prefers we handle resend by reobtaining data through SSL so we use unreliable and resend on timeout
            connection.SendUnreliablePacket(handshakePacket);
        }
#endif

        return result;
    }

    const uint8_t* DtlsEndpoint::DecodePacket([[maybe_unused]] UdpConnection& connection, const uint8_t* encryptedData, int32_t encryptedSize, uint8_t* outDecodedData, int32_t& outDecodedSize)
    {
        if (m_sslSocket == nullptr || IsConnecting())
        {
            // If the ssl socket is nullptr, it means encryption is not enabled, just passthrough the received data
            // If the socket is connecting/handshaking, it means we can't yet encrypt, just passthrough the received data
            outDecodedSize = encryptedSize;
            return encryptedData;
        }
#if AZ_TRAIT_USE_OPENSSL

        int32_t bioWriteSize = BIO_write(m_readBio, encryptedData, encryptedSize);
        if (m_state != HandshakeState::Failed)
        {
            if (bioWriteSize != encryptedSize)
            {
                AZLOG_ERROR("BIO did not write as many bytes as provided");
            }
            outDecodedSize = SSL_read(m_sslSocket, outDecodedData, encryptedSize);
        }
#endif
        return outDecodedData;
    }

    DtlsEndpoint::ConnectResult DtlsEndpoint::ConstructEndpointInternal([[maybe_unused]] const DtlsSocket& socket, [[maybe_unused]] const IpAddress& address)
    {
        if (m_sslSocket != nullptr)
        {
            AZLOG_WARN("An existing SSL socket was open during a call to connect, closing old socket");
            Close(m_sslSocket); // Note this also closes any attached BIO instances
        }
#if AZ_TRAIT_USE_OPENSSL
        m_address = address;
        m_sslSocket = SSL_new(socket.m_sslContext);
        m_readBio = BIO_new(BIO_s_mem());
        BIO_set_mem_eof_return(m_readBio, -1);
        m_writeBio = BIO_new(BIO_s_mem());
        BIO_set_mem_eof_return(m_writeBio, -1);
        SSL_set_bio(m_sslSocket, m_readBio, m_writeBio);
        if (net_UseDtlsCookies)
        {
            SSL_set_options(m_sslSocket, SSL_OP_COOKIE_EXCHANGE);
        }
        if (m_sslSocket == nullptr)
        {
            AZLOG_ERROR("SSL_new failed, could not create SSL socket wrapper instance");
            PrintSslErrorStack();
            return ConnectResult::Failed;
        }
#endif
        return ConnectResult::Pending;
    }

    DtlsEndpoint::ConnectResult DtlsEndpoint::PerformHandshakeInternal([[maybe_unused]] UdpPacketEncodingBuffer& outHandshakeData)
    {
        if (m_state == HandshakeState::Failed)
        {
            return ConnectResult::Failed;
        }

        ConnectResult connectResult = ConnectResult::Pending;
#if AZ_TRAIT_USE_OPENSSL
        ERR_clear_error();
        const int32_t result = SSL_do_handshake(m_sslSocket);
        if (result <= 0)
        {
            const int32_t error = SSL_get_error(m_sslSocket, result);
            if ((error != SSL_ERROR_WANT_READ)
             && (error != SSL_ERROR_WANT_WRITE))
            {
                AZLOG_ERROR("SSL handshake negotiation failed (%d), terminating connection", error);
                PrintSslErrorStack();
                Close(m_sslSocket);
                m_readBio = nullptr;
                m_writeBio = nullptr;
                m_state = HandshakeState::Failed;
                connectResult = ConnectResult::Failed;
            }
        }

        if (SSL_is_init_finished(m_sslSocket))
        {
            const char* stateString = GetEnumString(m_state);
            AZLOG(NET_DebugDtls, "dtls handshake is completed, unblocking connection for game traffic, prior state: %s", stateString);
            m_state = HandshakeState::Complete;
            connectResult = ConnectResult::Complete;
        }

        // Need to do this... connection negotiation may have left data in the write bio that we need to send out
        if (BIO_ctrl_pending(m_writeBio) > 0)
        {
            const uint32_t maxBufferSize = static_cast<uint32_t>(outHandshakeData.GetCapacity());
            outHandshakeData.Resize(maxBufferSize);
            const int32_t dataSize = BIO_read(m_writeBio, outHandshakeData.GetBuffer(), maxBufferSize);
            outHandshakeData.Resize(dataSize);
        }
#else
        connectResult = ConnectResult::Complete;
#endif
        return connectResult;
    }

    const char* GetEnumString(DtlsEndpoint::HandshakeState value)
    {
        switch (value)
        {
        case DtlsEndpoint::HandshakeState::None:
            return "None";
        case DtlsEndpoint::HandshakeState::Connecting:
            return "Connecting";
        case DtlsEndpoint::HandshakeState::Accepting:
            return "Accepting";
        case DtlsEndpoint::HandshakeState::Complete:
            return "Complete";
        case DtlsEndpoint::HandshakeState::Failed:
            return "Failed";
        }
        return "UNKNOWN";
    }
}
