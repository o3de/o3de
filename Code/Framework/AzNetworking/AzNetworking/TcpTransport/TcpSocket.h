/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzNetworking/Utilities/IpAddress.h>
#include <AzNetworking/Utilities/NetworkCommon.h>
#include <AzNetworking/ConnectionLayer/IConnection.h>

namespace AzNetworking
{
    //! @class TcpSocket
    //! @brief wrapper class for managing TCP sockets.
    class TcpSocket
    {
    public:

        TcpSocket();

        //! Construct with an existing socket file descriptor.
        //! @param socketFd existing socket file descriptor, this TcpSocket instance will assume ownership
        TcpSocket(SocketFd socketFd);

        virtual ~TcpSocket();

        //! Creates a new socket instance, transferring all ownership from the current instance to the new instance.
        //! @return new socket instance
        virtual TcpSocket* CloneAndTakeOwnership();

        //! Returns true if this is an encrypted socket, false if not.
        //! @return boolean true if this is an encrypted socket, false if not
        virtual bool IsEncrypted() const;

        //! Opens the TCP socket and binds it in listen mode.
        //! @param port the port number to open the TCP socket and begin listening on, 0 will bind to any available port
        //! @return boolean true on success
        virtual bool Listen(uint16_t port);

        //! Opens the TCP socket and connects to the requested remote address.
        //! @param address the remote endpoint to connect to
        //! @return boolean true on success
        virtual bool Connect(const IpAddress& address);

        //! Closes an open socket.
        virtual void Close();

        //! Returns true if the socket is currently in an open state.
        //! @return boolean true if the socket is in a connected state
        bool IsOpen() const;

        //! Sets the underlying socket file descriptor.
        //! @param socketFd the new underlying socket file descriptor to use for this TcpSocket instance
        void SetSocketFd(SocketFd socketFd);

        //! Returns the underlying socket file descriptor.
        //! @return the underlying socket file descriptor
        SocketFd GetSocketFd() const;

        //! Sends a chunk of data to the connected endpoint.
        //! @param address the address to send the payload to
        //! @param data    pointer to the data to send
        //! @param size    size of the payload in bytes
        //! @return number of bytes sent, <= 0 on error
        int32_t Send(const uint8_t* data, uint32_t size) const;

        //! Receives a payload from the TCP socket.
        //! @param outAddress on success, the address of the endpoint that sent the data
        //! @param outData    on success, address to write the received data to
        //! @param size       maximum size the output buffer supports for receiving
        //! @return number of bytes received, <= 0 on error
        int32_t Receive(uint8_t* outData, uint32_t size) const;

    protected:

        virtual int32_t SendInternal(const uint8_t* data, uint32_t size) const;
        virtual int32_t ReceiveInternal(uint8_t* outData, uint32_t size) const;

        bool BindSocketForListenInternal(uint16_t port);
        bool BindSocketForConnectInternal(const IpAddress& address);
        bool SocketCreateInternal();

        SocketFd m_socketFd;
    };
}

#include <AzNetworking/TcpTransport/TcpSocket.inl>
