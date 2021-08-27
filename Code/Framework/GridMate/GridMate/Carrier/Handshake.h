/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_HANDSHAKE_INTERFACE_H
#define GM_HANDSHAKE_INTERFACE_H

#include <GridMate/Types.h>
#include <GridMate/Serialize/Buffer.h>
#include <AzCore/std/string/string.h>

namespace GridMate
{
    enum class HandshakeErrorCode : int
    {
        OK = 0,
        REJECTED,
        PENDING,
        VERSION_MISMATCH,
    };

    /**
     * Handshake interface
     */
    class Handshake
    {
    public:
        virtual ~Handshake() {}
        /// Called from the system to write initial handshake data.
        virtual void                OnInitiate(ConnectionID id, WriteBuffer& wb) = 0;
        /**
         * Called when a system receives a handshake initiation from another system.
         * You can write a reply in the WriteBuffer.
         * return true if you accept this connection and false if you reject it.
         */
        virtual HandshakeErrorCode  OnReceiveRequest(ConnectionID id, ReadBuffer& rb, WriteBuffer& wb) = 0;
        /**
         * If we already have a valid connection and we receive another connection request, the system will
         * call this function to verify the state of the connection.
         */
        virtual bool                OnConfirmRequest(ConnectionID id, ReadBuffer& rb) = 0;
        /**
         * Called when we receive Ack from the other system on our initial data \ref OnInitiate.
         * return true to accept the ack or false to reject the handshake.
         */
        virtual bool                OnReceiveAck(ConnectionID id, ReadBuffer& rb) = 0;
        /**
         * Called when we receive Ack from the other system while we were connected. This callback is called
         * so we can just confirm that our connection is valid!
         */
        virtual bool                OnConfirmAck(ConnectionID id, ReadBuffer& rb) = 0;
        /// Return true if you want to reject early reject a connection.
        virtual bool                OnNewConnection(const AZStd::string& address) = 0;
        /// Called when we close a connection.
        virtual void                OnDisconnect(ConnectionID id) = 0;
        /// Return timeout in milliseconds of the handshake procedure.
        virtual unsigned int        GetHandshakeTimeOutMS() const = 0;
    };
}

#endif // GM_HANDSHAKE_INTERFACE_H

