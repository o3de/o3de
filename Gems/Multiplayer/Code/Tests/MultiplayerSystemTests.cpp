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
#include <AzCore/Jobs/JobManagerComponent.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/Name/Name.h>
#include <AzCore/RTTI/BehaviorContext.h>
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
#include <Multiplayer/Components/NetBindComponent.h>
#include <Multiplayer/MultiplayerConstants.h>
#include <Multiplayer/Session/SessionConfig.h>

namespace Multiplayer
{
    AZ_CVAR_EXTERNED(AZ::CVarFixedString, sv_map);
    AZ_CVAR_EXTERNED(bool, sv_versionMismatch_autoDisconnect);
    AZ_CVAR_EXTERNED(bool, sv_versionMismatch_sendManifestToClient);


    class MultiplayerSystemTests : public LeakDetectionFixture
    {
    public:
        void SetUp() override
        {
            AZ::NameDictionary::Create();

            m_ComponentApplicationRequests = AZStd::make_unique<BenchmarkComponentApplicationRequests>();
            AZ::Interface<AZ::ComponentApplicationRequests>::Register(m_ComponentApplicationRequests.get());

            m_mockTime = AZStd::make_unique<AZ::NiceTimeSystemMock>();
            m_mockLevelSystem = AZStd::make_unique<MockLevelSystemLifecycle>();

            m_console.reset(aznew AZ::Console());
            AZ::Interface<AZ::IConsole>::Register(m_console.get());

            // register components involved in testing
            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
            m_behaviorContext = AZStd::make_unique<AZ::BehaviorContext>();
            m_transformDescriptor.reset(AzFramework::TransformComponent::CreateDescriptor());
            m_transformDescriptor->Reflect(m_serializeContext.get());
            m_netBindDescriptor.reset(NetBindComponent::CreateDescriptor());
            m_netBindDescriptor->Reflect(m_serializeContext.get());
            m_jobComponentDescriptor.reset(AZ::JobManagerComponent::CreateDescriptor());
            m_jobComponentDescriptor->Reflect(m_serializeContext.get());

            m_netComponent = new AzNetworking::NetworkingSystemComponent();
            m_mpComponent = new Multiplayer::MultiplayerSystemComponent();
            m_mpComponent->Reflect(m_serializeContext.get());
            m_mpComponent->Reflect(m_behaviorContext.get());

            m_connAcquiredHandler = Multiplayer::ConnectionAcquiredEvent::Handler(
                [this](Multiplayer::MultiplayerAgentDatum value)
                {
                    TestConnectionAcquiredEvent(value);
                });
            m_mpComponent->AddConnectionAcquiredHandler(m_connAcquiredHandler);
            m_endpointDisconnectedHandler = Multiplayer::EndpointDisconnectedEvent::Handler(
                [this](Multiplayer::MultiplayerAgentType value)
                {
                    TestEndpointDisconnectedEvent(value);
                });
            m_mpComponent->AddEndpointDisconnectedHandler(m_endpointDisconnectedHandler);
            m_mpComponent->Activate();

            m_systemEntity = AZStd::make_unique<AZ::Entity>(AZ::EntityId(0));
            m_systemEntity->CreateComponent<AZ::JobManagerComponent>(); // Needed by Job system when @sv_multithreadedConnectionUpdates is on.
            m_systemEntity->Init();
            m_systemEntity->Activate();
        }

        void TearDown() override
        {
            m_systemEntity->Deactivate();
            m_systemEntity.reset();

            m_mpComponent->Deactivate();
            delete m_mpComponent;
            delete m_netComponent;
            AZ::Interface<AZ::IConsole>::Unregister(m_console.get());
            m_console.reset();
            m_mockTime.reset();
            m_mockLevelSystem.reset();
            AZ::Interface<AZ::ComponentApplicationRequests>::Unregister(m_ComponentApplicationRequests.get());
            m_ComponentApplicationRequests.reset();
            AZ::NameDictionary::Destroy();

            m_jobComponentDescriptor.reset();
            m_transformDescriptor.reset();
            m_netBindDescriptor.reset();
            m_serializeContext.reset();
            m_behaviorContext.reset();
        }

