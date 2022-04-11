/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/TcpTransport/TcpNetworkInterface.h>
#include <AzNetworking/Framework/NetworkingSystemComponent.h>
#include <AzNetworking/AutoGen/CorePackets.AutoPackets.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Console/LoggerSystemComponent.h>
#include <AzCore/Time/TimeSystem.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    using namespace AzNetworking;

    class TestTcpConnectionListener
        : public IConnectionListener
    {
    public:
        ConnectResult ValidateConnect([[maybe_unused]] const IpAddress& remoteAddress, [[maybe_unused]] const IPacketHeader& packetHeader, [[maybe_unused]] ISerializer& serializer) override
        {
            return ConnectResult::Accepted;
        }

        void OnConnect([[maybe_unused]] IConnection* connection) override
        {
            ;
        }

        PacketDispatchResult OnPacketReceived([[maybe_unused]] IConnection* connection, const IPacketHeader& packetHeader, [[maybe_unused]] ISerializer& serializer) override
        {
            EXPECT_TRUE((packetHeader.GetPacketType() == static_cast<PacketType>(CorePackets::PacketType::InitiateConnectionPacket))
                     || (packetHeader.GetPacketType() == static_cast<PacketType>(CorePackets::PacketType::HeartbeatPacket)));
            return PacketDispatchResult::Failure;
        }

        void OnPacketLost([[maybe_unused]] IConnection* connection, [[maybe_unused]] PacketId packetId) override
        {

        }

        void OnDisconnect([[maybe_unused]] IConnection* connection, [[maybe_unused]] DisconnectReason reason, [[maybe_unused]] TerminationEndpoint endpoint) override
        {

        }
    };
}
