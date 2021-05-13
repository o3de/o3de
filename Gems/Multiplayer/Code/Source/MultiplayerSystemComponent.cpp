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

#include <Source/MultiplayerSystemComponent.h>
#include <Source/Components/MultiplayerComponent.h>
#include <Source/AutoGen/AutoComponentTypes.h>
#include <Source/ConnectionData/ClientToServerConnectionData.h>
#include <Source/ConnectionData/ServerToClientConnectionData.h>
#include <Source/ReplicationWindows/NullReplicationWindow.h>
#include <Source/ReplicationWindows/ServerToClientReplicationWindow.h>
#include <Source/EntityDomains/FullOwnershipEntityDomain.h>
#include <AzNetworking/Framework/INetworking.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ILogger.h>

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

    static const AZStd::string_view s_networkInterfaceName("MultiplayerNetworkInterface");
    static constexpr uint16_t DefaultServerPort = 30090;

    AZ_CVAR(uint16_t, cl_clientport, 0, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "The port to bind to for game traffic when connecting to a remote host, a value of 0 will select any available port");
    AZ_CVAR(AZ::CVarFixedString, cl_serveraddr, "127.0.0.1", nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "The address of the remote server or host to connect to");
    AZ_CVAR(AZ::CVarFixedString, cl_serverpassword, "", nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Optional server password");
    AZ_CVAR(uint16_t, cl_serverport, DefaultServerPort, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "The port of the remote host to connect to for game traffic");
    AZ_CVAR(uint16_t, sv_port, DefaultServerPort, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "The port that this multiplayer gem will bind to for game traffic");
    AZ_CVAR(AZ::CVarFixedString, sv_map, "nolevel", nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "The map the server should load");
    AZ_CVAR(AZ::CVarFixedString, sv_gamerules, "norules", nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "GameRules server works with");
    AZ_CVAR(ProtocolType, sv_protocol, ProtocolType::Udp, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "This flag controls whether we use TCP or UDP for game networking");
    AZ_CVAR(bool, sv_isDedicated, true, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Whether the host command creates an independent or client hosted server");
    AZ_CVAR(AZ::TimeMs, cl_defaultNetworkEntityActivationTimeSliceMs, AZ::TimeMs{ 0 }, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Max Ms to use to activate entities coming from the network, 0 means instantiate everything");

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
        m_networkInterface = AZ::Interface<INetworking>::Get()->CreateNetworkInterface(AZ::Name(s_networkInterfaceName), sv_protocol, TrustZone::ExternalClientToServer, *this);
        m_consoleCommandHandler.Connect(AZ::Interface<AZ::IConsole>::Get()->GetConsoleCommandInvokedEvent());
        AZ::Interface<IMultiplayer>::Register(this);

        //! Register our gems multiplayer components to assign NetComponentIds
        RegisterMultiplayerComponents();
    }

    void MultiplayerSystemComponent::Deactivate()
    {
        AZ::Interface<IMultiplayer>::Unregister(this);
        AZ::TickBus::Handler::BusDisconnect();
    }

    void MultiplayerSystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        AZ::TimeMs deltaTimeMs = aznumeric_cast<AZ::TimeMs>(static_cast<int32_t>(deltaTime * 1000.0f));
        AZ::TimeMs hostTimeMs = AZ::GetElapsedTimeMs();

        // Handle deferred local rpc messages that were generated during the updates
        m_networkEntityManager.DispatchLocalDeferredRpcMessages();
        m_networkEntityManager.NotifyEntitiesChanged();

        // Let the network system know the frame is done and we can collect dirty bits
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
        [[maybe_unused]] AzNetworking::IConnection* connection,
        [[maybe_unused]] const IPacketHeader& packetHeader,
        [[maybe_unused]] MultiplayerPackets::SyncConsole& packet
    )
    {
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
        const bool isAcceptor = (connection->GetConnectionRole() == ConnectionRole::Acceptor); // We're hosting if we accepted the connection
        const AZ::ConsoleFunctorFlags requiredSet = isAcceptor ? AZ::ConsoleFunctorFlags::AllowClientSet : AZ::ConsoleFunctorFlags::Null;
        AZ::Interface<AZ::IConsole>::Get()->PerformCommand(packet.GetCommand().c_str(), AZ::ConsoleSilentMode::NotSilent, AZ::ConsoleInvokedFrom::AzNetworking, requiredSet);
        return true;
    }

    bool MultiplayerSystemComponent::HandleRequest
    (
        [[maybe_unused]] AzNetworking::IConnection* connection,
        [[maybe_unused]] const IPacketHeader& packetHeader,
        [[maybe_unused]] MultiplayerPackets::SyncConnectionCvars& packet
    )
    {
        connection->SetConnectionQuality(ConnectionQuality(packet.GetLossPercent(), packet.GetLatencyMs(), packet.GetVarianceMs()));
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
        
        // Ignore a_Request.GetServerGameTimePoint(), clients can't affect the server gametime
        for (AZStd::size_t i = 0; i < packet.GetEntityMessages().size(); ++i)
        {
            const NetworkEntityUpdateMessage& updateMessage = packet.GetEntityMessages()[i];
            handledAll &= replicationManager.HandleEntityUpdateMessage(connection, packetHeader, updateMessage);
            AZ_Assert(handledAll, "GameServerToClientNetworkRequestHandler EntityUpdates Did not handle all updates");
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

    bool MultiplayerSystemComponent::HandleRequest
    (
        [[maybe_unused]] AzNetworking::IConnection* connection,
        [[maybe_unused]] const AzNetworking::IPacketHeader& packetHeader,
        [[maybe_unused]] MultiplayerPackets::NotifyClientMigration& packet
    )
    {
        return false;
    }

    bool MultiplayerSystemComponent::HandleRequest
    (
        [[maybe_unused]] AzNetworking::IConnection* connection,
        [[maybe_unused]] const AzNetworking::IPacketHeader& packetHeader,
        [[maybe_unused]] MultiplayerPackets::EntityMigration& packet
    )
    {
        return false;
    }

    bool MultiplayerSystemComponent::HandleRequest( AzNetworking::IConnection* connection,
        [[maybe_unused]] const AzNetworking::IPacketHeader& packetHeader, MultiplayerPackets::ReadyForEntityUpdates& packet)
    {
        IConnectionData* connectionData = reinterpret_cast<IConnectionData*>(connection->GetUserData());
        if (connectionData)
        {
            connectionData->SetCanSendUpdates(packet.GetReadyForEntityUpdates());
            return true;
        }

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
        if (connection->GetConnectionRole() == ConnectionRole::Connector)
        {
            AZLOG_INFO("New outgoing connection to remote address: %s", connection->GetRemoteAddress().GetString().c_str());
            connection->SendReliablePacket(MultiplayerPackets::Connect(0));
        }
        else
        {
            AZLOG_INFO("New incoming connection from remote address: %s", connection->GetRemoteAddress().GetString().c_str());
            MultiplayerAgentDatum datum;
            datum.m_id = connection->GetConnectionId();
            datum.m_isInvited = false;
            datum.m_agentType = MultiplayerAgentType::Client;
            m_connAcquiredEvent.Signal(datum);
        }

        if (GetAgentType() == MultiplayerAgentType::ClientServer
         || GetAgentType() == MultiplayerAgentType::DedicatedServer)
        {
            // TODO: This needs to be set to the players autonomous proxy ------------v
            NetworkEntityHandle controlledEntity = GetNetworkEntityTracker()->Get(NetEntityId{ 0 });

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
                connection->SetUserData(new ClientToServerConnectionData(connection, *this));
            }

            AZStd::unique_ptr<IReplicationWindow> window = AZStd::make_unique<NullReplicationWindow>();
            reinterpret_cast<ServerToClientConnectionData*>(connection->GetUserData())->GetReplicationManager().SetEntityActivationTimeSliceMs(cl_defaultNetworkEntityActivationTimeSliceMs);
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

        // The authority is shutting down its connection
        if (connection->GetConnectionRole() == ConnectionRole::Acceptor)
        {
            m_shutdownEvent.Signal(m_networkInterface);
        }

        // Clean up any multiplayer connection data we've bound to this connection instance
        if (connection->GetUserData() != nullptr)
        {
            IConnectionData* connectionData = reinterpret_cast<IConnectionData*>(connection->GetUserData());
            delete connectionData;
            connection->SetUserData(nullptr);
        }
    }

    MultiplayerAgentType MultiplayerSystemComponent::GetAgentType() const
    {
        return m_agentType;
    }

    void MultiplayerSystemComponent::InitializeMultiplayer(MultiplayerAgentType multiplayerType)
    {
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
        AZLOG_INFO("Multiplayer operating in %s mode", GetEnumString(m_agentType));
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

    const char* MultiplayerSystemComponent::GetComponentGemName(NetComponentId netComponentId) const
    {
        return GetMultiplayerComponentRegistry()->GetComponentGemName(netComponentId);
    }

    const char* MultiplayerSystemComponent::GetComponentName(NetComponentId netComponentId) const
    {
        return GetMultiplayerComponentRegistry()->GetComponentName(netComponentId);
    }

    const char* MultiplayerSystemComponent::GetComponentPropertyName(NetComponentId netComponentId, PropertyIndex propertyIndex) const
    {
        return GetMultiplayerComponentRegistry()->GetComponentPropertyName(netComponentId, propertyIndex);
    }

    const char* MultiplayerSystemComponent::GetComponentRpcName(NetComponentId netComponentId, RpcIndex rpcIndex) const
    {
        return GetMultiplayerComponentRegistry()->GetComponentRpcName(netComponentId, rpcIndex);
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

    void host([[maybe_unused]] const AZ::ConsoleCommandContainer& arguments)
    {
        Multiplayer::MultiplayerAgentType serverType = sv_isDedicated ? MultiplayerAgentType::DedicatedServer : MultiplayerAgentType::ClientServer;
        AZ::Interface<IMultiplayer>::Get()->InitializeMultiplayer(serverType);
        INetworkInterface* networkInterface = AZ::Interface<INetworking>::Get()->RetrieveNetworkInterface(AZ::Name(s_networkInterfaceName));
        networkInterface->Listen(sv_port);
    }
    AZ_CONSOLEFREEFUNC(host, AZ::ConsoleFunctorFlags::DontReplicate, "Opens a multiplayer connection as a host for other clients to connect to");

    void connect([[maybe_unused]] const AZ::ConsoleCommandContainer& arguments)
    {
        AZ::Interface<IMultiplayer>::Get()->InitializeMultiplayer(MultiplayerAgentType::Client);
        INetworkInterface* networkInterface = AZ::Interface<INetworking>::Get()->RetrieveNetworkInterface(AZ::Name(s_networkInterfaceName));

        if (arguments.size() < 1)
        {
            const AZ::CVarFixedString remoteAddress = cl_serveraddr;
            const IpAddress ipAddress(remoteAddress.c_str(), cl_serverport, networkInterface->GetType());
            networkInterface->Connect(ipAddress);
            return;
        }

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
        const IpAddress ipAddress(addressStr, aznumeric_cast<uint16_t>(portNumber), networkInterface->GetType());
        networkInterface->Connect(ipAddress);
    }
    AZ_CONSOLEFREEFUNC(connect, AZ::ConsoleFunctorFlags::DontReplicate, "Opens a multiplayer connection to a remote host");

    void disconnect([[maybe_unused]] const AZ::ConsoleCommandContainer& arguments)
    {
        AZ::Interface<IMultiplayer>::Get()->InitializeMultiplayer(MultiplayerAgentType::Uninitialized);
        INetworkInterface* networkInterface = AZ::Interface<INetworking>::Get()->RetrieveNetworkInterface(AZ::Name(s_networkInterfaceName));
        auto visitor = [](IConnection& connection) { connection.Disconnect(DisconnectReason::TerminatedByUser, TerminationEndpoint::Local); };
        networkInterface->GetConnectionSet().VisitConnections(visitor);
    }
    AZ_CONSOLEFREEFUNC(disconnect, AZ::ConsoleFunctorFlags::DontReplicate, "Disconnects any open multiplayer connections");
}
