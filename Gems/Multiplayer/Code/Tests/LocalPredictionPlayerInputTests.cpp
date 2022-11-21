/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CommonBenchmarkSetup.h>
#include <CommonHierarchySetup.h>
#include <MockInterfaces.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/Name/Name.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Time/ITime.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Spawnable/SpawnableSystemComponent.h>
#include <AzNetworking/UdpTransport/UdpPacketHeader.h>
#include <AzNetworking/Framework/NetworkingSystemComponent.h>
#include <AzTest/AzTest.h>
#include <MultiplayerSystemComponent.h>
#include <IMultiplayerConnectionMock.h>
#include <IMultiplayerSpawnerMock.h>
#include <ConnectionData/ServerToClientConnectionData.h>
#include <ReplicationWindows/ServerToClientReplicationWindow.h>
#include <Multiplayer/Components/LocalPredictionPlayerInputComponent.h>
#include <Multiplayer/Components/NetBindComponent.h>
#include <Multiplayer/MultiplayerConstants.h>
#include <Multiplayer/Session/SessionConfig.h>

namespace Multiplayer
{
    class LocalPredictionPlayerInputTests : public AllocatorsFixture
    {
    public:
        void SetUp() override
        {
            AZ::NameDictionary::Create();

            m_ComponentApplicationRequests = AZStd::make_unique<BenchmarkComponentApplicationRequests>();
            AZ::Interface<AZ::ComponentApplicationRequests>::Register(m_ComponentApplicationRequests.get());

            m_console.reset(aznew AZ::Console());
            AZ::Interface<AZ::IConsole>::Register(m_console.get());

            m_timeSystem.reset();
            m_timeSystem = AZStd::make_unique<AZ::TimeSystem>();

            // register components involved in testing
            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
            m_behaviorContext = AZStd::make_unique<AZ::BehaviorContext>();
            m_transformDescriptor.reset(AzFramework::TransformComponent::CreateDescriptor());
            m_transformDescriptor->Reflect(m_serializeContext.get());
            m_netBindDescriptor.reset(NetBindComponent::CreateDescriptor());
            m_netBindDescriptor->Reflect(m_serializeContext.get());
            m_netTransformDescriptor.reset(NetworkTransformComponent::CreateDescriptor());
            m_netTransformDescriptor->Reflect(m_serializeContext.get());
            m_localPredictionDescriptor.reset(LocalPredictionPlayerInputComponent::CreateDescriptor());
            m_localPredictionDescriptor->Reflect(m_serializeContext.get());

            m_netComponent = new AzNetworking::NetworkingSystemComponent();
            m_mpComponent = new Multiplayer::MultiplayerSystemComponent();
            m_mpComponent->Reflect(m_serializeContext.get());
            m_mpComponent->Reflect(m_behaviorContext.get());

            m_mpComponent->Activate();
            m_eventScheduler = new AZ::EventSchedulerSystemComponent();
            m_eventScheduler->Reflect(m_serializeContext.get());
            m_eventScheduler->Activate();

            m_playerEntity = AZStd::make_unique<AZ::Entity>(AZ::EntityId(1), "Test");
            m_playerNetworkEntityTracker = AZStd::make_unique<NetworkEntityTracker>();
            m_playerEntity->CreateComponent<AzFramework::TransformComponent>();
            m_playerEntity->CreateComponent<NetworkTransformComponent>();
            m_localPredictionComponent = m_playerEntity->CreateComponent<LocalPredictionPlayerInputComponent>();
            const auto playerNetBindComponent = m_playerEntity->CreateComponent<NetBindComponent>();
            playerNetBindComponent->PreInit(
                m_playerEntity.get(), PrefabEntityId{ AZ::Name("test"), 1 }, NetEntityId{ 1 }, NetEntityRole::Autonomous);
            m_playerNetworkEntityTracker->RegisterNetBindComponent(m_playerEntity.get(), playerNetBindComponent);
            m_playerEntity->Init();
            m_playerEntity->Activate();
        }

