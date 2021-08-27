/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzNetworking/ConnectionLayer/IConnection.h>
#include <gmock/gmock.h>

// The following is a mock of IConnection used to test endpoints in MultiplayerSystemComponent
// without an actual IConnection

using namespace AzNetworking;

class IMultiplayerConnectionMock : public IConnection
    {
    public:        
        IMultiplayerConnectionMock(ConnectionId connectionId, const IpAddress& address, ConnectionRole connectionRole)
            : IConnection(connectionId, address)
            , m_role(connectionRole)
        {
            ;
        }

        ~IMultiplayerConnectionMock() override{};

        MOCK_METHOD1(SendReliablePacket, bool(const IPacket&));

        MOCK_METHOD1(SendUnreliablePacket, PacketId(const IPacket&));

        MOCK_CONST_METHOD1(WasPacketAcked, bool(const PacketId));

        MOCK_CONST_METHOD0(GetConnectionState, ConnectionState());

        ConnectionRole GetConnectionRole() const override
        {
            return m_role;
        };

        MOCK_METHOD2(Disconnect, bool(DisconnectReason, TerminationEndpoint));

        MOCK_METHOD1(SetConnectionMtu, void(uint32_t));

        MOCK_CONST_METHOD0(GetConnectionMtu, uint32_t());

        MOCK_METHOD1(SetConnectionMtu, void(const ConnectionQuality&));

        MOCK_METHOD1(SetConnectionQuality, void(const ConnectionQuality&));

        ConnectionRole m_role;
    };

