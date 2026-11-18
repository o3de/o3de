/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/UdpTransport/UdpNetworkInterface.h>
#include <AzNetworking/ConnectionLayer/IConnectionListener.h>
#include <AzNetworking/Framework/NetworkingSystemComponent.h>
#include <AzCore/Console/LoggerSystemComponent.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/Time/TimeSystem.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    using namespace AzNetworking;

    namespace
    {
        class DisconnectEventListener final
            : public IConnectionListener
        {
        public:
            ConnectResult ValidateConnect(const IpAddress&, const IPacketHeader&, ISerializer&) override { return ConnectResult::Accepted; }
            void OnConnect(IConnection*) override {}
            PacketDispatchResult OnPacketReceived(IConnection*, const IPacketHeader&, ISerializer&) override { return PacketDispatchResult::Success; }
            void OnPacketLost(IConnection*, PacketId) override {}
            void OnDisconnect(IConnection*, DisconnectReason reason, TerminationEndpoint) override { m_listenerReason = reason; }

            DisconnectReason m_listenerReason = DisconnectReason::None;
        };

        //! What one interface's disconnect event reported.
        struct ReportedDisconnect
        {
            bool m_signalled = false;
            ConnectionId m_connectionId = InvalidConnectionId;
            DisconnectReason m_reason = DisconnectReason::None;
            TerminationEndpoint m_endpoint = TerminationEndpoint::Local;
            //! The listener's reason when the event fired: none yet, as the event comes first.
            DisconnectReason m_listenerReasonAtSignal = DisconnectReason::MAX;
        };
    } // namespace

    class UdpDisconnectEventTests
        : public LeakDetectionFixture
    {
    public:
        void SetUp() override
        {
            AZ::NameDictionary::Create();
            m_loggerComponent = AZStd::make_unique<AZ::LoggerSystemComponent>();
            m_timeSystem = AZStd::make_unique<AZ::TimeSystem>();
            m_networkingSystemComponent = AZStd::make_unique<AzNetworking::NetworkingSystemComponent>();
        }

        void TearDown() override
        {
            m_networkingSystemComponent.reset();
            m_timeSystem.reset();
            m_loggerComponent.reset();
            AZ::NameDictionary::Destroy();
        }

        template<typename Condition>
        bool TickUntil(Condition condition)
        {
            const AZ::TimeMs startTimeMs = AZ::GetElapsedTimeMs();
            while (!condition())
            {
                if (AZ::GetElapsedTimeMs() - startTimeMs > AZ::TimeMs{ 5000 })
                {
                    return false;
                }
                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(10));
                m_networkingSystemComponent->OnSystemTick();
            }
            return true;
        }

        AZStd::unique_ptr<AZ::LoggerSystemComponent> m_loggerComponent;
        AZStd::unique_ptr<AZ::TimeSystem> m_timeSystem;
        AZStd::unique_ptr<AzNetworking::NetworkingSystemComponent> m_networkingSystemComponent;
    };

    TEST_F(UdpDisconnectEventTests, BothEndsHearWhyAConnectionEndedBeforeTheirListeners)
    {
        constexpr uint16_t Port = 12371;
        INetworking* networking = AZ::Interface<INetworking>::Get();
        DisconnectEventListener serverListener;
        DisconnectEventListener clientListener;
        const AZ::Name serverName("DisconnectEventServer");
        const AZ::Name clientName("DisconnectEventClient");
        auto* server = static_cast<UdpNetworkInterface*>(
            networking->CreateNetworkInterface(serverName, ProtocolType::Udp, TrustZone::ExternalClientToServer, serverListener));
        auto* client = static_cast<UdpNetworkInterface*>(
            networking->CreateNetworkInterface(clientName, ProtocolType::Udp, TrustZone::ExternalClientToServer, clientListener));
        ASSERT_TRUE(server->Listen(Port));

        ReportedDisconnect serverReport;
        ReportedDisconnect clientReport;
        const auto record = [](ReportedDisconnect& report, const DisconnectEventListener& listener)
        {
            return UdpNetworkInterface::DisconnectEvent::Handler(
                [&report, &listener](ConnectionId connectionId, DisconnectReason reason, TerminationEndpoint endpoint)
                {
                    report = { true, connectionId, reason, endpoint, listener.m_listenerReason };
                });
        };
        UdpNetworkInterface::DisconnectEvent::Handler serverHandler = record(serverReport, serverListener);
        UdpNetworkInterface::DisconnectEvent::Handler clientHandler = record(clientReport, clientListener);
        server->AddDisconnectHandler(serverHandler);
        client->AddDisconnectHandler(clientHandler);

        const ConnectionId clientConnectionId = client->Connect(IpAddress(127, 0, 0, 1, Port));
        ASSERT_NE(clientConnectionId, InvalidConnectionId);
        ASSERT_TRUE(TickUntil([&]
        {
            return server->GetConnectionSet().GetConnectionCount() == 1 && client->GetConnectionSet().GetConnectionCount() == 1;
        }));

        // The server ends the session: the server reports its own end, the client the server's reason, from the far end.
        ConnectionId serverConnectionId = InvalidConnectionId;
        server->GetConnectionSet().VisitConnections([&serverConnectionId](IConnection& connection)
        {
            serverConnectionId = connection.GetConnectionId();
        });
        EXPECT_TRUE(server->Disconnect(serverConnectionId, DisconnectReason::TerminatedByServer));
        ASSERT_TRUE(TickUntil([&] { return serverReport.m_signalled && clientReport.m_signalled; }));

        EXPECT_EQ(serverReport.m_connectionId, serverConnectionId);
        EXPECT_EQ(serverReport.m_reason, DisconnectReason::TerminatedByServer);
        EXPECT_EQ(serverReport.m_endpoint, TerminationEndpoint::Local);
        EXPECT_EQ(clientReport.m_connectionId, clientConnectionId);
        EXPECT_EQ(clientReport.m_reason, DisconnectReason::TerminatedByServer);
        EXPECT_EQ(clientReport.m_endpoint, TerminationEndpoint::Remote);
        EXPECT_EQ(serverReport.m_listenerReasonAtSignal, DisconnectReason::None);
        EXPECT_EQ(clientReport.m_listenerReasonAtSignal, DisconnectReason::None);
        EXPECT_EQ(serverListener.m_listenerReason, DisconnectReason::TerminatedByServer);
        EXPECT_EQ(clientListener.m_listenerReason, DisconnectReason::TerminatedByServer);

        networking->DestroyNetworkInterface(clientName);
        networking->DestroyNetworkInterface(serverName);
    }
} // namespace UnitTest
