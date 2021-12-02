/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/Name/Name.h>
#include <AzFramework/Spawnable/SpawnableSystemComponent.h>
#include <AzNetworking/Framework/NetworkingSystemComponent.h>
#include <AzTest/AzTest.h>
#include <MultiplayerSystemComponent.h>
#include <IMultiplayerConnectionMock.h>

namespace UnitTest
{
    class MultiplayerSystemTests
        : public AllocatorsFixture
    {
    public:
        void SetUp() override
        {
            SetupAllocator();
            AZ::NameDictionary::Create();
            m_netComponent = new AzNetworking::NetworkingSystemComponent();
            m_mpComponent = new Multiplayer::MultiplayerSystemComponent();

            m_initHandler = Multiplayer::SessionInitEvent::Handler([this](AzNetworking::INetworkInterface* value) { TestInitEvent(value); });
            m_mpComponent->AddSessionInitHandler(m_initHandler);
            m_shutdownHandler = Multiplayer::SessionShutdownEvent::Handler([this](AzNetworking::INetworkInterface* value) { TestShutdownEvent(value); });
            m_mpComponent->AddSessionShutdownHandler(m_shutdownHandler);
            m_connAcquiredHandler = Multiplayer::ConnectionAcquiredEvent::Handler([this](Multiplayer::MultiplayerAgentDatum value) { TestConnectionAcquiredEvent(value); });
            m_mpComponent->AddConnectionAcquiredHandler(m_connAcquiredHandler);
            m_mpComponent->Activate();
        }

        void TearDown() override
        {
            m_mpComponent->Deactivate();
            delete m_mpComponent;
            delete m_netComponent;
            AZ::NameDictionary::Destroy();
            TeardownAllocator();
        }

        void TestInitEvent([[maybe_unused]] AzNetworking::INetworkInterface* network)
        {
            ++m_initEventTriggerCount;
        }

        void TestShutdownEvent([[maybe_unused]] AzNetworking::INetworkInterface* network)
        {
            ++m_shutdownEventTriggerCount;
        }

        void TestConnectionAcquiredEvent(Multiplayer::MultiplayerAgentDatum& datum)
        {
            m_connectionAcquiredCount += aznumeric_cast<uint32_t>(datum.m_id);
        }

        uint32_t m_initEventTriggerCount = 0;
        uint32_t m_shutdownEventTriggerCount = 0;
        uint32_t m_connectionAcquiredCount = 0;

        Multiplayer::SessionInitEvent::Handler m_initHandler;
        Multiplayer::SessionShutdownEvent::Handler m_shutdownHandler;
        Multiplayer::ConnectionAcquiredEvent::Handler m_connAcquiredHandler;

        AzNetworking::NetworkingSystemComponent* m_netComponent = nullptr;
        Multiplayer::MultiplayerSystemComponent* m_mpComponent = nullptr;
    };

    TEST_F(MultiplayerSystemTests, TestInitEvent)
    {
        m_mpComponent->InitializeMultiplayer(Multiplayer::MultiplayerAgentType::DedicatedServer);
        m_mpComponent->InitializeMultiplayer(Multiplayer::MultiplayerAgentType::ClientServer);
        m_mpComponent->InitializeMultiplayer(Multiplayer::MultiplayerAgentType::Client);
        EXPECT_EQ(m_initEventTriggerCount, 1);
    }

    TEST_F(MultiplayerSystemTests, TestShutdownEvent)
    {
        m_mpComponent->InitializeMultiplayer(Multiplayer::MultiplayerAgentType::DedicatedServer);
        IMultiplayerConnectionMock connMock1 = IMultiplayerConnectionMock(AzNetworking::ConnectionId(), AzNetworking::IpAddress(), AzNetworking::ConnectionRole::Acceptor);
        IMultiplayerConnectionMock connMock2 = IMultiplayerConnectionMock(AzNetworking::ConnectionId(), AzNetworking::IpAddress(), AzNetworking::ConnectionRole::Connector);
        m_mpComponent->OnDisconnect(&connMock1, AzNetworking::DisconnectReason::None, AzNetworking::TerminationEndpoint::Local);
        m_mpComponent->OnDisconnect(&connMock2, AzNetworking::DisconnectReason::None, AzNetworking::TerminationEndpoint::Local);

        EXPECT_EQ(m_shutdownEventTriggerCount, 1);
    }

    TEST_F(MultiplayerSystemTests, TestConnectionDatum)
    {
        using namespace testing;
        NiceMock<IMultiplayerConnectionMock> connMock1(aznumeric_cast<AzNetworking::ConnectionId>(10), AzNetworking::IpAddress(), AzNetworking::ConnectionRole::Acceptor);
        NiceMock<IMultiplayerConnectionMock> connMock2(aznumeric_cast<AzNetworking::ConnectionId>(15), AzNetworking::IpAddress(), AzNetworking::ConnectionRole::Acceptor);
        m_mpComponent->OnConnect(&connMock1);
        m_mpComponent->OnConnect(&connMock2);

        EXPECT_EQ(m_connectionAcquiredCount, 25);

        // Clean up connection data
        m_mpComponent->OnDisconnect(&connMock1, AzNetworking::DisconnectReason::None, AzNetworking::TerminationEndpoint::Local);
        m_mpComponent->OnDisconnect(&connMock2, AzNetworking::DisconnectReason::None, AzNetworking::TerminationEndpoint::Local);
    }
}

