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

#include <AzNetworking/Utilities/IpAddress.h>
#include <AzNetworking/DataStructures/ByteBuffer.h>

// OpenSSL forward declarations
typedef struct ssl_st SSL;
typedef struct ssl_ctx_st SSL_CTX;
typedef struct bio_st BIO;

namespace AzNetworking
{
    class UdpSocket;
    class DtlsSocket;

    //! @class DtlsEndpoint
    //! @brief Helper class defining an encrypted DTLS endpoint.
    //! Note that multiple connections are multiplexed onto a single DTLS socket
    class DtlsEndpoint final
    {
        friend class DtlsSocket;

    public:

        enum class ConnectResult
        {
            Failed, 
            Pending, 
            Complete
        };

        enum class HandshakeState
        {
            None,       // Not an active dtls endpoint, Connect has not been called
            Connecting, // This is a connecting endpoint, initiating the connection
            Accepting,  // This is an accepting endpoint
            Complete,   // Handshake is complete, connection is established and encrypted
            Failed      // Handshake failed
        };

        DtlsEndpoint();
        ~DtlsEndpoint();

        //! Opens a connection with the remote encrypted endpoint.
        //! @param socket      the dtls socket being used for data transmission
        //! @param address     the address of the remote endpoint being connected to
        //! @param outDtlsData data buffer to store the dtls handshake packet
        //! @return a connect result specifying whether the connection is still pending, failed, or complete
        ConnectResult Connect(const DtlsSocket& socket, const IpAddress& address, UdpPacketEncodingBuffer& outDtlsData);

        //! Accepts a connection from the remote encrypted endpoint.
        //! @param socket   the dtls socket being used for data transmission
        //! @param address  the address of the remote endpoint connecting to us
        //! @param dtlsData data buffer containing the initial dtls handshake packet
        //! @return a connect result specifying whether the connection is still pending, failed, or complete
        ConnectResult Accept(const DtlsSocket& socket, const IpAddress& address, const UdpPacketEncodingBuffer& dtlsData);

        //! Returns whether or not the endpoint is still negotiating the dtls handshake.
        //! @return true if the endpoint is still in a connecting state
        bool IsConnecting() const;

        //! Attempts to complete the dtls handshake and establish an encrypted connection.
        //! @param socket the dtls socket being used for data transmission
        //! @return a connect result specifying whether the connection is still pending, failed, or complete
        ConnectResult CompleteHandshake(const UdpSocket& socket);

        //! If the endpoint has encryption enabled, this will decrypt the transmitted data and return the result.
        //! @note sizes have to be signed since OpenSSL often returns negative values to represent error results
        //! @param socket         the DTLS socket being used for data transmission
        //! @param encryptedData  the potentially encrypted data received from the socket
        //! @param encryptedSize  the size of the received raw data
        //! @param outDecodedData an appropriately sized output buffer to store decrypted data
        //! @param outDecodedSize the size of the output buffer
        //! @return pointer to the decoded data
        const uint8_t* DecodePacket(const UdpSocket& socket, const uint8_t* encryptedData, int32_t encryptedSize, uint8_t* outDecodedData, int32_t& outDecodedSize);

    private:

        //! Performs internal common dtls endpoint setup.
        //! @param socket  the dtls socket being used for data transmission
        //! @param address the address of the remote endpoint connecting to us
        //! @return a connect result specifying whether the connection is still pending, failed, or complete
        ConnectResult ConstructEndpointInternal(const DtlsSocket& socket, const IpAddress& address);

        //! Attempts to complete the dtls handshake and establish an encrypted connection.
        //! @param outHandshakeData buffer to store any required outgoing dtls handshake data
        //! @return a connect result specifying whether the connection is still pending, failed, or complete
        ConnectResult PerformHandshakeInternal(UdpPacketEncodingBuffer& outHandshakeData);

        HandshakeState m_state;
        IpAddress m_address;
        SSL* m_sslSocket;
        BIO* m_readBio;
        BIO* m_writeBio;
    };

    const char* GetEnumString(DtlsEndpoint::HandshakeState value);
}