        void TestConnectionAcquiredEvent(Multiplayer::MultiplayerAgentDatum& datum)
        {
            m_connectionAcquiredCount += aznumeric_cast<uint32_t>(datum.m_id);
        }

        void TestEndpointDisconnectedEvent([[maybe_unused]] Multiplayer::MultiplayerAgentType value)
        {
            ++m_endpointDisconnectedCount;
        }

        void CreateAndRegisterNetBindComponent(
            AZ::Entity& playerEntity, NetworkEntityTracker& playerNetworkEntityTracker, NetEntityRole netEntityRole)
        {
            playerEntity.CreateComponent<AzFramework::TransformComponent>();
            const auto playerNetBindComponent = playerEntity.CreateComponent<NetBindComponent>();
            playerNetBindComponent->m_netEntityRole = netEntityRole;
            playerNetworkEntityTracker.RegisterNetBindComponent(&playerEntity, playerNetBindComponent);
            playerEntity.Init();
            playerEntity.Activate();
        }

        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::BehaviorContext> m_behaviorContext;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_transformDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_netBindDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_jobComponentDescriptor;
        AZStd::unique_ptr<AZ::IConsole> m_console;
        AZStd::unique_ptr<AZ::NiceTimeSystemMock> m_mockTime;
        AZStd::unique_ptr<AZ::Entity> m_systemEntity;

        class MockLevelSystemLifecycle : public AzFramework::ILevelSystemLifecycle
        {
        public:
            MockLevelSystemLifecycle()
            {
                AZ::Interface<AzFramework::ILevelSystemLifecycle>::Register(this);
            }

            ~MockLevelSystemLifecycle() override
            {
                AZ::Interface<AzFramework::ILevelSystemLifecycle>::Unregister(this);
            }

            const char* GetCurrentLevelName() const override
            {
                return m_levelName.c_str();
            }

            bool IsLevelLoaded() const override
            {
                return true;
            }

            AZStd::string m_levelName = "MockedMultiplayerLevelName";
        };

        AZStd::unique_ptr<MockLevelSystemLifecycle> m_mockLevelSystem;

        uint32_t m_connectionAcquiredCount = 0;
        uint32_t m_endpointDisconnectedCount = 0;

        Multiplayer::ConnectionAcquiredEvent::Handler m_connAcquiredHandler;
        Multiplayer::EndpointDisconnectedEvent::Handler m_endpointDisconnectedHandler;

        AzNetworking::NetworkingSystemComponent* m_netComponent = nullptr;
        Multiplayer::MultiplayerSystemComponent* m_mpComponent = nullptr;

        AZStd::unique_ptr<BenchmarkComponentApplicationRequests> m_ComponentApplicationRequests;

        IMultiplayerSpawnerMock m_mpSpawnerMock;
    };

    TEST_F(MultiplayerSystemTests, TestShutdownEvent)
    {
        m_mpComponent->InitializeMultiplayer(Multiplayer::MultiplayerAgentType::DedicatedServer);
        IMultiplayerConnectionMock connMock1 =
            IMultiplayerConnectionMock(AzNetworking::ConnectionId(), AzNetworking::IpAddress(), AzNetworking::ConnectionRole::Acceptor);
        IMultiplayerConnectionMock connMock2 =
            IMultiplayerConnectionMock(AzNetworking::ConnectionId(), AzNetworking::IpAddress(), AzNetworking::ConnectionRole::Connector);
        m_mpComponent->OnDisconnect(&connMock1, AzNetworking::DisconnectReason::None, AzNetworking::TerminationEndpoint::Local);
        m_mpComponent->OnDisconnect(&connMock2, AzNetworking::DisconnectReason::None, AzNetworking::TerminationEndpoint::Local);

        EXPECT_EQ(m_endpointDisconnectedCount, 2);
    }

