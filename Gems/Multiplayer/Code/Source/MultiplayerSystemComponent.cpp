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
#include <AzCore/Interface/Interface.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ILogger.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzNetworking/Framework/INetworking.h>

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
    AZ_CVAR(bool, sv_isdedicated, true, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Whether the host command creates an independent or client hosted server");

    void MultiplayerSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MultiplayerSystemComponent, AZ::Component>()
                ->Version(1);
        }
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
    }

    void MultiplayerSystemComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
    }

    void MultiplayerSystemComponent::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        AZ::TimeMs elapsedMs = aznumeric_cast<AZ::TimeMs>(aznumeric_cast<int64_t>(deltaTime / 1000.0f));

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
        IConnection* connection,
        [[maybe_unused]] const IPacketHeader& packetHeader,
        [[maybe_unused]] const MultiplayerPackets::Connect& packet
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
        [[maybe_unused]] IConnection* connection,
        [[maybe_unused]] const IPacketHeader& packetHeader,
        [[maybe_unused]] const MultiplayerPackets::Accept& packet
    )
    {
        AZ::CVarFixedString commandString = "sv_map " + packet.GetMap();
        AZ::Interface<AZ::IConsole>::Get()->PerformCommand(commandString.c_str());

        // This is a bit tricky, so it warrants extra commenting
        // The cry level loader has a 'map' command used to invoke the level load system
        // We don't want any explicit cry dependencies, so instead we rely on the 
        // az console binding inside SystemInit to echo any unhandled commands to
        // the cry console by stripping off the prefix 'sv_'
        AZ::Interface<AZ::IConsole>::Get()->PerformCommand(commandString.c_str() + 3);
        return true;
    }

    bool MultiplayerSystemComponent::HandleRequest
    (
        [[maybe_unused]] IConnection* connection,
        [[maybe_unused]] const IPacketHeader& packetHeader,
        const MultiplayerPackets::SyncConsole& packet
    )
    {
        ExecuteConsoleCommandList(connection, packet.GetCommandSet());
        return true;
    }

    bool MultiplayerSystemComponent::HandleRequest
    (
        [[maybe_unused]] IConnection* connection,
        [[maybe_unused]] const IPacketHeader& packetHeader,
        const MultiplayerPackets::ConsoleCommand& packet
    )
    {
        const bool isAcceptor = (connection->GetConnectionRole() == ConnectionRole::Acceptor); // We're hosting if we accepted the connection
        const AZ::ConsoleFunctorFlags requiredSet = isAcceptor ? AZ::ConsoleFunctorFlags::AllowClientSet : AZ::ConsoleFunctorFlags::Null;
        AZ::Interface<AZ::IConsole>::Get()->PerformCommand(packet.GetCommand().c_str(), AZ::ConsoleSilentMode::NotSilent, AZ::ConsoleInvokedFrom::AzNetworking, requiredSet);
        return true;
    }

    bool MultiplayerSystemComponent::HandleRequest
    (
        IConnection* connection,
        [[maybe_unused]] const IPacketHeader& packetHeader,
        const MultiplayerPackets::SyncConnectionCvars& packet
    )
    {
        connection->SetConnectionQuality(ConnectionQuality(packet.GetLossPercent(), packet.GetLatencyMs(), packet.GetVarianceMs()));
        return true;
    }

    bool MultiplayerSystemComponent::HandleRequest
    (
        [[maybe_unused]] IConnection* connection,
        [[maybe_unused]] const IPacketHeader& packetHeader,
        [[maybe_unused]] const MultiplayerPackets::EntityUpdates& packet
    )
    {
        return false;
    }

    bool MultiplayerSystemComponent::HandleRequest
    (
        [[maybe_unused]] IConnection* connection,
        [[maybe_unused]] const IPacketHeader& packetHeader,
        [[maybe_unused]] const MultiplayerPackets::EntityRpcs& packet
    )
    {
        return false;
    }

    bool MultiplayerSystemComponent::HandleRequest
    (
        [[maybe_unused]] IConnection* connection,
        [[maybe_unused]] const IPacketHeader& packetHeader,
        [[maybe_unused]] const MultiplayerPackets::ClientMigration& packet
    )
    {
        return false;
    }

    bool MultiplayerSystemComponent::HandleRequest
    (
        [[maybe_unused]] AzNetworking::IConnection* connection,
        [[maybe_unused]] const AzNetworking::IPacketHeader& packetHeader,
        [[maybe_unused]] const MultiplayerPackets::NotifyClientMigration& packet
    )
    {
        return false;
    }

    bool MultiplayerSystemComponent::HandleRequest
    (
        [[maybe_unused]] AzNetworking::IConnection* connection,
        [[maybe_unused]] const AzNetworking::IPacketHeader& packetHeader,
        [[maybe_unused]] const MultiplayerPackets::EntityMigration& packet
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

    void MultiplayerSystemComponent::OnConnect(IConnection* connection)
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
    }

    bool MultiplayerSystemComponent::OnPacketReceived(IConnection* connection, const IPacketHeader& packetHeader, ISerializer& serializer)
    {
        return MultiplayerPackets::DispatchPacket(connection, packetHeader, serializer, *this);
    }

    void MultiplayerSystemComponent::OnPacketLost([[maybe_unused]] IConnection* connection, [[maybe_unused]] PacketId packetId)
    {
        ;
    }

    void MultiplayerSystemComponent::OnDisconnect(IConnection* connection, DisconnectReason reason, TerminationEndpoint endpoint)
    {
        const char* endpointString = (endpoint == TerminationEndpoint::Local) ? "Disconnecting" : "Remote host disconnected";
        AZStd::string reasonString = ToString(reason);
        AZLOG_INFO("%s due to %s from remote address: %s", endpointString, reasonString.c_str(), connection->GetRemoteAddress().GetString().c_str());

        // The authority is shutting down its connection
        if (connection->GetConnectionRole() == ConnectionRole::Acceptor)
        {
            m_shutdownEvent.Signal(m_networkInterface);
        }
    }

    MultiplayerAgentType MultiplayerSystemComponent::GetAgentType()
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
            }
        }
        m_agentType = multiplayerType;

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
        Multiplayer::MultiplayerAgentType serverType = sv_isdedicated ? MultiplayerAgentType::DedicatedServer : MultiplayerAgentType::ClientServer;
        INetworkInterface* networkInterface = AZ::Interface<INetworking>::Get()->RetrieveNetworkInterface(AZ::Name(s_networkInterfaceName));
        networkInterface->Listen(sv_port);
        AZ::Interface<IMultiplayer>::Get()->InitializeMultiplayer(serverType);
    }
    AZ_CONSOLEFREEFUNC(host, AZ::ConsoleFunctorFlags::DontReplicate, "Opens a multiplayer connection as a host for other clients to connect to");

    void connect([[maybe_unused]] const AZ::ConsoleCommandContainer& arguments)
    {
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
        AZ::Interface<IMultiplayer>::Get()->InitializeMultiplayer(MultiplayerAgentType::Client);
    }
    AZ_CONSOLEFREEFUNC(connect, AZ::ConsoleFunctorFlags::DontReplicate, "Opens a multiplayer connection to a remote host");

    void disconnect([[maybe_unused]] const AZ::ConsoleCommandContainer& arguments)
    {
        INetworkInterface* networkInterface = AZ::Interface<INetworking>::Get()->RetrieveNetworkInterface(AZ::Name(s_networkInterfaceName));
        auto visitor = [](IConnection& connection) { connection.Disconnect(DisconnectReason::TerminatedByUser, TerminationEndpoint::Local); };
        networkInterface->GetConnectionSet().VisitConnections(visitor);
    }
    AZ_CONSOLEFREEFUNC(disconnect, AZ::ConsoleFunctorFlags::DontReplicate, "Disconnects any open multiplayer connections");
}
