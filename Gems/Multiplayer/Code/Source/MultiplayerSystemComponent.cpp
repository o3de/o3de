/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Multiplayer/MultiplayerConstants.h>
#include <Multiplayer/Components/MultiplayerComponent.h>
#include <MultiplayerSystemComponent.h>
#include <ConnectionData/ClientToServerConnectionData.h>
#include <ConnectionData/ServerToClientConnectionData.h>
#include <EntityDomains/FullOwnershipEntityDomain.h>
#include <ReplicationWindows/NullReplicationWindow.h>
#include <ReplicationWindows/ServerToClientReplicationWindow.h>
#include <Source/AutoGen/AutoComponentTypes.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ILogger.h>
#include <AzCore/Math/ShapeIntersection.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/Components/CameraBus.h>
#include <AzFramework/Session/ISessionRequests.h>
#include <AzFramework/Session/SessionConfig.h>
#include <AzFramework/Spawnable/Spawnable.h>
#include <AzFramework/Visibility/IVisibilitySystem.h>
#include <AzFramework/Visibility/EntityBoundsUnionBus.h>

#include <AzNetworking/Framework/INetworking.h>

#include <cmath>

namespace AZ::ConsoleTypeHelpers
{
    template <>
    inline CVarFixedString ValueToString(const AzNetworking::ProtocolType& value)
    {
        return (value == AzNetworking::ProtocolType::Tcp) ? "tcp" : "udp";
    }

    template <>
    inline bool StringSetToValue<AzNetworking::ProtocolType>(AzNetworking::ProtocolType& outValue, const ConsoleCommandContainer& arguments)
    {
        if (!arguments.empty())
        {
            if (arguments.front() == "tcp")
            {
                outValue = AzNetworking::ProtocolType::Tcp;
                return true;
            }
            else if (arguments.front() == "udp")
            {
                outValue = AzNetworking::ProtocolType::Udp;
                return true;
            }
        }
        return false;
    }
}

namespace Multiplayer
{
    using namespace AzNetworking;

    AZ_CVAR(uint16_t, cl_clientport, 0, nullptr, AZ::ConsoleFunctorFlags::DontReplicate,
        "The port to bind to for game traffic when connecting to a remote host, a value of 0 will select any available port");
    AZ_CVAR(AZ::CVarFixedString, cl_serveraddr, AZ::CVarFixedString(LocalHost), nullptr, AZ::ConsoleFunctorFlags::DontReplicate,
        "The address of the remote server or host to connect to");
    AZ_CVAR(uint16_t, cl_serverport, DefaultServerPort, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "The port of the remote host to connect to for game traffic");
    AZ_CVAR(uint16_t, sv_port, DefaultServerPort, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "The port that this multiplayer gem will bind to for game traffic");
    AZ_CVAR(AZ::CVarFixedString, sv_map, "nolevel", nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "The map the server should load");
    AZ_CVAR(ProtocolType, sv_protocol, ProtocolType::Udp, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "This flag controls whether we use TCP or UDP for game networking");
    AZ_CVAR(bool, sv_isDedicated, true, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Whether the host command creates an independent or client hosted server");
    AZ_CVAR(bool, sv_isTransient, true, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Whether a dedicated server shuts down if all existing connections disconnect.");
    AZ_CVAR(AZ::TimeMs, cl_defaultNetworkEntityActivationTimeSliceMs, AZ::TimeMs{ 0 }, nullptr, AZ::ConsoleFunctorFlags::DontReplicate,
        "Max Ms to use to activate entities coming from the network, 0 means instantiate everything");
    AZ_CVAR(AZ::TimeMs, sv_serverSendRateMs, AZ::TimeMs{ 50 }, nullptr, AZ::ConsoleFunctorFlags::Null, "Minimum number of milliseconds between each network update");
    AZ_CVAR(AZ::CVarFixedString, sv_defaultPlayerSpawnAsset, "prefabs/player.network.spawnable", nullptr, AZ::ConsoleFunctorFlags::DontReplicate,
        "The default spawnable to use when a new player connects");
    AZ_CVAR(float, cl_renderTickBlendBase, 0.15f, nullptr, AZ::ConsoleFunctorFlags::Null,
        "The base used for blending between network updates, 0.1 will be quite linear, 0.2 or 0.3 will "
        "slow down quicker and may be better suited to connections with highly variable latency");

    void MultiplayerSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MultiplayerSystemComponent, AZ::Component>()
                ->Version(1);

            serializeContext->Class<HostId>()
                ->Version(1);
            serializeContext->Class<NetEntityId>()
                ->Version(1);
            serializeContext->Class<NetComponentId>()
                ->Version(1);
            serializeContext->Class<PropertyIndex>()
                ->Version(1);
            serializeContext->Class<RpcIndex>()
                ->Version(1);
            serializeContext->Class<ClientInputId>()
                ->Version(1);
            serializeContext->Class<HostFrameId>()
                ->Version(1);
        }
        else if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<HostId>();
            behaviorContext->Class<NetEntityId>();
            behaviorContext->Class<NetComponentId>();
            behaviorContext->Class<PropertyIndex>();
            behaviorContext->Class<RpcIndex>();
            behaviorContext->Class<ClientInputId>();
            behaviorContext->Class<HostFrameId>();

            behaviorContext->Class<MultiplayerSystemComponent>("MultiplayerSystemComponent")
                ->Attribute(AZ::Script::Attributes::Module, "multiplayer")
                ->Attribute(AZ::Script::Attributes::Category, "Multiplayer")
                ->Method("GetOnClientDisconnectedEvent", [](AZ::EntityId id) -> AZ::Event<>*
                {
                    AZ::Entity* entity = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->FindEntity(id);
                    if (!entity)
                    {
                        AZ_Warning("Multiplayer Property", false, "MultiplayerSystemComponent GetOnClientDisconnectedEvent failed. The entity with id %s doesn't exist, please provide a valid entity id.", id.ToString().c_str())
                        return nullptr;
                    }

                    MultiplayerSystemComponent* mpComponent = entity->FindComponent<MultiplayerSystemComponent>();
                    if (!mpComponent)
                    {
                        AZ_Warning("Multiplayer Property", false, "MultiplayerSystemComponent GetOnClientDisconnected failed. Entity '%s' (id: %s) is missing MultiplayerSystemComponent, be sure to add MultiplayerSystemComponent to this entity.", entity->GetName().c_str(), id.ToString().c_str())
                        return nullptr;
                    }

                    return &mpComponent->m_clientDisconnectedEvent;
                })
                ->Attribute(
                    AZ::Script::Attributes::AzEventDescription,
                    AZ::BehaviorAzEventDescription{"On Client Disconnected Event"});
        }

