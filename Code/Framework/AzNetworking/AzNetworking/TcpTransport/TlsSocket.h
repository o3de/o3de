/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzNetworking/TcpTransport/TcpSocket.h>
#include <AzNetworking/Utilities/EncryptionCommon.h>

namespace AzNetworking
{
    //! @class TlsSocket
    //! @brief wrapper class for managing encrypted Tcp sockets.
    class TlsSocket final
        : public TcpSocket
    {
    public:

        TlsSocket(TrustZone trustZone);

        //! Construct with an existing socket file descriptor.
        //! @param socketFd  existing socket file descriptor, this TlsSocket instance will assume ownership
        //! @param trustZone for encrypted connections, the level of trust we associate with this connection (internal or external)
        TlsSocket(SocketFd socketFd, TrustZone trustZone);

        ~TlsSocket();

        //! Returns true if this is an encrypted socket, false if not.
        //! @return boolean true if this is an encrypted socket, false if not
        bool IsEncrypted() const override;

        //! Creates a new socket instance, transferring all ownership from the current instance to the new instance.
        //! @return new socket instance
        TcpSocket* CloneAndTakeOwnership() override;

        //! Opens the TCP socket and binds it in listen mode.
        //! @param port the port number to open the TCP socket and begin listening on, 0 will bind to any available port
        //! @return boolean true on success
        bool Listen(uint16_t port) override;

        //! Opens the TCP socket and connects to the requested remote address.
        //! @param address the remote endpoint to connect to
        //! @return boolean true on success
        bool Connect(const IpAddress& address) override;

        //! Closes an open socket.
        void Close() override;

    protected:

        int32_t SendInternal(const uint8_t* data, uint32_t size) const override;
        int32_t ReceiveInternal(uint8_t* outData, uint32_t size) const override;

        SSL_CTX* m_sslContext;
        SSL*     m_sslSocket;

        TrustZone m_trustZone;
    };
}