        void TearDown() override
        {
            m_playerEntity->Deactivate();
            m_playerNetworkEntityTracker.reset();
            m_mpComponent->Deactivate();
            m_eventScheduler->Deactivate();
            m_localPredictionComponent = nullptr;
            m_playerEntity.reset();
            delete m_mpComponent;
            delete m_netComponent;
            delete m_eventScheduler;
            AZ::Interface<AZ::IConsole>::Unregister(m_console.get());
            m_console.reset();
            m_timeSystem.reset();
            AZ::Interface<AZ::ComponentApplicationRequests>::Unregister(m_ComponentApplicationRequests.get());
            m_ComponentApplicationRequests.reset();
            AZ::NameDictionary::Destroy();

            m_localPredictionDescriptor.reset();
            m_transformDescriptor.reset();
            m_netTransformDescriptor.reset();
            m_netBindDescriptor.reset();
            m_serializeContext.reset();
            m_behaviorContext.reset();
        }

        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::BehaviorContext> m_behaviorContext;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_transformDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_netBindDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_netTransformDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_localPredictionDescriptor;
        AZStd::unique_ptr<AZ::IConsole> m_console;
        AZStd::unique_ptr<AZ::TimeSystem> m_timeSystem;

        AzNetworking::NetworkingSystemComponent* m_netComponent = nullptr;
        Multiplayer::MultiplayerSystemComponent* m_mpComponent = nullptr;
        AZ::EventSchedulerSystemComponent* m_eventScheduler = nullptr;
        Multiplayer::LocalPredictionPlayerInputComponent* m_localPredictionComponent = nullptr;

        AZStd::unique_ptr<BenchmarkComponentApplicationRequests> m_ComponentApplicationRequests;

        AZStd::unique_ptr<AZ::Entity> m_playerEntity;
        AZStd::unique_ptr<NetworkEntityTracker> m_playerNetworkEntityTracker;

        IMultiplayerSpawnerMock m_mpSpawnerMock;
    };

    TEST_F(LocalPredictionPlayerInputTests, TestChildController)
    {
        m_mpComponent->InitializeMultiplayer(MultiplayerAgentType::DedicatedServer);
        EXPECT_EQ(m_mpComponent->GetAgentType(), MultiplayerAgentType::DedicatedServer);

        MultiplayerController* parentController = m_localPredictionComponent->GetController();
        LocalPredictionPlayerInputComponentController* controllerPtr =
            dynamic_cast<LocalPredictionPlayerInputComponentController*>(parentController);
        LocalPredictionPlayerInputComponentController childCotroller(*controllerPtr);
    }

    TEST_F(LocalPredictionPlayerInputTests, TestUpdateAutonomous)
    {
        m_mpComponent->InitializeMultiplayer(MultiplayerAgentType::DedicatedServer);
        EXPECT_EQ(m_mpComponent->GetAgentType(), MultiplayerAgentType::DedicatedServer);

        LocalPredictionPlayerInputComponentController* controller =
            dynamic_cast<LocalPredictionPlayerInputComponentController*>(m_localPredictionComponent->GetController());
        controller->ForceEnableAutonomousUpdate();
        AZ::Interface<AZ::ITime>::Get()->SetElapsedTimeMsDebug(AZ::TimeMs(1000));
        m_eventScheduler->OnTick(1000, AZ::ScriptTimePoint());
        controller->ForceDisableAutonomousUpdate();
    }

    TEST_F(LocalPredictionPlayerInputTests, TestHandleSendClientInput)
    {
        m_mpComponent->InitializeMultiplayer(MultiplayerAgentType::DedicatedServer);
        EXPECT_EQ(m_mpComponent->GetAgentType(), MultiplayerAgentType::DedicatedServer);

        IMultiplayerConnectionMock connection(
            ConnectionId{ 1 }, IpAddress("127.0.0.1", DefaultServerPort, ProtocolType::Udp), ConnectionRole::Connector);
        ServerToClientConnectionData connectionUserData(&connection, *m_mpComponent);
        connection.SetUserData(&connectionUserData);

        Multiplayer::NetworkInputArray netInputArray;
        netInputArray[0].SetHostBlendFactor(0.8f);
        netInputArray[0].SetHostTimeMs(AZ::TimeMs(1));
        netInputArray[0].SetHostFrameId(HostFrameId(1));
        AZ::HashValue32 dummyHash = AZ::HashValue32(0);

        LocalPredictionPlayerInputComponentController* controller =
            dynamic_cast<LocalPredictionPlayerInputComponentController*>(m_localPredictionComponent->GetController());
        controller->HandleSendClientInput(nullptr, netInputArray, dummyHash);
        controller->HandleSendClientInput(&connection, netInputArray, dummyHash);
        netInputArray[0].SetClientInputId(ClientInputId(1));
        // Force update to increment client input ID
        AZ::Interface<AZ::ITime>::Get()->SetElapsedTimeMsDebug(AZ::TimeMs(1000));
        controller->HandleSendClientInput(&connection, netInputArray, dummyHash);
        // Force update to update banked time
        AZ::Interface<AZ::ITime>::Get()->SetElapsedTimeMsDebug(AZ::TimeMs(1010));
        m_eventScheduler->OnTick(1000, AZ::ScriptTimePoint());

        EXPECT_EQ(controller->GetInputFrameId(netInputArray[0]), netInputArray[0].GetHostFrameId());
    }