    TEST_F(MultiplayerSystemTests, TestConnectionDatum)
    {
        using namespace testing;
        NiceMock<IMultiplayerConnectionMock> connMock1(
            aznumeric_cast<AzNetworking::ConnectionId>(10), AzNetworking::IpAddress(), AzNetworking::ConnectionRole::Acceptor);
        NiceMock<IMultiplayerConnectionMock> connMock2(
            aznumeric_cast<AzNetworking::ConnectionId>(15), AzNetworking::IpAddress(), AzNetworking::ConnectionRole::Acceptor);
        m_mpComponent->OnConnect(&connMock1);
        m_mpComponent->OnConnect(&connMock2);

        EXPECT_EQ(m_connectionAcquiredCount, 25);

        // Clean up connection data
        m_mpComponent->OnDisconnect(&connMock1, AzNetworking::DisconnectReason::None, AzNetworking::TerminationEndpoint::Local);
        m_mpComponent->OnDisconnect(&connMock2, AzNetworking::DisconnectReason::None, AzNetworking::TerminationEndpoint::Local);

        EXPECT_EQ(m_endpointDisconnectedCount, 2);
    }

    TEST_F(MultiplayerSystemTests, TestSpawnerEvents)
    {
        AZ::Interface<Multiplayer::IMultiplayerSpawner>::Register(&m_mpSpawnerMock);
        m_mpComponent->InitializeMultiplayer(Multiplayer::MultiplayerAgentType::ClientServer);

        AZ_TEST_START_TRACE_SUPPRESSION;
        // Setup mock connection and dummy connection data, this should raise two errors around entity validity
        Multiplayer::NetworkEntityHandle controlledEntity;
        IMultiplayerConnectionMock connMock =
            IMultiplayerConnectionMock(AzNetworking::ConnectionId(), AzNetworking::IpAddress(), AzNetworking::ConnectionRole::Acceptor);
        Multiplayer::ServerToClientConnectionData* connectionData =
            new Multiplayer::ServerToClientConnectionData(&connMock, *m_mpComponent);
        connectionData->GetReplicationManager().SetReplicationWindow(
            AZStd::make_unique<Multiplayer::ServerToClientReplicationWindow>(controlledEntity, &connMock));
        connMock.SetUserData(connectionData);

        m_mpComponent->OnDisconnect(&connMock, AzNetworking::DisconnectReason::None, AzNetworking::TerminationEndpoint::Local);
        AZ_TEST_STOP_TRACE_SUPPRESSION(2);

        EXPECT_EQ(m_endpointDisconnectedCount, 1);
        EXPECT_EQ(m_mpSpawnerMock.m_playerCount, 0);
        AZ::Interface<Multiplayer::IMultiplayerSpawner>::Unregister(&m_mpSpawnerMock);
    }

    TEST_F(MultiplayerSystemTests, TestClientServerConnectingWithoutPlayerEntity)
    {
        AZ::Interface<IMultiplayerSpawner>::Register(&m_mpSpawnerMock);

        m_mpSpawnerMock.m_networkEntityHandle = NetworkEntityHandle();
        EXPECT_FALSE(m_mpSpawnerMock.m_networkEntityHandle.Exists());

        m_mpComponent->InitializeMultiplayer(MultiplayerAgentType::ClientServer);
        EXPECT_EQ(m_mpSpawnerMock.m_playerEntityRequestedCount, 1);

        // We don't have a player entity yet, so MultiplayerSystemComponent should request another player entity when root spawnable (a new
        // level) is finished loading
        AzFramework::RootSpawnableNotificationBus::Broadcast(
            &AzFramework::RootSpawnableNotificationBus::Events::OnRootSpawnableReady, AZ::Data::Asset<AzFramework::Spawnable>(), 0);
        EXPECT_EQ(m_mpSpawnerMock.m_playerEntityRequestedCount, 2);

        AZ::Interface<IMultiplayerSpawner>::Unregister(&m_mpSpawnerMock);
    }

