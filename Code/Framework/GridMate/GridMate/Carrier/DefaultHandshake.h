/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_DEFAULT_HANDSHAKE_H
#define GM_DEFAULT_HANDSHAKE_H

#include <GridMate/Carrier/Handshake.h>
#include <AzCore/std/string/string.h>

namespace GridMate
{
    /**
     * Default handshake interface.
     */
    class DefaultHandshake
        : public Handshake
    {
    public:
        GM_CLASS_ALLOCATOR(DefaultHandshake);

        DefaultHandshake(unsigned int timeOut, VersionType version);
        virtual ~DefaultHandshake();
        /// Called from the system to write initial handshake data.
        void                OnInitiate(ConnectionID id, WriteBuffer& wb) override;
        /**
        * Called when a system receives a handshake initiation from another system.
        * You can write a reply in the WriteBuffer.
        * return true if you accept this connection and false if you reject it.
        */
        HandshakeErrorCode  OnReceiveRequest(ConnectionID id, ReadBuffer& rb, WriteBuffer& wb) override;
        /**
        * If we already have a valid connection and we receive another connection request, the system will
        * call this function to verify the state of the connection.
        */
        bool                OnConfirmRequest(ConnectionID id, ReadBuffer& rb) override;
        /**
        * Called when we receive Ack from the other system on our initial data \ref OnInitiate.
        * return true to accept the ack or false to reject the handshake.
        */
        bool                OnReceiveAck(ConnectionID id, ReadBuffer& rb) override;
        /**
        * Called when we receive Ack from the other system while we were connected. This callback is called
        * so we can just confirm that our connection is valid!
        */
        bool                OnConfirmAck(ConnectionID id, ReadBuffer& rb) override;
        /// Return true if you want to reject early reject a connection.
        bool                OnNewConnection(const AZStd::string& address) override;
        /// Called when we close a connection.
        void                OnDisconnect(ConnectionID id) override;
        /// Return timeout in milliseconds of the handshake procedure.
        unsigned int        GetHandshakeTimeOutMS() const override       { return m_handshakeTimeOutMS; }
    private:
        unsigned int m_handshakeTimeOutMS;
        VersionType m_version;
    };
}

#endif // GM_DEFAULT_HANDSHAKE_H

