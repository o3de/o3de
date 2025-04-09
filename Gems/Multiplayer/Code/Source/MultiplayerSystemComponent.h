/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Multiplayer/IMultiplayer.h>
#include <Multiplayer/Session/ISessionHandlingRequests.h>
#include <Multiplayer/Session/SessionNotifications.h>
#include <Editor/MultiplayerEditorConnection.h>
#include <NetworkTime/NetworkTime.h>
#include <NetworkEntity/NetworkEntityManager.h>
#include <Source/AutoGen/Multiplayer.AutoPacketDispatcher.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Threading/ThreadSafeDeque.h>
#include <AzCore/std/string/string.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Physics/Common/PhysicsEvents.h>
#include <AzNetworking/ConnectionLayer/IConnectionListener.h>

// This can be overridden in the build files by defining O3DE_EDITOR_CONNECTION_LISTENER_ENABLE to 0 or 1
// to force disabling or enabling the listener. But by default, it will be enabled in non-monolithic builds
// and disabled in monolithic builds.
#if !defined(O3DE_EDITOR_CONNECTION_LISTENER_ENABLE)
    #if !defined(AZ_MONOLITHIC_BUILD)
        #define O3DE_EDITOR_CONNECTION_LISTENER_ENABLE 1
    #endif
#endif

namespace AzFramework
{
    struct SessionConfig;
}

namespace AzNetworking
{
    class INetworkInterface;
}