    TEST_F(MultiplayerSystemTests, TestClientServerConnectingWithPlayerEntity)
    {
        AZ::Interface<IMultiplayerSpawner>::Register(&m_mpSpawnerMock);

        // Setup a net player entity
        AZ::Entity playerEntity;
        NetworkEntityTracker playerNetworkEntityTracker;
        CreateAndRegisterNetBindComponent(playerEntity, playerNetworkEntityTracker, NetEntityRole::Authority);
        m_mpSpawnerMock.m_networkEntityHandle = NetworkEntityHandle(&playerEntity, &playerNetworkEntityTracker);
        EXPECT_TRUE(m_mpSpawnerMock.m_networkEntityHandle.Exists());

        // Initialize the ClientServer (this will also spawn a player for the host)
        m_mpComponent->InitializeMultiplayer(MultiplayerAgentType::ClientServer);
        EXPECT_EQ(m_mpSpawnerMock.m_playerEntityRequestedCount, 1);

        // Send a connection request. This should cause another player to be spawned.
        MultiplayerPackets::Connect connectPacket(
            0, 1, "connect_ticket", GetMultiplayerComponentRegistry()->GetSystemVersionHash());
        IMultiplayerConnectionMock connection(
            ConnectionId{ 1 }, IpAddress("127.0.0.1", DefaultServerPort, ProtocolType::Udp), ConnectionRole::Connector);
        ServerToClientConnectionData connectionUserData(&connection, *m_mpComponent);
        connection.SetUserData(&connectionUserData);

        m_mpComponent->HandleRequest(&connection, UdpPacketHeader(), connectPacket);

        EXPECT_EQ(m_mpSpawnerMock.m_playerEntityRequestedCount, 2);

        // Players should already be created and we should not request another player entity when root spawnable (a new
        // level) is finished loading
        AzFramework::RootSpawnableNotificationBus::Broadcast(
            &AzFramework::RootSpawnableNotificationBus::Events::OnRootSpawnableReady, AZ::Data::Asset<AzFramework::Spawnable>(), 0);
        EXPECT_EQ(m_mpSpawnerMock.m_playerEntityRequestedCount, 2); // player count is still 2 (stays the same)

        AZ::Interface<IMultiplayerSpawner>::Unregister(&m_mpSpawnerMock);
    }

    TEST_F(MultiplayerSystemTests, TestMultiplayerTick)
    {
        m_mpComponent->InitializeMultiplayer(MultiplayerAgentType::DedicatedServer);
        EXPECT_EQ(m_mpComponent->GetAgentType(), MultiplayerAgentType::DedicatedServer);

        IMultiplayerConnectionMock connection(
            ConnectionId{ 1 }, IpAddress("127.0.0.1", DefaultServerPort, ProtocolType::Udp), ConnectionRole::Connector);
        ServerToClientConnectionData connectionUserData(&connection, *m_mpComponent);
        connection.SetUserData(&connectionUserData);

         m_mpComponent->OnTick(1, AZ::ScriptTimePoint());
    }

    TEST_F(MultiplayerSystemTests, TestHandleAccept)
    {
        m_mpComponent->InitializeMultiplayer(MultiplayerAgentType::Client);
        EXPECT_EQ(m_mpComponent->GetAgentType(), MultiplayerAgentType::Client);

        IMultiplayerConnectionMock connection(
            ConnectionId{ 1 }, IpAddress("127.0.0.1", DefaultServerPort, ProtocolType::Udp), ConnectionRole::Connector);
        ServerToClientConnectionData connectionUserData(&connection, *m_mpComponent);
        connection.SetUserData(&connectionUserData);

        MultiplayerPackets::Accept accept;
        EXPECT_TRUE(m_mpComponent->HandleRequest(&connection, UdpPacketHeader(), accept));

        MultiplayerPackets::ClientMigration clientMigration;
        clientMigration.SetTemporaryUserIdentifier(1);
        EXPECT_TRUE(m_mpComponent->HandleRequest(&connection, UdpPacketHeader(), clientMigration));
        EXPECT_TRUE(m_mpComponent->HandleRequest(&connection, UdpPacketHeader(), accept));
        m_mpComponent->RequestPlayerLeaveSession();
        m_netComponent->ForceUpdate();
    }