    TEST_F(LocalPredictionPlayerInputTests, TestHandleSendClientInputCorrection)
    {
        m_mpComponent->InitializeMultiplayer(MultiplayerAgentType::DedicatedServer);
        EXPECT_EQ(m_mpComponent->GetAgentType(), MultiplayerAgentType::DedicatedServer);

        AzNetworking::PacketEncodingBuffer buffer;

        LocalPredictionPlayerInputComponentController* controller =
            dynamic_cast<LocalPredictionPlayerInputComponentController*>(m_localPredictionComponent->GetController());
        AZ_TEST_START_TRACE_SUPPRESSION;
        controller->HandleSendClientInputCorrection(nullptr, ClientInputId(1), buffer);
        AZ_TEST_STOP_TRACE_SUPPRESSION(2);

        IMultiplayerConnectionMock connection(
            ConnectionId{ 1 }, IpAddress("127.0.0.1", DefaultServerPort, ProtocolType::Udp), ConnectionRole::Connector);
        ServerToClientConnectionData connectionUserData(&connection, *m_mpComponent);
        connection.SetUserData(&connectionUserData);

        // Force update to increment client input id
        controller->ForceEnableAutonomousUpdate();
        AZ::Interface<AZ::ITime>::Get()->SetElapsedTimeMsDebug(AZ::TimeMs(1000));
        m_eventScheduler->OnTick(100, AZ::ScriptTimePoint());
        controller->HandleSendClientInputCorrection(&connection, ClientInputId(0), buffer);
        controller->HandleSendClientInputCorrection(&connection, ClientInputId(1), buffer);
    }

    TEST_F(LocalPredictionPlayerInputTests, TestHandleSendMigrateClientInput)
    {
        m_mpComponent->InitializeMultiplayer(MultiplayerAgentType::DedicatedServer);
        EXPECT_EQ(m_mpComponent->GetAgentType(), MultiplayerAgentType::DedicatedServer);

        IMultiplayerConnectionMock connection(
            ConnectionId{ 1 }, IpAddress("127.0.0.1", DefaultServerPort, ProtocolType::Udp), ConnectionRole::Connector);
        ServerToClientConnectionData connectionUserData(&connection, *m_mpComponent);
        connection.SetUserData(&connectionUserData);

        Multiplayer::NetworkInputArray netInputArray;
        netInputArray[0].SetHostBlendFactor(0.8f);
        netInputArray[0].SetHostTimeMs(AZ::TimeMs(1));
        netInputArray[0].SetHostFrameId(HostFrameId(1));
        Multiplayer::NetworkInputMigrationVector netMigrationVector;
        netMigrationVector.PushBack(netInputArray[0]);

        LocalPredictionPlayerInputComponentController* controller =
            dynamic_cast<LocalPredictionPlayerInputComponentController*>(m_localPredictionComponent->GetController());
        controller->OnDeactivate(EntityIsMigrating::False);
        controller->OnActivate(EntityIsMigrating::True);
        controller->HandleSendMigrateClientInput(&connection, netMigrationVector);
        controller->HandleSendMigrateClientInput(&connection, netMigrationVector);
        controller->OnDeactivate(EntityIsMigrating::False);
        controller->OnActivate(EntityIsMigrating::True);
        controller->HandleSendMigrateClientInput(nullptr, netMigrationVector);
    }
} // namespace Multiplayer