        MultiplayerComponent::Reflect(context);
    }

    void MultiplayerSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("NetworkingService"));
    }

    void MultiplayerSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("MultiplayerService"));
    }

    void MultiplayerSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("MultiplayerService"));
    }

    MultiplayerSystemComponent::MultiplayerSystemComponent()
        : m_consoleCommandHandler([this]
        (
            AZStd::string_view command,
            const AZ::ConsoleCommandContainer& args,
            AZ::ConsoleFunctorFlags flags,
            AZ::ConsoleInvokedFrom invokedFrom
        ) { OnConsoleCommandInvoked(command, args, flags, invokedFrom); })
    {
        ;
    }

    void MultiplayerSystemComponent::Activate()
    {
        AZ::TickBus::Handler::BusConnect();
        AzFramework::SessionNotificationBus::Handler::BusConnect();
        m_networkInterface = AZ::Interface<INetworking>::Get()->CreateNetworkInterface(AZ::Name(MPNetworkInterfaceName), sv_protocol, TrustZone::ExternalClientToServer, *this);
        if (AZ::Interface<AZ::IConsole>::Get())
        {
            m_consoleCommandHandler.Connect(AZ::Interface<AZ::IConsole>::Get()->GetConsoleCommandInvokedEvent());
        }
        AZ::Interface<IMultiplayer>::Register(this);
        AZ::Interface<AzFramework::ISessionHandlingClientRequests>::Register(this);

        //! Register our gems multiplayer components to assign NetComponentIds
        RegisterMultiplayerComponents();
    }

    void MultiplayerSystemComponent::Deactivate()
    {
        AZ::Interface<AzFramework::ISessionHandlingClientRequests>::Unregister(this);
        AZ::Interface<IMultiplayer>::Unregister(this);
        m_consoleCommandHandler.Disconnect();
        AZ::Interface<INetworking>::Get()->DestroyNetworkInterface(AZ::Name(MPNetworkInterfaceName));
        AzFramework::SessionNotificationBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
    }

    bool MultiplayerSystemComponent::StartHosting(uint16_t port, bool isDedicated)
    {
        InitializeMultiplayer(isDedicated ? MultiplayerAgentType::DedicatedServer : MultiplayerAgentType::ClientServer);
        return m_networkInterface->Listen(port);
    }

    bool MultiplayerSystemComponent::Connect(AZStd::string remoteAddress, uint16_t port)
    {
        InitializeMultiplayer(MultiplayerAgentType::Client);
        const IpAddress address(remoteAddress.c_str(), port, m_networkInterface->GetType());
        return m_networkInterface->Connect(address) != InvalidConnectionId;
    }

    void MultiplayerSystemComponent::Terminate(AzNetworking::DisconnectReason reason)
    {
        // Cleanup connections, fire events and uninitialize state
        auto visitor = [reason](IConnection& connection) { connection.Disconnect(reason, TerminationEndpoint::Local); };
        m_networkInterface->GetConnectionSet().VisitConnections(visitor);
        MultiplayerAgentType agentType = GetAgentType();
        if (agentType == MultiplayerAgentType::DedicatedServer || agentType == MultiplayerAgentType::ClientServer)
        {
            m_networkInterface->StopListening();
            m_shutdownEvent.Signal(m_networkInterface);
        }
        InitializeMultiplayer(MultiplayerAgentType::Uninitialized);

        // Signal session management, do this after uninitializing state
        if (agentType == MultiplayerAgentType::DedicatedServer || agentType == MultiplayerAgentType::ClientServer)
        {
            if (AZ::Interface<AzFramework::ISessionHandlingProviderRequests>::Get() != nullptr)
            {
                AZ::Interface<AzFramework::ISessionHandlingProviderRequests>::Get()->HandleDestroySession();
            }
        }
    }

    bool MultiplayerSystemComponent::RequestPlayerJoinSession(const AzFramework::SessionConnectionConfig& config)
    {
        m_pendingConnectionTickets.push(config.m_playerSessionId);
        AZStd::string hostname = config.m_dnsName.empty() ? config.m_ipAddress : config.m_dnsName;
        Connect(hostname.c_str(), config.m_port);

        return true;
    }

    void MultiplayerSystemComponent::RequestPlayerLeaveSession()
    {
        if (GetAgentType() == MultiplayerAgentType::Client)
        {
            Terminate(DisconnectReason::TerminatedByUser);
        }
    }

    bool MultiplayerSystemComponent::OnSessionHealthCheck()
    {
        return true;
    }

    bool MultiplayerSystemComponent::OnCreateSessionBegin(const AzFramework::SessionConfig& sessionConfig)
    {
        // Check if session manager has a certificate for us and pass it along if so
        if (AZ::Interface<AzFramework::ISessionHandlingProviderRequests>::Get() != nullptr)
        {
            AZ::CVarFixedString externalCertPath = AZ::CVarFixedString(
                AZ::Interface<AzFramework::ISessionHandlingProviderRequests>::Get()->GetExternalSessionCertificate().c_str());
            if (!externalCertPath.empty())
            {
                AZ::CVarFixedString commandString = "net_SslExternalCertificateFile " + externalCertPath;
                AZ::Interface<AZ::IConsole>::Get()->PerformCommand(commandString.c_str());
            }

            AZ::CVarFixedString internalCertPath = AZ::CVarFixedString(
                AZ::Interface<AzFramework::ISessionHandlingProviderRequests>::Get()->GetInternalSessionCertificate().c_str());
            if (!internalCertPath.empty())
            {
                AZ::CVarFixedString commandString = "net_SslInternalCertificateFile " + internalCertPath;
                AZ::Interface<AZ::IConsole>::Get()->PerformCommand(commandString.c_str());
            }
        }

        Multiplayer::MultiplayerAgentType serverType = sv_isDedicated ? MultiplayerAgentType::DedicatedServer : MultiplayerAgentType::ClientServer;
        InitializeMultiplayer(serverType);
        return m_networkInterface->Listen(sessionConfig.m_port);
    }

    bool MultiplayerSystemComponent::OnDestroySessionBegin()
    {
        // This can be triggered external from Multiplayer so only run if we are in an Initialized state
        if (GetAgentType() == MultiplayerAgentType::Uninitialized)
        {
            return true;
        }

        auto visitor = [](IConnection& connection) { connection.Disconnect(DisconnectReason::TerminatedByServer, TerminationEndpoint::Local); };
        m_networkInterface->GetConnectionSet().VisitConnections(visitor);
        if (GetAgentType() == MultiplayerAgentType::DedicatedServer || GetAgentType() == MultiplayerAgentType::ClientServer)
        {
            m_networkInterface->StopListening();
            m_shutdownEvent.Signal(m_networkInterface);
        }
        InitializeMultiplayer(MultiplayerAgentType::Uninitialized);

        return true;
    }

    void MultiplayerSystemComponent::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        const AZ::TimeMs deltaTimeMs = aznumeric_cast<AZ::TimeMs>(static_cast<int32_t>(deltaTime * 1000.0f));
        const AZ::TimeMs hostTimeMs = AZ::GetElapsedTimeMs();
        const AZ::TimeMs serverRateMs = static_cast<AZ::TimeMs>(sv_serverSendRateMs);
        const float serverRateSeconds = static_cast<float>(serverRateMs) / 1000.0f;

        TickVisibleNetworkEntities(deltaTime, serverRateSeconds);

        if (GetAgentType() == MultiplayerAgentType::ClientServer
         || GetAgentType() == MultiplayerAgentType::DedicatedServer)
        {
            m_serverSendAccumulator += deltaTime;
            if (m_serverSendAccumulator < serverRateSeconds)
            {
                return;
            }
            m_serverSendAccumulator -= serverRateSeconds;
            m_networkTime.IncrementHostFrameId();
        }

        // Handle deferred local rpc messages that were generated during the updates
        m_networkEntityManager.DispatchLocalDeferredRpcMessages();

        // INetworking ticks immediately before IMultiplayer, so all our pending RPC's and network property updates have now been processed
        // Restore any entities that were rewound during input processing so that normal gameplay updates have the correct state
        Multiplayer::GetNetworkTime()->ClearRewoundEntities();

        // Let the network system know the frame is done and we can collect dirty bits
        m_networkEntityManager.NotifyEntitiesChanged();
        m_networkEntityManager.NotifyEntitiesDirtied();

        MultiplayerStats& stats = GetStats();
        stats.TickStats(deltaTimeMs);
        stats.m_entityCount = GetNetworkEntityManager()->GetEntityCount();
        stats.m_serverConnectionCount = 0;
        stats.m_clientConnectionCount = 0;

        // Send out the game state update to all connections
        {
            auto sendNetworkUpdates = [hostTimeMs, &stats](IConnection& connection)
            {
                if (connection.GetUserData() != nullptr)
                {
                    IConnectionData* connectionData = reinterpret_cast<IConnectionData*>(connection.GetUserData());
                    connectionData->Update(hostTimeMs);
                    if (connectionData->GetConnectionDataType() == ConnectionDataType::ServerToClient)
                    {
                        stats.m_clientConnectionCount++;
                    }
                    else
                    {
                        stats.m_serverConnectionCount++;
                    }
                }
            };

            m_networkInterface->GetConnectionSet().VisitConnections(sendNetworkUpdates);
        }

        MultiplayerPackets::SyncConsole packet;
        AZ::ThreadSafeDeque<AZStd::string>::DequeType cvarUpdates;
        m_cvarCommands.Swap(cvarUpdates);

        auto visitor = [&packet](IConnection& connection)
        {
            if (connection.GetConnectionRole() == ConnectionRole::Acceptor)
            {
                connection.SendReliablePacket(packet);
            }
        };

        while (cvarUpdates.size() > 0)
        {
            packet.ModifyCommandSet().emplace_back(cvarUpdates.front());
            if (packet.GetCommandSet().full())
            {
                m_networkInterface->GetConnectionSet().VisitConnections(visitor);
                packet.ModifyCommandSet().clear();
            }
            cvarUpdates.pop_front();
        }

        if (!packet.GetCommandSet().empty())
        {
            m_networkInterface->GetConnectionSet().VisitConnections(visitor);
        }
    }

    int MultiplayerSystemComponent::GetTickOrder()
    {
        // Tick immediately after the network system component
        return AZ::TICK_PLACEMENT + 1;
    }

    struct ConsoleReplicator
    {
        ConsoleReplicator(IConnection* connection)
            : m_connection(connection)
        {
            ;
        }

        virtual ~ConsoleReplicator()
        {
            if (!m_syncPacket.GetCommandSet().empty())
            {
                m_connection->SendReliablePacket(m_syncPacket);
            }
        }

        void Visit(AZ::ConsoleFunctorBase* functor)
        {
            if ((functor->GetFlags() & AZ::ConsoleFunctorFlags::DontReplicate) == AZ::ConsoleFunctorFlags::DontReplicate)
            {
                // If the cvar is marked don't replicate, don't send it at all
                return;
            }
            AZ::CVarFixedString replicateValue;
            if (functor->GetReplicationString(replicateValue))
            {
                m_syncPacket.ModifyCommandSet().emplace_back(AZStd::move(replicateValue));
                if (m_syncPacket.GetCommandSet().full())
                {
                    m_connection->SendReliablePacket(m_syncPacket);
                    m_syncPacket.ModifyCommandSet().clear();
                }
            }
        }

        IConnection* m_connection;
        MultiplayerPackets::SyncConsole m_syncPacket;
    };

    bool MultiplayerSystemComponent::HandleRequest
    (
        [[maybe_unused]] AzNetworking::IConnection* connection,
        [[maybe_unused]] const IPacketHeader& packetHeader,
        [[maybe_unused]] MultiplayerPackets::Connect& packet
    )
    {
        // Validate our session with the provider if any
        if (AZ::Interface<AzFramework::ISessionHandlingProviderRequests>::Get() != nullptr)
        {
            AzFramework::PlayerConnectionConfig config;
            config.m_playerConnectionId = aznumeric_cast<uint32_t>(connection->GetConnectionId());
            config.m_playerSessionId = packet.GetTicket();
            if(!AZ::Interface<AzFramework::ISessionHandlingProviderRequests>::Get()->ValidatePlayerJoinSession(config))
            {
                auto visitor = [](IConnection& connection) { connection.Disconnect(DisconnectReason::TerminatedByUser, TerminationEndpoint::Local); };
                m_networkInterface->GetConnectionSet().VisitConnections(visitor);
                return true;
            }   
        }
        reinterpret_cast<ServerToClientConnectionData*>(connection->GetUserData())->SetProviderTicket(packet.GetTicket().c_str());

        if (connection->SendReliablePacket(MultiplayerPackets::Accept(InvalidHostId, sv_map)))
        {
            // Sync our console
            ConsoleReplicator consoleReplicator(connection);
            AZ::Interface<AZ::IConsole>::Get()->VisitRegisteredFunctors([&consoleReplicator](AZ::ConsoleFunctorBase* functor) { consoleReplicator.Visit(functor); });
            return true;
        }
        return false;
    }

    bool MultiplayerSystemComponent::HandleRequest
    (
        [[maybe_unused]] AzNetworking::IConnection* connection,
        [[maybe_unused]] const IPacketHeader& packetHeader,
        [[maybe_unused]] MultiplayerPackets::Accept& packet
    )
    {
        AZ::CVarFixedString commandString = "sv_map " + packet.GetMap();
        AZ::Interface<AZ::IConsole>::Get()->PerformCommand(commandString.c_str());

        AZ::CVarFixedString loadLevelString = "LoadLevel " + packet.GetMap();
        AZ::Interface<AZ::IConsole>::Get()->PerformCommand(loadLevelString.c_str());
        return true;
    }

    bool MultiplayerSystemComponent::HandleRequest
    (
        AzNetworking::IConnection* connection,
        [[maybe_unused]] const AzNetworking::IPacketHeader& packetHeader,
        MultiplayerPackets::ReadyForEntityUpdates& packet
    )
    {
        IConnectionData* connectionData = reinterpret_cast<IConnectionData*>(connection->GetUserData());
        if (connectionData)
        {
            connectionData->SetCanSendUpdates(packet.GetReadyForEntityUpdates());
            return true;
        }
        return false;
    }

    bool MultiplayerSystemComponent::HandleRequest
    (
        [[maybe_unused]] AzNetworking::IConnection* connection,
        [[maybe_unused]] const IPacketHeader& packetHeader,
        [[maybe_unused]] MultiplayerPackets::SyncConsole& packet
    )
    {
        if (GetAgentType() != MultiplayerAgentType::Client)
        {
            return false;
        }
        ExecuteConsoleCommandList(connection, packet.GetCommandSet());
        return true;
    }

    bool MultiplayerSystemComponent::HandleRequest
    (
        [[maybe_unused]] AzNetworking::IConnection* connection,
        [[maybe_unused]] const IPacketHeader& packetHeader,
        [[maybe_unused]] MultiplayerPackets::ConsoleCommand& packet
    )
    {
        const bool isClient = (GetAgentType() == MultiplayerAgentType::Client);
        const AZ::ConsoleFunctorFlags requiredSet = isClient ? AZ::ConsoleFunctorFlags::Null : AZ::ConsoleFunctorFlags::AllowClientSet;
        AZ::Interface<AZ::IConsole>::Get()->PerformCommand(packet.GetCommand().c_str(), AZ::ConsoleSilentMode::NotSilent, AZ::ConsoleInvokedFrom::AzNetworking, requiredSet);
        return true;
    }

    bool MultiplayerSystemComponent::HandleRequest
    (
        [[maybe_unused]] AzNetworking::IConnection* connection,
        [[maybe_unused]] const IPacketHeader& packetHeader,
        [[maybe_unused]] MultiplayerPackets::EntityUpdates& packet
    )
    {
        bool handledAll = true;
        if (connection->GetUserData() == nullptr)
        {
            AZLOG_WARN("Missing connection data, likely due to a connection in the process of closing, entity updates size %u", aznumeric_cast<uint32_t>(packet.GetEntityMessages().size()));
            return handledAll;
        }

        EntityReplicationManager& replicationManager = reinterpret_cast<IConnectionData*>(connection->GetUserData())->GetReplicationManager();

        if ((GetAgentType() == MultiplayerAgentType::Client) && (packet.GetHostFrameId() > m_lastReplicatedHostFrameId))
        {
            // Update client to latest server time
            m_tickFactor = 0.0f;
            m_lastReplicatedHostTimeMs = packet.GetHostTimeMs();
            m_lastReplicatedHostFrameId = packet.GetHostFrameId();
            m_networkTime.ForceSetTime(m_lastReplicatedHostFrameId, m_lastReplicatedHostTimeMs);
        }

        for (AZStd::size_t i = 0; i < packet.GetEntityMessages().size(); ++i)
        {
            const NetworkEntityUpdateMessage& updateMessage = packet.GetEntityMessages()[i];
            handledAll &= replicationManager.HandleEntityUpdateMessage(connection, packetHeader, updateMessage);
            AZ_Assert(handledAll, "EntityUpdates did not handle all update messages");
        }

        return handledAll;
    }

    bool MultiplayerSystemComponent::HandleRequest
    (
        [[maybe_unused]] AzNetworking::IConnection* connection,
        [[maybe_unused]] const IPacketHeader& packetHeader,
        [[maybe_unused]] MultiplayerPackets::EntityRpcs& packet
    )
    {
        bool handledAll = true;
        if (connection->GetUserData() == nullptr)
        {
            AZLOG_WARN("Missing connection data, likely due to a connection in the process of closing, entity updates size %u", aznumeric_cast<uint32_t>(packet.GetEntityRpcs().size()));
            return handledAll;
        }

        EntityReplicationManager& replicationManager = reinterpret_cast<IConnectionData*>(connection->GetUserData())->GetReplicationManager();
        for (AZStd::size_t i = 0; i < packet.GetEntityRpcs().size(); ++i)
        {
            handledAll &= replicationManager.HandleEntityRpcMessage(connection, packet.ModifyEntityRpcs()[i]);
        }

        return handledAll;
    }

    bool MultiplayerSystemComponent::HandleRequest
    (
        [[maybe_unused]] AzNetworking::IConnection* connection,
        [[maybe_unused]] const IPacketHeader& packetHeader,
        [[maybe_unused]] MultiplayerPackets::ClientMigration& packet
    )
    {
        return false;
    }

    ConnectResult MultiplayerSystemComponent::ValidateConnect
    (
        [[maybe_unused]] const IpAddress& remoteAddress,
        [[maybe_unused]] const IPacketHeader& packetHeader,
        [[maybe_unused]] ISerializer& serializer
    )
    {
        return ConnectResult::Accepted;
    }

    void MultiplayerSystemComponent::OnConnect(AzNetworking::IConnection* connection)
    {
        MultiplayerAgentDatum datum;
        datum.m_id = connection->GetConnectionId();
        datum.m_isInvited = false;
        datum.m_agentType = MultiplayerAgentType::Client;

        AZStd::string providerTicket;
        if (connection->GetConnectionRole() == ConnectionRole::Connector)
        {
            AZLOG_INFO("New outgoing connection to remote address: %s", connection->GetRemoteAddress().GetString().c_str());
            if (!m_pendingConnectionTickets.empty())
            {
                providerTicket = m_pendingConnectionTickets.front();
                m_pendingConnectionTickets.pop();
            }
            connection->SendReliablePacket(MultiplayerPackets::Connect(0, providerTicket.c_str()));
        }
        else
        {
            AZLOG_INFO("New incoming connection from remote address: %s", connection->GetRemoteAddress().GetString().c_str());
            m_connAcquiredEvent.Signal(datum);
        }

        // Hosts will spawn a new default player prefab for the user that just connected
        if (GetAgentType() == MultiplayerAgentType::ClientServer
         || GetAgentType() == MultiplayerAgentType::DedicatedServer)
        {
            NetworkEntityHandle controlledEntity = SpawnDefaultPlayerPrefab();
            if (controlledEntity.Exists())
            {
                controlledEntity.GetNetBindComponent()->SetOwningConnectionId(connection->GetConnectionId());
            }
            
            if (connection->GetUserData() == nullptr) // Only add user data if the connect event handler has not already done so
            {
                connection->SetUserData(new ServerToClientConnectionData(connection, *this, controlledEntity));
            }

            AZStd::unique_ptr<IReplicationWindow> window = AZStd::make_unique<ServerToClientReplicationWindow>(controlledEntity, connection);
            reinterpret_cast<ServerToClientConnectionData*>(connection->GetUserData())->GetReplicationManager().SetReplicationWindow(AZStd::move(window));
        }
        else
        {
            if (connection->GetUserData() == nullptr) // Only add user data if the connect event handler has not already done so
            {
                connection->SetUserData(new ClientToServerConnectionData(connection, *this, providerTicket));
            }
            else
            {
                reinterpret_cast<ClientToServerConnectionData*>(connection->GetUserData())->SetProviderTicket(providerTicket);
            }

            AZStd::unique_ptr<IReplicationWindow> window = AZStd::make_unique<NullReplicationWindow>();
            reinterpret_cast<ClientToServerConnectionData*>(connection->GetUserData())->GetReplicationManager().SetEntityActivationTimeSliceMs(cl_defaultNetworkEntityActivationTimeSliceMs);
            reinterpret_cast<ClientToServerConnectionData*>(connection->GetUserData())->GetReplicationManager().SetReplicationWindow(AZStd::move(window));
        }
    }

    bool MultiplayerSystemComponent::OnPacketReceived(AzNetworking::IConnection* connection, const IPacketHeader& packetHeader, ISerializer& serializer)
    {
        return MultiplayerPackets::DispatchPacket(connection, packetHeader, serializer, *this);
    }

    void MultiplayerSystemComponent::OnPacketLost([[maybe_unused]] IConnection* connection, [[maybe_unused]] PacketId packetId)
    {
        ;
    }

    void MultiplayerSystemComponent::OnDisconnect(AzNetworking::IConnection* connection, DisconnectReason reason, TerminationEndpoint endpoint)
    {
        const char* endpointString = (endpoint == TerminationEndpoint::Local) ? "Disconnecting" : "Remote host disconnected";
        AZStd::string reasonString = ToString(reason);
        AZLOG_INFO("%s due to %s from remote address: %s", endpointString, reasonString.c_str(), connection->GetRemoteAddress().GetString().c_str());

        // The client is disconnecting
        if (GetAgentType() == MultiplayerAgentType::Client)
        {
            AZ_Assert(connection->GetConnectionRole() == ConnectionRole::Connector, "Client connection role should only ever be Connector");
            m_clientDisconnectedEvent.Signal();
        }

        // Signal to session management that a user has left the server
        if (m_agentType == MultiplayerAgentType::DedicatedServer || m_agentType == MultiplayerAgentType::ClientServer)
        {
            if (AZ::Interface<AzFramework::ISessionHandlingProviderRequests>::Get() != nullptr &&
                connection->GetConnectionRole() == ConnectionRole::Acceptor)
            {
                AzFramework::PlayerConnectionConfig config;
                config.m_playerConnectionId = aznumeric_cast<uint32_t>(connection->GetConnectionId());
                config.m_playerSessionId = reinterpret_cast<ServerToClientConnectionData*>(connection->GetUserData())->GetProviderTicket();
                AZ::Interface<AzFramework::ISessionHandlingProviderRequests>::Get()->HandlePlayerLeaveSession(config);
            }
        }

        // Clean up any multiplayer connection data we've bound to this connection instance
        if (connection->GetUserData() != nullptr)
        {
            IConnectionData* connectionData = reinterpret_cast<IConnectionData*>(connection->GetUserData());
            delete connectionData;
            connection->SetUserData(nullptr);
        }

        // Signal to session management when there are no remaining players in a dedicated server for potential cleanup
        // We avoid this for client server as the host itself is a user and non-transient dedicated servers
        if (sv_isTransient && m_agentType == MultiplayerAgentType::DedicatedServer && connection->GetConnectionRole() == ConnectionRole::Acceptor)
        {   
            if (m_networkInterface->GetConnectionSet().GetActiveConnectionCount() == 0)
            {
                Terminate(DisconnectReason::TerminatedByServer);
            }
        }
    }

    MultiplayerAgentType MultiplayerSystemComponent::GetAgentType() const
    {
        return m_agentType;
    }

    void MultiplayerSystemComponent::InitializeMultiplayer(MultiplayerAgentType multiplayerType)
    {
        if (m_agentType == multiplayerType)
        {
            return;
        }

        if (m_agentType != MultiplayerAgentType::Uninitialized && multiplayerType != MultiplayerAgentType::Uninitialized)
        {
            AZLOG_WARN("Attemping to InitializeMultiplayer from one initialized type to another. Your session may not have been properly torn down.");
        }

        if (m_agentType == MultiplayerAgentType::Uninitialized)
        {
            if (multiplayerType == MultiplayerAgentType::ClientServer || multiplayerType == MultiplayerAgentType::DedicatedServer)
            {
                m_initEvent.Signal(m_networkInterface);

                const AZ::Aabb worldBounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-16384.0f), AZ::Vector3(16384.0f));
                //const AZ::Aabb worldBounds = AZ::Interface<IPhysics>.Get()->GetWorldBounds();
                AZStd::unique_ptr<IEntityDomain> newDomain = AZStd::make_unique<FullOwnershipEntityDomain>();
                m_networkEntityManager.Initialize(InvalidHostId, AZStd::move(newDomain));
            }
        }
        m_agentType = multiplayerType;

        // Spawn the default player for this host since the host is also a player (not a dedicated server)
        if (m_agentType == MultiplayerAgentType::ClientServer)
        {
            NetworkEntityHandle controlledEntity = SpawnDefaultPlayerPrefab();
            if (NetBindComponent* controlledEntityNetBindComponent = controlledEntity.GetNetBindComponent())
            {
                controlledEntityNetBindComponent->SetAllowAutonomy(true);
            }
        }
        
        AZLOG_INFO("Multiplayer operating in %s mode", GetEnumString(m_agentType));
    }

    void MultiplayerSystemComponent::AddClientDisconnectedHandler(ClientDisconnectedEvent::Handler& handler)
    {
        handler.Connect(m_clientDisconnectedEvent);
    }

    void MultiplayerSystemComponent::AddConnectionAcquiredHandler(ConnectionAcquiredEvent::Handler& handler)
    {
        handler.Connect(m_connAcquiredEvent);
    }

    void MultiplayerSystemComponent::AddSessionInitHandler(SessionInitEvent::Handler& handler)
    {
        handler.Connect(m_initEvent);
    }

    void MultiplayerSystemComponent::AddSessionShutdownHandler(SessionShutdownEvent::Handler& handler)
    {
        handler.Connect(m_shutdownEvent);
    }

    void MultiplayerSystemComponent::SendReadyForEntityUpdates(bool readyForEntityUpdates)
    {
        IConnectionSet& connectionSet = m_networkInterface->GetConnectionSet();
        connectionSet.VisitConnections([readyForEntityUpdates](IConnection& connection)
        {
            connection.SendReliablePacket(MultiplayerPackets::ReadyForEntityUpdates(readyForEntityUpdates));
        });
    }

    AZ::TimeMs MultiplayerSystemComponent::GetCurrentHostTimeMs() const
    {
        if (GetAgentType() == MultiplayerAgentType::Client)
        {
            return m_lastReplicatedHostTimeMs;
        }
        else // ClientServer or DedicatedServer
        {
            return m_networkTime.GetHostTimeMs();
        }
    }

    float MultiplayerSystemComponent::GetCurrentBlendFactor() const
    {
        return m_renderBlendFactor;
    }

    INetworkTime* MultiplayerSystemComponent::GetNetworkTime()
    {
        return &m_networkTime;
    }

    INetworkEntityManager* MultiplayerSystemComponent::GetNetworkEntityManager()
    {
        return &m_networkEntityManager;
    }

    void MultiplayerSystemComponent::SetFilterEntityManager(IFilterEntityManager* entityFilter)
    {
        m_filterEntityManager = entityFilter;
    }

    IFilterEntityManager* MultiplayerSystemComponent::GetFilterEntityManager()
    {
        return m_filterEntityManager;
    }

    void MultiplayerSystemComponent::DumpStats([[maybe_unused]] const AZ::ConsoleCommandContainer& arguments)
    {
        const MultiplayerStats& stats = GetStats();

        AZLOG_INFO("Total networked entities: %llu", aznumeric_cast<AZ::u64>(stats.m_entityCount));
        AZLOG_INFO("Total client connections: %llu", aznumeric_cast<AZ::u64>(stats.m_clientConnectionCount));
        AZLOG_INFO("Total server connections: %llu", aznumeric_cast<AZ::u64>(stats.m_serverConnectionCount));

        const MultiplayerStats::Metric propertyUpdatesSent = stats.CalculateTotalPropertyUpdateSentMetrics();
        const MultiplayerStats::Metric propertyUpdatesRecv = stats.CalculateTotalPropertyUpdateRecvMetrics();
        const MultiplayerStats::Metric rpcsSent = stats.CalculateTotalRpcsSentMetrics();
        const MultiplayerStats::Metric rpcsRecv = stats.CalculateTotalRpcsRecvMetrics();

        AZLOG_INFO("Total property updates sent: %llu", aznumeric_cast<AZ::u64>(propertyUpdatesSent.m_totalCalls));
        AZLOG_INFO("Total property updates sent bytes: %llu", aznumeric_cast<AZ::u64>(propertyUpdatesSent.m_totalBytes));
        AZLOG_INFO("Total property updates received: %llu", aznumeric_cast<AZ::u64>(propertyUpdatesRecv.m_totalCalls));
        AZLOG_INFO("Total property updates received bytes: %llu", aznumeric_cast<AZ::u64>(propertyUpdatesRecv.m_totalBytes));
        AZLOG_INFO("Total RPCs sent: %llu", aznumeric_cast<AZ::u64>(rpcsSent.m_totalCalls));
        AZLOG_INFO("Total RPCs sent bytes: %llu", aznumeric_cast<AZ::u64>(rpcsSent.m_totalBytes));
        AZLOG_INFO("Total RPCs received: %llu", aznumeric_cast<AZ::u64>(rpcsRecv.m_totalCalls));
        AZLOG_INFO("Total RPCs received bytes: %llu", aznumeric_cast<AZ::u64>(rpcsRecv.m_totalBytes));
    }

    void MultiplayerSystemComponent::TickVisibleNetworkEntities(float deltaTime, float serverRateSeconds)
    {
        m_tickFactor += deltaTime / serverRateSeconds;
        // Linear close to the origin, but asymptote at y = 1
        m_renderBlendFactor = AZStd::clamp(1.0f - (std::pow(cl_renderTickBlendBase, m_tickFactor)), 0.0f, m_tickFactor);
        AZLOG
        (
            NET_Blending,
            "Computed blend factor of %0.3f using a tick factor of %0.3f, a frametime of %0.3f and a serverTickRate of %0.3f",
            m_renderBlendFactor,
            m_tickFactor,
            deltaTime,
            serverRateSeconds
        );

        if (Camera::ActiveCameraRequestBus::HasHandlers())
        {
            // If there's a camera, update only what's visible
            AZ::Transform activeCameraTransform;
            Camera::Configuration activeCameraConfiguration;
            Camera::ActiveCameraRequestBus::BroadcastResult(activeCameraTransform, &Camera::ActiveCameraRequestBus::Events::GetActiveCameraTransform);
            Camera::ActiveCameraRequestBus::BroadcastResult(activeCameraConfiguration, &Camera::ActiveCameraRequestBus::Events::GetActiveCameraConfiguration);

            const AZ::ViewFrustumAttributes frustumAttributes
            (
                activeCameraTransform,
                activeCameraConfiguration.m_frustumHeight / activeCameraConfiguration.m_frustumWidth,
                activeCameraConfiguration.m_fovRadians,
                activeCameraConfiguration.m_nearClipDistance,
                activeCameraConfiguration.m_farClipDistance
            );
            const AZ::Frustum viewFrustum = AZ::Frustum(frustumAttributes);

            // Unfortunately necessary, as NotifyPreRender can update transforms and thus cause a deadlock inside the vis system
            AZStd::vector<NetBindComponent*> gatheredEntities;
            AzFramework::IEntityBoundsUnion* entityBoundsUnion = AZ::Interface<AzFramework::IEntityBoundsUnion>::Get();
            AZ::Interface<AzFramework::IVisibilitySystem>::Get()->GetDefaultVisibilityScene()->Enumerate(viewFrustum,
                [&gatheredEntities, entityBoundsUnion](const AzFramework::IVisibilityScene::NodeData& nodeData)
            {
                gatheredEntities.reserve(gatheredEntities.size() + nodeData.m_entries.size());
                for (AzFramework::VisibilityEntry* visEntry : nodeData.m_entries)
                {
                    if (visEntry->m_typeFlags & AzFramework::VisibilityEntry::TypeFlags::TYPE_Entity)
                    {
                        AZ::Entity* entity = static_cast<AZ::Entity*>(visEntry->m_userData);
                        NetBindComponent* netBindComponent = entity->FindComponent<NetBindComponent>();
                        if (netBindComponent != nullptr)
                        {
                            gatheredEntities.push_back(netBindComponent);
                        }
                    }
                }
            });

            for (NetBindComponent* netBindComponent : gatheredEntities)
            {
                netBindComponent->NotifyPreRender(deltaTime, m_renderBlendFactor);
            }
        }
        else
        {
            // If there's no camera, fall back to updating all net entities
            for (auto& iter : *(m_networkEntityManager.GetNetworkEntityTracker()))
            {
                AZ::Entity* entity = iter.second;
                NetBindComponent* netBindComponent = entity->FindComponent<NetBindComponent>();
                if (netBindComponent != nullptr)
                {
                    netBindComponent->NotifyPreRender(deltaTime, m_renderBlendFactor);
                }
            }
        }
    }

    void MultiplayerSystemComponent::OnConsoleCommandInvoked
    (
        AZStd::string_view command,
        const AZ::ConsoleCommandContainer& args,
        AZ::ConsoleFunctorFlags flags,
        AZ::ConsoleInvokedFrom invokedFrom
    )
    {
        if (invokedFrom == AZ::ConsoleInvokedFrom::AzNetworking)
        {
            return;
        }

        if ((flags & AZ::ConsoleFunctorFlags::DontReplicate) == AZ::ConsoleFunctorFlags::DontReplicate)
        {
            // If the cvar is marked don't replicate, don't send it at all
            return;
        }

        AZStd::string replicateString = AZStd::string(command) + " ";
        AZ::StringFunc::Join(replicateString, args.begin(), args.end(), " ");
        m_cvarCommands.PushBackItem(AZStd::move(replicateString));
    }

    void MultiplayerSystemComponent::ExecuteConsoleCommandList(IConnection* connection, const AZStd::fixed_vector<Multiplayer::LongNetworkString, 32>& commands)
    {
        AZ::IConsole* console = AZ::Interface<AZ::IConsole>::Get();
        const bool isAcceptor = (connection->GetConnectionRole() == ConnectionRole::Acceptor); // We're hosting if we accepted the connection
        const AZ::ConsoleFunctorFlags requiredSet = isAcceptor ? AZ::ConsoleFunctorFlags::AllowClientSet : AZ::ConsoleFunctorFlags::Null;
        for (auto& command : commands)
        {
            console->PerformCommand(command.c_str(), AZ::ConsoleSilentMode::NotSilent, AZ::ConsoleInvokedFrom::AzNetworking, requiredSet);
        }
    }

    NetworkEntityHandle MultiplayerSystemComponent::SpawnDefaultPlayerPrefab()
    {
        PrefabEntityId playerPrefabEntityId(AZ::Name(static_cast<AZ::CVarFixedString>(sv_defaultPlayerSpawnAsset).c_str()));
        INetworkEntityManager::EntityList entityList = m_networkEntityManager.CreateEntitiesImmediate(playerPrefabEntityId, NetEntityRole::Authority, AZ::Transform::CreateIdentity());

        NetworkEntityHandle controlledEntity;
        if (entityList.size() > 0)
        {
            controlledEntity = entityList[0];
        }
        return controlledEntity;
    }

    void host([[maybe_unused]] const AZ::ConsoleCommandContainer& arguments)
    {
        AZ::Interface<IMultiplayer>::Get()->StartHosting(sv_port, sv_isDedicated);
    }
    AZ_CONSOLEFREEFUNC(host, AZ::ConsoleFunctorFlags::DontReplicate, "Opens a multiplayer connection as a host for other clients to connect to");

    void connect([[maybe_unused]] const AZ::ConsoleCommandContainer& arguments)
    {
        if (arguments.size() < 1)
        {
            const AZ::CVarFixedString remoteAddress = cl_serveraddr;
            AZ::Interface<IMultiplayer>::Get()->Connect(remoteAddress.c_str(), cl_serverport);
        }
        else
        {
            AZ::CVarFixedString remoteAddress{ arguments.front() };
            const AZStd::size_t portSeparator = remoteAddress.find_first_of(':');
            if (portSeparator == AZStd::string::npos)
            {
                AZLOG_INFO("Remote address %s was malformed", remoteAddress.c_str());
                return;
            }
            char* mutableAddress = remoteAddress.data();
            mutableAddress[portSeparator] = '\0';
            const char* addressStr = mutableAddress;
            const char* portStr = &(mutableAddress[portSeparator + 1]);
            int32_t portNumber = atol(portStr);
            AZ::Interface<IMultiplayer>::Get()->Connect(addressStr, portNumber);
        }
    }
    AZ_CONSOLEFREEFUNC(connect, AZ::ConsoleFunctorFlags::DontReplicate, "Opens a multiplayer connection to a remote host");

    void disconnect([[maybe_unused]] const AZ::ConsoleCommandContainer& arguments)
    {
        AZ::Interface<IMultiplayer>::Get()->Terminate(DisconnectReason::TerminatedByUser);
    }
    AZ_CONSOLEFREEFUNC(disconnect, AZ::ConsoleFunctorFlags::DontReplicate, "Disconnects any open multiplayer connections");
}
