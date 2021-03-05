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

#include <AzNetworking/UdpTransport/DtlsEndpoint.h>
#include <AzNetworking/UdpTransport/DtlsSocket.h>
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

    DtlsEndpoint::ConnectResult DtlsEndpoint::Accept(const DtlsSocket& socket, const IpAddress& address, const UdpPacketEncodingBuffer& dtlsData)
    {
        if (dtlsData.GetSize() <= 0)
        {
            AZLOG_WARN("Encryption is enabled on accepting endpoint, but connector provided an empty DTLS handshake blob.  Check that encryption is properly disabled on *BOTH* endpoints");
            return DtlsEndpoint::ConnectResult::Failed;
        }

        const ConnectResult result = ConstructEndpointInternal(socket, address);
#if AZ_TRAIT_USE_OPENSSL
        if (result != ConnectResult::Failed)
        {
            // This SSL should be configured to accept connections
            SSL_set_accept_state(m_sslSocket);
            m_state = HandshakeState::Accepting;
            const uint8_t* encryptedData = dtlsData.GetBuffer();
            const uint32_t encryptedSize = dtlsData.GetSize();
            BIO_write(m_readBio, encryptedData, encryptedSize);
            return CompleteHandshake(socket);
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

    DtlsEndpoint::ConnectResult DtlsEndpoint::CompleteHandshake(const UdpSocket& socket)
    {
        UdpPacketEncodingBuffer responseData;
        const ConnectResult result = PerformHandshakeInternal(responseData);
        if ((result != ConnectResult::Failed) && (responseData.GetSize() > 0))
        {
            struct sockaddr_in dest;
            memset(&dest, 0, sizeof(dest));
            dest.sin_family = AF_INET;
            dest.sin_addr.s_addr = m_address.GetAddress(ByteOrder::Network);
            dest.sin_port = m_address.GetPort(ByteOrder::Network);
            sendto(static_cast<int32_t>(socket.GetSocketFd()), reinterpret_cast<char*>(responseData.GetBuffer()), responseData.GetSize(), 0, (sockaddr*)&dest, sizeof(dest));
            AZLOG(NET_DebugDtls, "Replying to DTLS handshake datagram, %u bytes", static_cast<int32_t>(responseData.GetSize()));
        }

        return result;
    }

    const uint8_t* DtlsEndpoint::DecodePacket
    (
        [[maybe_unused]] const UdpSocket& socket,
        [[maybe_unused]] const uint8_t* encryptedData,
        [[maybe_unused]] int32_t encryptedSize,
        [[maybe_unused]] uint8_t* outDecodedData,
        [[maybe_unused]] int32_t& outDecodedSize
    )
    {
        if (m_sslSocket == nullptr)
        {
            // If the ssl socket is nullptr, it means encryption is not enabled, just passthrough the received data
            outDecodedSize = encryptedSize;
            return encryptedData;
        }
#if AZ_TRAIT_USE_OPENSSL
        BIO_write(m_readBio, encryptedData, encryptedSize);
        if (IsConnecting())
        {
            CompleteHandshake(socket);
            outDecodedSize = 0;
        }
        // CompleteHandshake() above may have failed and destroyed the SSL context, check here that state is valid so we don't crash on SSL_read
        if (m_state != HandshakeState::Failed)
        {
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
        if (SSL_is_init_finished(m_sslSocket))
        {
            const char* stateString = GetEnumString(m_state);
            AZLOG(NET_DebugDtls, "dtls handshake is completed, unblocking connection for game traffic, prior state: %s", stateString);
            m_state = HandshakeState::Complete;
            connectResult = ConnectResult::Complete;
        }
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

        // Need to do this... connection negotiation may have left data in the write bio that we need to send out
        if (BIO_ctrl_pending(m_writeBio) > 0)
        {
            const uint32_t maxBufferSize = outHandshakeData.GetCapacity();
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
