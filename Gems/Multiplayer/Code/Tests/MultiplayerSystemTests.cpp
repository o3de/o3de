/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/Name/Name.h>
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
            m_mpComponent = new Multiplayer::MultiplayerSystemComponent();

            m_initHandler = Multiplayer::SessionInitEvent::Handler([this](AzNetworking::INetworkInterface* value) { TestInitEvent(value); });
            m_mpComponent->AddSessionInitHandler(m_initHandler);
            m_shutdownHandler = Multiplayer::SessionInitEvent::Handler([this](AzNetworking::INetworkInterface* value) { TestShutdownEvent(value); });
            m_mpComponent->AddSessionShutdownHandler(m_shutdownHandler);
            m_connAcquiredHandler = Multiplayer::ConnectionAcquiredEvent::Handler([this](Multiplayer::MultiplayerAgentDatum value) { TestConnectionAcquiredEvent(value); });
            m_mpComponent->AddConnectionAcquiredHandler(m_connAcquiredHandler);
        }

        void TearDown() override
        {
            delete m_mpComponent;
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
        IMultiplayerConnectionMock connMock1 = IMultiplayerConnectionMock(AzNetworking::ConnectionId(), AzNetworking::IpAddress(), AzNetworking::ConnectionRole::Acceptor);
        IMultiplayerConnectionMock connMock2 = IMultiplayerConnectionMock(AzNetworking::ConnectionId(), AzNetworking::IpAddress(), AzNetworking::ConnectionRole::Connector);
        m_mpComponent->OnDisconnect(&connMock1, AzNetworking::DisconnectReason::None, AzNetworking::TerminationEndpoint::Local);
        m_mpComponent->OnDisconnect(&connMock2, AzNetworking::DisconnectReason::None, AzNetworking::TerminationEndpoint::Local);

        EXPECT_EQ(m_shutdownEventTriggerCount, 1);
    }

    TEST_F(MultiplayerSystemTests, TestConnectionDatum)
    {
        IMultiplayerConnectionMock connMock1 = IMultiplayerConnectionMock(aznumeric_cast<AzNetworking::ConnectionId>(10), AzNetworking::IpAddress(), AzNetworking::ConnectionRole::Acceptor);
        IMultiplayerConnectionMock connMock2 = IMultiplayerConnectionMock(aznumeric_cast<AzNetworking::ConnectionId>(15), AzNetworking::IpAddress(), AzNetworking::ConnectionRole::Acceptor);
        m_mpComponent->OnConnect(&connMock1);
        m_mpComponent->OnConnect(&connMock2);

        EXPECT_EQ(m_connectionAcquiredCount, 25);

        // Clean up connection data
        m_mpComponent->OnDisconnect(&connMock1, AzNetworking::DisconnectReason::None, AzNetworking::TerminationEndpoint::Local);
        m_mpComponent->OnDisconnect(&connMock2, AzNetworking::DisconnectReason::None, AzNetworking::TerminationEndpoint::Local);
    }
}