    TEST_F(MultiplayerSystemTests, TestHandleReadyForEntityUpdate)
    {
        m_mpComponent->InitializeMultiplayer(MultiplayerAgentType::Client);
        EXPECT_EQ(m_mpComponent->GetAgentType(), MultiplayerAgentType::Client);

        IMultiplayerConnectionMock connection(
            ConnectionId{ 1 }, IpAddress("127.0.0.1", DefaultServerPort, ProtocolType::Udp), ConnectionRole::Connector);

        MultiplayerPackets::ReadyForEntityUpdates readyForEntityUpdates;
        EXPECT_FALSE(m_mpComponent->HandleRequest(&connection, UdpPacketHeader(), readyForEntityUpdates));
        ServerToClientConnectionData connectionUserData(&connection, *m_mpComponent);
        connection.SetUserData(&connectionUserData);
        EXPECT_TRUE(m_mpComponent->HandleRequest(&connection, UdpPacketHeader(), readyForEntityUpdates));
    }

    TEST_F(MultiplayerSystemTests, TestHandleClientMigrationFailOnServer)
    {
        m_mpComponent->InitializeMultiplayer(MultiplayerAgentType::DedicatedServer);
        EXPECT_EQ(m_mpComponent->GetAgentType(), MultiplayerAgentType::DedicatedServer);

        IMultiplayerConnectionMock connection(
            ConnectionId{ 1 }, IpAddress("127.0.0.1", DefaultServerPort, ProtocolType::Udp), ConnectionRole::Connector);
        MultiplayerPackets::ClientMigration clientMigration;
        EXPECT_FALSE(m_mpComponent->HandleRequest(&connection, UdpPacketHeader(), clientMigration));
    }

    TEST_F(MultiplayerSystemTests, TestHandleSyncConsole)
    {
        m_mpComponent->InitializeMultiplayer(MultiplayerAgentType::DedicatedServer);
        EXPECT_EQ(m_mpComponent->GetAgentType(), MultiplayerAgentType::DedicatedServer);

        IMultiplayerConnectionMock connection(
            ConnectionId{ 1 }, IpAddress("127.0.0.1", DefaultServerPort, ProtocolType::Udp), ConnectionRole::Connector);
        MultiplayerPackets::SyncConsole syncConsole;
        EXPECT_FALSE(m_mpComponent->HandleRequest(&connection, UdpPacketHeader(), syncConsole));

        m_mpComponent->InitializeMultiplayer(MultiplayerAgentType::Client);
        EXPECT_EQ(m_mpComponent->GetAgentType(), MultiplayerAgentType::Client);
        EXPECT_TRUE(m_mpComponent->HandleRequest(&connection, UdpPacketHeader(), syncConsole));
    }

    TEST_F(MultiplayerSystemTests, TestHandleConsoleCommand)
    {
        m_mpComponent->InitializeMultiplayer(MultiplayerAgentType::DedicatedServer);
        EXPECT_EQ(m_mpComponent->GetAgentType(), MultiplayerAgentType::DedicatedServer);

        IMultiplayerConnectionMock connection(
            ConnectionId{ 1 }, IpAddress("127.0.0.1", DefaultServerPort, ProtocolType::Udp), ConnectionRole::Connector);
        MultiplayerPackets::ConsoleCommand consoleCommand;
        EXPECT_TRUE(m_mpComponent->HandleRequest(&connection, UdpPacketHeader(), consoleCommand));

        m_mpComponent->InitializeMultiplayer(MultiplayerAgentType::Client);
        EXPECT_EQ(m_mpComponent->GetAgentType(), MultiplayerAgentType::Client);
        EXPECT_TRUE(m_mpComponent->HandleRequest(&connection, UdpPacketHeader(), consoleCommand));
    }

