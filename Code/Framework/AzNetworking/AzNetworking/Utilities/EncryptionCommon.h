/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzNetworking/Utilities/IpAddress.h>
#include <AzNetworking/Utilities/NetworkCommon.h>
#include <AzNetworking/ConnectionLayer/IConnection.h>
#include <AzNetworking/AzNetworking_Traits_Platform.h>

// Forward declarations
typedef struct ssl_st SSL;
typedef struct ssl_ctx_st SSL_CTX;

// Common helper methods needed for both TLS and DTLS transport implementations
namespace AzNetworking
{
    // Constants that map to OpenSSL's success and failure return codes
    static const int32_t OpenSslResultFailure = 0;
    static const int32_t OpenSslResultSuccess = 1;

    //! Helper function to dump the OpenSSL error stack to the console.
    void PrintSslErrorStack();

    //! Initializes the encryption layer, required for TLS and DTLS usage.
    //! Should be called once at program initialization, prior to creating any encrypted network connections
    //! @return boolean true on success
    bool EncryptionLayerInit();

    //! Shuts down the encryption layer, required for TLS and DTLS usage.
    //! Should be called once at program halt, after stopping all encrypted network connections
    //! @return boolean true on success
    bool EncryptionLayerShutdown();

    enum class SslContextType
    {
        TlsGeneric  // Can initiate or accept connections using a streaming protocol (intended for Tcp)
    ,   TlsClient   // Initiates a connection to a server over a streaming protocol (intended for Tcp)
    ,   TlsServer   // Accepts connections from clients over a streaming protocol (intended for Tcp)
    ,   DtlsGeneric // Can initiate or accept connections over a datagram protocol, streaming ciphers are not valid (intended for Udp)
    ,   DtlsClient  // Initiates a connection to a server over a datagram protocol, streaming ciphers are not valid (intended for Udp)
    ,   DtlsServer  // Accepts connections from clients over a datagram protocol, streaming ciphers are not valid (intended for Udp)
    };

    //! Returns a new SSL context given a remote endpoints certificate, intended to initiate a connection to a secure remote endpoint.
    //! @param contextType the type of context to create (connection initiating or connection accepting, datagram or streaming)
    //! @param trustZone the level of trust we associate with this connection, used to determine which certificate file should be used (internal or external)
    //! @return pointer to the new SSL context, nullptr on error
    SSL_CTX* CreateSslContext(SslContextType contextType, TrustZone trustZone);

    //! Call to clean up an SSL context.
    //! @param context pointer to the context to clean up
    void FreeSslContext(SSL_CTX*& context);

    //! Accepts an incoming connection using the provided context.
    //! @param socketFd the socket file descriptor of the incoming connection
    //! @param context  the SSL context instance to use
    //! @return pointer to the new SSL socket instance, nullptr on error
    SSL* CreateSslForAccept(SocketFd socketFd, SSL_CTX* context);

    //! Initiates a secure connection to a remote endpoint using the provided context.
    //! @param socketFd the socket file descriptor to initiate connections on
    //! @param context  the SSL context instance to use
    //! @return pointer to the new SSL socket instance, nullptr on error
    SSL* CreateSslForConnect(SocketFd socketFd, SSL_CTX* context);

    //! Terminates and closes the provided SSL socket instance.
    //! @param sslSocket the SSL socket instance to close
    void Close(SSL*& sslSocket);

    //! Returns true if the platform specific error code maps to a 'would block' error.
    //! @return true if the platform specific error code maps to a 'would block' error
    bool SslErrorIsWouldBlock(int32_t errorCode);

    //! Returns a 32-bit random number using the crypto random generator.
    //! note that 4 bytes is a really small number of bytes for crypto purposes!
    //! @return 32-bit unsigned random number
    uint32_t CryptoRand32();

    //! Returns a 64-bit random number using the crypto random generator.
    //! @return 64-bit unsigned random number
    uint64_t CryptoRand64();
}
