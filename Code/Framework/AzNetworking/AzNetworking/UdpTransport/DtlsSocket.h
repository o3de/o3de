/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzNetworking/UdpTransport/UdpSocket.h>
#include <AzNetworking/ConnectionLayer/IConnection.h>
#include <AzNetworking/Utilities/EncryptionCommon.h>

namespace AzNetworking
{
    class DtlsEndpoint;

    //! @class DtlsSocket
    //! @brief wrapper class for managing encrypted Udp sockets.
    class DtlsSocket final
        : public UdpSocket
    {
        friend class DtlsEndpoint;

    public:

        DtlsSocket() = default;
        ~DtlsSocket();

        //! Returns true if this is an encrypted socket, false if not.
        //! @return boolean true if this is an encrypted socket, false if not
        bool IsEncrypted() const override;

        //! Creates an encryption socket wrapper.
        //! @param dtlsEndpoint the encryption wrapper instance to create a connection over
        //! @param address      the IP address of the endpoint to connect to
        //! @param outDtlsData  data buffer to store the dtls handshake packet
        //! @return a connect result specifying whether the connection is still pending, failed, or complete
        DtlsEndpoint::ConnectResult ConnectDtlsEndpoint(DtlsEndpoint& dtlsEndpoint, const IpAddress& address, UdpPacketEncodingBuffer& outDtlsData) const override;

        //! Accepts an encryption socket wrapper.
        //! @param dtlsEndpoint the encryption wrapper instance to create a connection over
        //! @param address      the IP address of the endpoint to connect to
        //! @return a connect result specifying whether the connection is still pending, failed, or complete
        DtlsEndpoint::ConnectResult AcceptDtlsEndpoint(DtlsEndpoint& dtlsEndpoint, const IpAddress& address) const override;

        //! Opens the UDP socket on the given port.
        //! @param port      the port number to open the UDP socket on, 0 will bind to any available port
        //! @param canAccept if true, the socket will be opened in a way that allows accepting incoming connections
        //! @param trustZone for encrypted connections, the level of trust we associate with this connection (internal or external)
        //! @return boolean true on success
        bool Open(uint16_t port, UdpSocket::CanAcceptConnections canAccept, TrustZone trustZone) override;

        //! Closes an open socket.
        void Close() override;

    private:

        int32_t SendInternal(const IpAddress& address, const uint8_t* data, uint32_t size, bool encrypt, DtlsEndpoint& dtlsEndpoint) const override;

        SSL_CTX* m_sslContext = nullptr;
    };
}