    TEST_F(MultiplayerSystemTests, TestHandleEntityUpdates)
    {
        m_mpComponent->InitializeMultiplayer(MultiplayerAgentType::Client);
        EXPECT_EQ(m_mpComponent->GetAgentType(), MultiplayerAgentType::Client);

        IMultiplayerConnectionMock connection(
            ConnectionId{ 1 }, IpAddress("127.0.0.1", DefaultServerPort, ProtocolType::Udp), ConnectionRole::Connector);

        MultiplayerPackets::EntityUpdates entityUpdates;
        EXPECT_TRUE(m_mpComponent->HandleRequest(&connection, UdpPacketHeader(), entityUpdates));

        ServerToClientConnectionData connectionUserData(&connection, *m_mpComponent);
        connection.SetUserData(&connectionUserData);

        entityUpdates.SetHostFrameId(HostFrameId(100));
        NetworkEntityUpdateVector entityUpdateVector;
        NetworkEntityUpdateMessage entityUpdateMessage;
        entityUpdateVector.push_back(entityUpdateMessage);
        entityUpdates.SetEntityMessages(entityUpdateVector);
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(m_mpComponent->HandleRequest(&connection, UdpPacketHeader(), entityUpdates));
        AZ_TEST_STOP_TRACE_SUPPRESSION(2);
    }

    TEST_F(MultiplayerSystemTests, TestHandleEntityRpcs)
    {
        m_mpComponent->InitializeMultiplayer(MultiplayerAgentType::Client);
        EXPECT_EQ(m_mpComponent->GetAgentType(), MultiplayerAgentType::Client);

        IMultiplayerConnectionMock connection(
            ConnectionId{ 1 }, IpAddress("127.0.0.1", DefaultServerPort, ProtocolType::Udp), ConnectionRole::Connector);

        MultiplayerPackets::EntityRpcs entityRpcs;
        EXPECT_TRUE(m_mpComponent->HandleRequest(&connection, UdpPacketHeader(), entityRpcs));

        ServerToClientConnectionData connectionUserData(&connection, *m_mpComponent);
        connection.SetUserData(&connectionUserData);

        EXPECT_TRUE(m_mpComponent->HandleRequest(&connection, UdpPacketHeader(), entityRpcs));
    }

    TEST_F(MultiplayerSystemTests, TestHandleRequestReplicatorReset)
    {
        m_mpComponent->InitializeMultiplayer(MultiplayerAgentType::Client);
        EXPECT_EQ(m_mpComponent->GetAgentType(), MultiplayerAgentType::Client);

        IMultiplayerConnectionMock connection(
            ConnectionId{ 1 }, IpAddress("127.0.0.1", DefaultServerPort, ProtocolType::Udp), ConnectionRole::Connector);

        MultiplayerPackets::RequestReplicatorReset replicatorReset;
        EXPECT_TRUE(m_mpComponent->HandleRequest(&connection, UdpPacketHeader(), replicatorReset));

        ServerToClientConnectionData connectionUserData(&connection, *m_mpComponent);
        connection.SetUserData(&connectionUserData);

        EXPECT_TRUE(m_mpComponent->HandleRequest(&connection, UdpPacketHeader(), replicatorReset));
    }

    TEST_F(MultiplayerSystemTests, TestGetTime)
    {
        m_mpComponent->InitializeMultiplayer(MultiplayerAgentType::Client);
        EXPECT_EQ(m_mpComponent->GetAgentType(), MultiplayerAgentType::Client);
        EXPECT_EQ(m_mpComponent->GetCurrentHostTimeMs(), AZ::Time::ZeroTimeMs);

        m_mpComponent->InitializeMultiplayer(MultiplayerAgentType::DedicatedServer);
        EXPECT_EQ(m_mpComponent->GetAgentType(), MultiplayerAgentType::DedicatedServer);
        EXPECT_EQ(m_mpComponent->GetCurrentHostTimeMs(), AZ::Time::ZeroTimeMs);

        EXPECT_EQ(m_mpComponent->GetNetworkTime()->GetHostTimeMs(), AZ::Time::ZeroTimeMs);
    }