namespace Multiplayer
{
    //! Multiplayer system component wraps the bridging logic between the game and transport layer.
    class MultiplayerSystemComponent final
        : public AZ::Component
        , public AZ::TickBus::Handler
        , public SessionNotificationBus::Handler
        , public ISessionHandlingClientRequests
        , public AzNetworking::IConnectionListener
        , public IMultiplayer
        , AzFramework::RootSpawnableNotificationBus::Handler
        , AzFramework::LevelLoadBlockerBus::Handler
    {
    public:
        AZ_COMPONENT(MultiplayerSystemComponent, "{7C99C4C1-1103-43F9-AD62-8B91CF7C1981}");

        static void Reflect(AZ::ReflectContext* context);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        MultiplayerSystemComponent();
        ~MultiplayerSystemComponent() override;

        //! AZ::Component overrides.
        //! @{
        void Activate() override;
        void Deactivate() override;
        //! @}

        //! SessionNotificationBus::Handler overrides.
        //! @{
        bool OnSessionHealthCheck() override;
        bool OnCreateSessionBegin(const SessionConfig& sessionConfig) override;
        void OnCreateSessionEnd() override;
        bool OnDestroySessionBegin() override;
        void OnDestroySessionEnd() override;
        void OnUpdateSessionBegin(const SessionConfig& sessionConfig, const AZStd::string& updateReason) override;
        void OnUpdateSessionEnd() override;
        //! @}

        //! AZ::TickBus::Handler overrides.
        //! @{
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        int GetTickOrder() override;
        //! @}

        bool IsHandshakeComplete(AzNetworking::IConnection* connection) const;
        bool HandleRequest(AzNetworking::IConnection* connection, const AzNetworking::IPacketHeader& packetHeader, MultiplayerPackets::Connect& packet);
        bool HandleRequest(AzNetworking::IConnection* connection, const AzNetworking::IPacketHeader& packetHeader, MultiplayerPackets::Accept& packet);
        bool HandleRequest(AzNetworking::IConnection* connection, const AzNetworking::IPacketHeader& packetHeader, MultiplayerPackets::ReadyForEntityUpdates& packet);
        bool HandleRequest(AzNetworking::IConnection* connection, const AzNetworking::IPacketHeader& packetHeader, MultiplayerPackets::SyncConsole& packet);
        bool HandleRequest(AzNetworking::IConnection* connection, const AzNetworking::IPacketHeader& packetHeader, MultiplayerPackets::ConsoleCommand& packet);
        bool HandleRequest(AzNetworking::IConnection* connection, const AzNetworking::IPacketHeader& packetHeader, MultiplayerPackets::EntityUpdates& packet);
        bool HandleRequest(AzNetworking::IConnection* connection, const AzNetworking::IPacketHeader& packetHeader, MultiplayerPackets::EntityRpcs& packet);
        bool HandleRequest(AzNetworking::IConnection* connection, const AzNetworking::IPacketHeader& packetHeader, MultiplayerPackets::RequestReplicatorReset& packet);
        bool HandleRequest(AzNetworking::IConnection* connection, const AzNetworking::IPacketHeader& packetHeader, MultiplayerPackets::ClientMigration& packet);
        bool HandleRequest(AzNetworking::IConnection* connection, const AzNetworking::IPacketHeader& packetHeader, MultiplayerPackets::VersionMismatch& packet);

        //! IConnectionListener interface
        //! @{
        AzNetworking::ConnectResult ValidateConnect(const AzNetworking::IpAddress& remoteAddress, const AzNetworking::IPacketHeader& packetHeader, AzNetworking::ISerializer& serializer) override;
        void OnConnect(AzNetworking::IConnection* connection) override;
        AzNetworking::PacketDispatchResult OnPacketReceived(AzNetworking::IConnection* connection, const AzNetworking::IPacketHeader& packetHeader, AzNetworking::ISerializer& serializer) override;
        void OnPacketLost(AzNetworking::IConnection* connection, AzNetworking::PacketId packetId) override;
        void OnDisconnect(AzNetworking::IConnection* connection, AzNetworking::DisconnectReason reason, AzNetworking::TerminationEndpoint endpoint) override;
        //! @}

        //! ISessionHandlingClientRequests interface
        //! @{
        bool RequestPlayerJoinSession(const SessionConnectionConfig& sessionConnectionConfig) override;
        void RequestPlayerLeaveSession() override;
        //! @}

        //! IMultiplayer interface
        //! @{
        MultiplayerAgentType GetAgentType() const override;
        void InitializeMultiplayer(MultiplayerAgentType state) override;
        bool StartHosting(uint16_t port = UseDefaultHostPort, bool isDedicated = true) override;
        bool Connect(const AZStd::string& remoteAddress, uint16_t port, const AZStd::string& connectionTicket = "") override;
        void Terminate(AzNetworking::DisconnectReason reason) override;
        void AddClientMigrationStartEventHandler(ClientMigrationStartEvent::Handler& handler) override;
        void AddClientMigrationEndEventHandler(ClientMigrationEndEvent::Handler& handler) override;
        void AddEndpointDisconnectedHandler(EndpointDisconnectedEvent::Handler& handler) override;
        void AddNotifyClientMigrationHandler(NotifyClientMigrationEvent::Handler& handler) override;
        void AddNotifyEntityMigrationEventHandler(NotifyEntityMigrationEvent::Handler& handler) override;
        void AddConnectionAcquiredHandler(ConnectionAcquiredEvent::Handler& handler) override;
        void AddNetworkInitHandler(NetworkInitEvent::Handler& handler) override;

        //! @deprecated If looking for an event when a multiplayer session is created, use SessionNotificationBus::OnCreateSessionBegin or
        //! SessionNotificationBus::OnCreateSessionEnd.
        void AddSessionInitHandler(SessionInitEvent::Handler&) override {}

        //! @deprecated If looking for an event when the multiplayer session ends, use SessionNotificationBus::OnDestroySessionBegin or
        //! SessionNotificationBus::OnDestroySessionEnd.
        void AddSessionShutdownHandler(SessionShutdownEvent::Handler&) override {}

        void AddLevelLoadBlockedHandler(LevelLoadBlockedEvent::Handler& handler) override;
        void AddNoServerLevelLoadedHandler(NoServerLevelLoadedEvent::Handler& handler) override;
        void AddVersionMismatchHandler(VersionMismatchEvent::Handler& handler) override;
        void AddServerAcceptanceReceivedHandler(ServerAcceptanceReceivedEvent::Handler& handler) override;
        void SendNotifyClientMigrationEvent(AzNetworking::ConnectionId connectionId, const HostId& hostId, uint64_t userIdentifier, ClientInputId lastClientInputId, NetEntityId controlledEntityId) override;
        void SendNotifyEntityMigrationEvent(const ConstNetworkEntityHandle& entityHandle, const HostId& remoteHostId) override;
        void SendReadyForEntityUpdates(bool readyForEntityUpdates) override;
        AZ::TimeMs GetCurrentHostTimeMs() const override;
        float GetCurrentBlendFactor() const override;
        INetworkTime* GetNetworkTime() override;
        INetworkEntityManager* GetNetworkEntityManager() override;
        void RegisterPlayerIdentifierForRejoin(uint64_t temporaryUserIdentifier, NetEntityId controlledEntityId) override;
        void CompleteClientMigration(uint64_t temporaryUserIdentifier, AzNetworking::ConnectionId connectionId, const HostId& publicHostId, ClientInputId migratedClientInputId) override;
        void SetShouldSpawnNetworkEntities(bool value) override;
        bool GetShouldSpawnNetworkEntities() const override;
        //! @}

        //! Console commands.
        //! @{
        void DumpStats(const AZ::ConsoleCommandContainer& arguments);
        //! @}

        //! AzFramework::RootSpawnableNotificationBus::Handler
        //! @{
        void OnRootSpawnableAssigned(AZ::Data::Asset<AzFramework::Spawnable> rootSpawnable, uint32_t generation) override;
        void OnRootSpawnableReady(AZ::Data::Asset<AzFramework::Spawnable> rootSpawnable, uint32_t generation) override;
        void OnRootSpawnableReleased(uint32_t generation) override;
        //! @}

        //! AzFramework::LevelLoadBlockerBus::Handler overrides.
        //! @{
        bool ShouldBlockLevelLoading(const char* levelName) override;
        //! @}

    private:
        bool IsHosting() const;

        bool AttemptPlayerConnect(AzNetworking::IConnection* connection, MultiplayerPackets::Connect& packet);
        void TickVisibleNetworkEntities(float deltaTime, float serverRateSeconds);
        void OnConsoleCommandInvoked(AZStd::string_view command, const AZ::ConsoleCommandContainer& args, AZ::ConsoleFunctorFlags flags, AZ::ConsoleInvokedFrom invokedFrom);
        void OnAutonomousEntityReplicatorCreated();
        void ExecuteConsoleCommandList(AzNetworking::IConnection* connection, const AZStd::fixed_vector<Multiplayer::LongNetworkString, 32>& commands);
        static void EnableAutonomousControl(NetworkEntityHandle entityHandle, AzNetworking::ConnectionId ownerConnectionId);
        static void StartServerToClientReplication(uint64_t userId, NetworkEntityHandle controlledEntity, AzNetworking::IConnection* connection);

        AZ_CONSOLEFUNC(MultiplayerSystemComponent, DumpStats, AZ::ConsoleFunctorFlags::Null, "Dumps stats for the current multiplayer session");
        void HostConsoleCommand(const AZ::ConsoleCommandContainer& arguments);
        void ConnectConsoleCommand(const AZ::ConsoleCommandContainer& arguments);

        AZStd::unique_ptr<AZ::ConsoleFunctor<MultiplayerSystemComponent, false>> m_hostConsoleCommand;
        AZStd::unique_ptr<AZ::ConsoleFunctor<MultiplayerSystemComponent, false>> m_connectConsoleCommand;


        AzNetworking::INetworkInterface* m_networkInterface = nullptr;
        AZ::ConsoleCommandInvokedEvent::Handler m_consoleCommandHandler;
        AZ::ThreadSafeDeque<AZStd::string> m_cvarCommands;

        NetworkEntityManager m_networkEntityManager;
        NetworkTime m_networkTime;
        MultiplayerAgentType m_agentType = MultiplayerAgentType::Uninitialized;
        
        IFilterEntityManager* m_filterEntityManager = nullptr; // non-owning pointer

        ConnectionAcquiredEvent m_connectionAcquiredEvent;
        NetworkInitEvent m_networkInitEvent;
        ServerAcceptanceReceivedEvent m_serverAcceptanceReceivedEvent;
        EndpointDisconnectedEvent m_endpointDisconnectedEvent;
        ClientMigrationStartEvent m_clientMigrationStartEvent;
        ClientMigrationEndEvent m_clientMigrationEndEvent;
        NotifyClientMigrationEvent m_notifyClientMigrationEvent;
        NotifyEntityMigrationEvent m_notifyEntityMigrationEvent;
        LevelLoadBlockedEvent m_levelLoadBlockedEvent;
        NoServerLevelLoadedEvent m_noServerLevelLoadedEvent;
        VersionMismatchEvent m_versionMismatchEvent;

        AZ::Event<NetEntityId>::Handler m_autonomousEntityReplicatorCreatedHandler;
        AZ::SettingsRegistryInterface::NotifyEventHandler m_componentApplicationLifecycleHandler;

        AZStd::queue<AZStd::string> m_pendingConnectionTickets;
        AZStd::unordered_map<uint64_t, NetEntityId> m_playerRejoinData;

        AZ::TimeMs m_lastReplicatedHostTimeMs = AZ::Time::ZeroTimeMs;
        HostFrameId m_lastReplicatedHostFrameId = HostFrameId{0};

        uint64_t m_temporaryUserIdentifier = 0; // Used in the event of a migration or rejoin

        double m_serverSendAccumulator = 0.0;
        float m_renderBlendFactor = 0.0f;
        float m_tickFactor = 0.0f;
        bool m_spawnNetboundEntities = false;

        // Store player information if they connect before there is a level and IPlayerSpawner available
        struct PlayerWaitingToBeSpawned
        {
            PlayerWaitingToBeSpawned(uint64_t userId, MultiplayerAgentDatum agent, AzNetworking::IConnection* connection)
                : userId(userId), agent(agent), connection(connection)
            {
            }

            uint64_t userId;
            MultiplayerAgentDatum agent;
            AzNetworking::IConnection* connection = nullptr;
        };

        AZStd::vector<PlayerWaitingToBeSpawned> m_playersWaitingToBeSpawned;
        bool m_blockClientLoadLevel = true;
        bool m_levelEntitiesActivated = false;

        AZStd::unordered_map<AzNetworking::ConnectionId, MultiplayerPackets::Connect> m_originalConnectPackets;

        void RegisterMetrics();
        void MetricsEvent();
        AZ::ScheduledEvent m_metricsEvent{ [this]()
        {
            MetricsEvent();
        }, AZ::Name("MultiplayerSystemComponent Metrics") };

        void UpdatedMetricsConnectionCount();

        void UpdateConnections();

        void OnPhysicsPreSimulate(float dt);
        AzPhysics::SystemEvents::OnPresimulateEvent::Handler m_preSimulateHandler{[this](float dt)
        {
            OnPhysicsPreSimulate(dt);
        }};
        AZStd::chrono::steady_clock::time_point m_startPhysicsTickTime;
        void OnPhysicsPostSimulate(float dt);
        AzPhysics::SystemEvents::OnPostsimulateEvent::Handler m_postSimulateHandler{ [this](float dt)
        {
            OnPhysicsPostSimulate(dt);
        } };

        // By default, this is only enabled in non-monolithic builds, since the Editor doesn't support monolithic builds.
#if (O3DE_EDITOR_CONNECTION_LISTENER_ENABLE)
        // This is a unique_ptr instead of a raw instance so that we can defer the construction
        // until the Activate(). If it gets constructed during the MultiplayerSystemComponent constructor,
        // the AzNetworking systems might not be constructed and activated yet, which would crash.
        AZStd::unique_ptr<MultiplayerEditorConnection> m_editorConnectionListener;
#endif
    };
}