    // Useful matchers to help sniff packets
    MATCHER_P(IsMultiplayerPacketType, packetType, "Checks an IPacket's packet type")
    {
        *result_listener << "where the packet type id is "
                         << ToString(static_cast<MultiplayerPackets::PacketType>(arg.GetPacketType())).data();
        return arg.GetPacketType() == packetType;
    }

    MATCHER_P(IsMismatchPacketWithComponentCount, totalComponentCount, "Checks how many multiplayer component versions are inside the VersionMismatch packet.")
    {
        if (arg.GetPacketType() != MultiplayerPackets::VersionMismatch::Type)
        {
            *result_listener << "where the packet is NOT a VersionMismatch packet";
            return false;
        }

        const uint32_t packetComponentCount = aznumeric_cast<uint32_t>(static_cast<const MultiplayerPackets::VersionMismatch&>(arg).GetComponentVersions().size());
        *result_listener << "where the packet is a VersionMismatch packet with component count " << packetComponentCount;
        return packetComponentCount == totalComponentCount;
    }

    TEST_F(MultiplayerSystemTests, TestConnectingWithoutLevelLoaded)
    {
        m_mpComponent->InitializeMultiplayer(MultiplayerAgentType::DedicatedServer);

        MultiplayerPackets::Connect connectPacket(
            0, 1, "connect_ticket", GetMultiplayerComponentRegistry()->GetSystemVersionHash());
        IMultiplayerConnectionMock connection(
            ConnectionId{ 1 }, IpAddress("127.0.0.1", DefaultServerPort, ProtocolType::Udp), ConnectionRole::Acceptor);
        ServerToClientConnectionData connectionUserData(&connection, *m_mpComponent);
        connection.SetUserData(&connectionUserData);

        // server doesn't have a level loaded, expect a disconnect
        EXPECT_CALL(connection, Disconnect(DisconnectReason::ServerNoLevelLoaded, TerminationEndpoint::Local));
        m_mockLevelSystem->m_levelName = "";
        m_mpComponent->HandleRequest(&connection, UdpPacketHeader(), connectPacket);

        AZ::Interface<IMultiplayerSpawner>::Unregister(&m_mpSpawnerMock);
    }

    TEST_F(MultiplayerSystemTests, TestConnectingWithMatchingComponentHash)
    {
        m_mpComponent->InitializeMultiplayer(MultiplayerAgentType::DedicatedServer);

        MultiplayerPackets::Connect connectPacket(
            0, 1, "connect_ticket", GetMultiplayerComponentRegistry()->GetSystemVersionHash());
        IMultiplayerConnectionMock connection(
            ConnectionId{ 1 }, IpAddress("127.0.0.1", DefaultServerPort, ProtocolType::Udp), ConnectionRole::Acceptor);
        ServerToClientConnectionData connectionUserData(&connection, *m_mpComponent);
        connection.SetUserData(&connectionUserData);

        // no mismatch, expect an acceptance packet
        m_mockLevelSystem->m_levelName = "dummylevel";
        EXPECT_CALL(connection, SendReliablePacket(IsMultiplayerPacketType(MultiplayerPackets::Accept::Type)));
        m_mpComponent->HandleRequest(&connection, UdpPacketHeader(), connectPacket);

        AZ::Interface<IMultiplayerSpawner>::Unregister(&m_mpSpawnerMock);
    }

    TEST_F(MultiplayerSystemTests, TestConnectingWithMismatchComponentHash)
    {
        // cvars affecting mismatch behavior:
        //   1. sv_versionMismatch_autoDisconnect
        //   2. sv_versionMismatch_sendManifestToClient

        m_mockLevelSystem->m_levelName = "dummylevel";
        m_mpComponent->InitializeMultiplayer(MultiplayerAgentType::DedicatedServer);

        // Send a connection request with a different component hash to trigger a mismatch
        const AZ::HashValue64 differentMultiplayerComponentHash = AZ::HashValue64{ 42 };
        MultiplayerPackets::Connect connectPacket(0, 1, "connect_ticket", differentMultiplayerComponentHash);
        IMultiplayerConnectionMock connection(
            ConnectionId{ 1 }, IpAddress("127.0.0.1", DefaultServerPort, ProtocolType::Udp), ConnectionRole::Acceptor);
        ServerToClientConnectionData connectionUserData(&connection, *m_mpComponent);
        connection.SetUserData(&connectionUserData);

        // Mismatch, send client a mismatch packet will all our components
        sv_versionMismatch_sendManifestToClient = true;
        const auto ourMultiplayerComponentCount = aznumeric_cast<uint32_t>(GetMultiplayerComponentRegistry()->GetMultiplayerComponentVersionHashes().size());
        EXPECT_CALL(connection, SendReliablePacket(IsMismatchPacketWithComponentCount(ourMultiplayerComponentCount))).Times(1);
        m_mpComponent->HandleRequest(&connection, UdpPacketHeader(), connectPacket);

        // Mismatch, send client a mismatch packet but don't send all our components to the client
        sv_versionMismatch_sendManifestToClient = false;
        EXPECT_CALL(connection, SendReliablePacket(IsMismatchPacketWithComponentCount(uint32_t{ 0 }))).Times(1);
        m_mpComponent->HandleRequest(&connection, UdpPacketHeader(), connectPacket);

        // Test the client sending components back to the server
        // Receive a multiplayer version mismatch packet and disconnect
        sv_versionMismatch_autoDisconnect = true;
        MultiplayerPackets::VersionMismatch mismatchPacket;
        EXPECT_CALL(connection, Disconnect(DisconnectReason::VersionMismatch, TerminationEndpoint::Local)).Times(1);
        m_mpComponent->HandleRequest(&connection, UdpPacketHeader(), mismatchPacket);

        // Receive a multiplayer version mismatch packet and but don't disconnect and instead accept
        sv_versionMismatch_autoDisconnect = false;
        EXPECT_CALL(connection, Disconnect).Times(0);
        EXPECT_CALL(connection, SendReliablePacket(IsMultiplayerPacketType(MultiplayerPackets::Accept::Type))).Times(1);
        m_mpComponent->HandleRequest(&connection, UdpPacketHeader(), mismatchPacket);

        AZ::Interface<IMultiplayerSpawner>::Unregister(&m_mpSpawnerMock);
    }

    TEST_F(MultiplayerSystemTests, TestMiscellaneous)
    {
        m_mpComponent->DumpStats({});
        m_mpComponent->SetShouldSpawnNetworkEntities(true);
        EXPECT_TRUE(m_mpComponent->GetShouldSpawnNetworkEntities());
        EXPECT_EQ(m_mpComponent->GetTickOrder(), AZ::TICK_PLACEMENT + 1);
        EXPECT_TRUE(m_mpComponent->OnSessionHealthCheck());
        m_mpComponent->RegisterPlayerIdentifierForRejoin(0, NetEntityId(0));
        m_mpComponent->OnCreateSessionEnd();
        m_mpComponent->OnDestroySessionEnd();
        Multiplayer::SessionConfig sessionConfig;

        m_mpComponent->OnUpdateSessionBegin(sessionConfig, "");
        m_mpComponent->OnUpdateSessionEnd();
        EXPECT_EQ(m_mpComponent->GetCurrentBlendFactor(), 0.0f);

        IMultiplayerConnectionMock connection(
            ConnectionId{ 1 }, IpAddress("127.0.0.1", DefaultServerPort, ProtocolType::Udp), ConnectionRole::Connector);
        ServerToClientConnectionData connectionUserData(&connection, *m_mpComponent);
        connection.SetUserData(&connectionUserData);
        EXPECT_FALSE(m_mpComponent->IsHandshakeComplete(&connection));
    }
} // namespace Multiplayer
