/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Multiplayer/MultiplayerConstants.h>
#include <Multiplayer/Components/MultiplayerComponent.h>
#include <Multiplayer/Components/NetworkHierarchyRootComponent.h>
#include <Multiplayer/IMultiplayerSpawner.h>
#include <MultiplayerSystemComponent.h>
#include <ConnectionData/ClientToServerConnectionData.h>
#include <ConnectionData/ServerToClientConnectionData.h>
#include <EntityDomains/FullOwnershipEntityDomain.h>
#include <EntityDomains/NullEntityDomain.h>
#include <ReplicationWindows/NullReplicationWindow.h>
#include <ReplicationWindows/ServerToClientReplicationWindow.h>
#include <Source/AutoGen/AutoComponentTypes.h>
#include <Multiplayer/Session/ISessionRequests.h>
#include <Multiplayer/Session/SessionConfig.h>
#include <Multiplayer/MultiplayerPerformanceStats.h>
#include <Multiplayer/MultiplayerMetrics.h>

#include <AzCore/Debug/Profiler.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Component/ComponentApplicationLifecycle.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ILogger.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzFramework/Components/CameraBus.h>
#include <AzFramework/Visibility/IVisibilitySystem.h>

#include <AzNetworking/Framework/INetworking.h>
#include <AzFramework/Process/ProcessWatcher.h>

#include <cmath>
#include <AzCore/Jobs/JobCompletion.h>
#include <AzCore/Jobs/JobFunction.h>
#include <System/PhysXSystem.h>

#include <AzCore/Jobs/JobCompletion.h>
#include <AzCore/Jobs/JobFunction.h>

AZ_DEFINE_BUDGET(MULTIPLAYER);

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(Multiplayer::MultiplayerAgentType, "{53EA1938-5FFB-4305-B50A-D20730E8639B}");
}


namespace AZ::ConsoleTypeHelpers
{
    inline CVarFixedString ValueToString(const AzNetworking::ProtocolType& value)
    {
        return (value == AzNetworking::ProtocolType::Tcp) ? "tcp" : "udp";
    }

    inline bool StringSetToValue(AzNetworking::ProtocolType& outValue, const ConsoleCommandContainer& arguments)
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
    AZ_CVAR(uint16_t, sv_portRange, 999, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "The range of ports the host will incrementally attempt to bind to when initializing");
    AZ_CVAR(AZ::CVarFixedString, sv_map, "", nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "The map the server should load");
    AZ_CVAR(ProtocolType, sv_protocol, ProtocolType::Udp, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "This flag controls whether we use TCP or UDP for game networking");
    AZ_CVAR(bool, sv_isDedicated, true, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Whether the host command creates an independent or client hosted server");
    AZ_CVAR(bool, sv_isTransient, true, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "[DEPRECATED: use sv_terminateOnPlayerExit instead] Whether a dedicated server shuts down if all existing connections disconnect.");
    AZ_CVAR(bool, sv_terminateOnPlayerExit, true, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Whether a dedicated server shuts down if all existing connections disconnect.");
    AZ_CVAR(AZ::TimeMs, sv_serverSendRateMs, AZ::TimeMs{ 50 }, nullptr, AZ::ConsoleFunctorFlags::Null, "Minimum number of milliseconds between each network update");
    AZ_CVAR(float, cl_renderTickBlendBase, 0.15f, nullptr, AZ::ConsoleFunctorFlags::Null,
        "The base used for blending between network updates, 0.1 will be quite linear, 0.2 or 0.3 will "
        "slow down quicker and may be better suited to connections with highly variable latency");
    AZ_CVAR(bool, bg_multiplayerDebugDraw, false, nullptr, AZ::ConsoleFunctorFlags::Null, "Enables debug draw for the multiplayer gem");
    AZ_CVAR(bool, sv_dedicated_host_onstartup, true, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Whether dedicated servers will begin hosting on app startup.");
    AZ_CVAR(bool, cl_connect_onstartup, false, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "[DEPRECATED: use connect instead] Whether to call connect as soon as the Multiplayer SystemComponent is activated.");
    AZ_CVAR(bool, sv_versionMismatch_autoDisconnect, true, nullptr, AZ::ConsoleFunctorFlags::DontReplicate,
        "Should the server automatically disconnect a client that is attempting connect who is running a build containing different/modified multiplayer components.");
    AZ_CVAR(bool, sv_versionMismatch_sendManifestToClient, true, nullptr, AZ::ConsoleFunctorFlags::DontReplicate,
        "Should the server send all its individual multiplayer component version information to the client when there's a mismatch? "
        "Upon receiving the information, the client will print the mismatch information to the game log. "
        "Provided for debugging during development, but you may want to mark false for release builds.");
    AZ_CVAR(
        bool,
        sv_versionMismatch_check_enabled,
        true,
        nullptr,
        AZ::ConsoleFunctorFlags::DontReplicate,
        "If true, the server will check that client version of multiplayer component matches the server's.");

    AZ_CVAR(bool, bg_capturePhysicsTickMetric, true, nullptr, AZ::ConsoleFunctorFlags::DontReplicate,
        "Should the Multiplayer gem record average physics tick time?");
    AZ_CVAR(bool, bg_captureTransportMetrics, true, nullptr, AZ::ConsoleFunctorFlags::DontReplicate,
        "Should the Multiplayer gem record transport metrics?");
    AZ_CVAR(AzNetworking::ProtocolType, bg_captureTransportType, AzNetworking::ProtocolType::Udp, nullptr, AZ::ConsoleFunctorFlags::DontReplicate,
        "Capture either UDP or TCP transport metrics.");
    AZ_CVAR(AZ::TimeMs, bg_captureTransportPeriod, AZ::TimeMs{1000}, nullptr, AZ::ConsoleFunctorFlags::DontReplicate,
        "How often in milliseconds to record transport metrics.");

    AZ_CVAR(bool, sv_multithreadedConnectionUpdates, false, nullptr, AZ::ConsoleFunctorFlags::DontReplicate,
        "If true, the server will send updates to clients on different threads, which improves performance with large number of clients");
    AZ_CVAR(bool, bg_parallelNotifyPreRender, false, nullptr, AZ::ConsoleFunctorFlags::DontReplicate,
        "If true, OnPreRender events will be sent in parallel from job threads. Please make sure the handlers of the event are thread safe.");
    

    void MultiplayerSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        NetworkSpawnable::Reflect(context);
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MultiplayerSystemComponent, AZ::Component>()
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
            behaviorContext->Class<NetEntityId>();
            behaviorContext->Class<NetComponentId>();
            behaviorContext->Class<PropertyIndex>();
            behaviorContext->Class<RpcIndex>();
            behaviorContext->Class<ClientInputId>();
            behaviorContext->Class<HostFrameId>();

            behaviorContext->Enum<static_cast<int>(MultiplayerAgentType::Uninitialized)>("MultiplayerAgentType_Uninitialized")
                ->Enum<static_cast<int>(MultiplayerAgentType::Client)>("MultiplayerAgentType_Client")
                ->Enum<static_cast<int>(MultiplayerAgentType::ClientServer)>("MultiplayerAgentType_ClientServer")
                ->Enum<static_cast<int>(MultiplayerAgentType::DedicatedServer)>("MultiplayerAgentType_DedicatedServer");

            behaviorContext->Class<MultiplayerSystemComponent>("MultiplayerSystemComponent")
                ->Attribute(AZ::Script::Attributes::Module, "multiplayer")
                ->Attribute(AZ::Script::Attributes::Category, "Multiplayer")
                ->Method("GetOnEndpointDisconnectedEvent", []() -> EndpointDisconnectedEvent*
                {
                    const auto mpComponent = static_cast<MultiplayerSystemComponent*>(AZ::Interface<IMultiplayer>::Get());
                    if (!mpComponent)
                    {
                        AZ_Assert(false, "GetOnEndpointDisconnectedEvent failed to find the multiplayer system component. Please update behavior context to properly retrieve the event.");
                        return nullptr;
                    }

                    return &mpComponent->m_endpointDisconnectedEvent;
                })
                    ->Attribute(
                        AZ::Script::Attributes::AzEventDescription,
                        AZ::BehaviorAzEventDescription{ "On Endpoint Disconnected Event", { "Type of Multiplayer Agent that disconnected" } })
                ->Method("ClearAllEntities", []()
                {
                    const auto mpComponent = static_cast<MultiplayerSystemComponent*>(AZ::Interface<IMultiplayer>::Get());
                    if (!mpComponent)
                    {
                        AZ_Assert( false, "ClearAllEntities failed to find the multiplayer system component. Please update behavior context to properly clear all entities.");
                        return;
                    }

                    mpComponent->GetNetworkEntityManager()->ClearAllEntities();
                })
                ->Method("GetCurrentBlendFactor", []()
                {
                    if (const IMultiplayer* multiplayerSystem = GetMultiplayer())
                    {
                        return multiplayerSystem->GetCurrentBlendFactor();
                    }
                    return 0.f;
                })
            ;
        }

        MultiplayerComponent::Reflect(context);
        NetworkTime::Reflect(context);
    }

    void MultiplayerSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("NetworkingService"));
        required.push_back(AZ_CRC_CE("MultiplayerStatSystemComponent"));
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
        , m_autonomousEntityReplicatorCreatedHandler([this]([[maybe_unused]] NetEntityId netEntityId) { OnAutonomousEntityReplicatorCreated(); })
    {
        AZ::Interface<IMultiplayer>::Register(this);
    }

    MultiplayerSystemComponent::~MultiplayerSystemComponent()
    {
        AZ::Interface<IMultiplayer>::Unregister(this);
    }

    void MultiplayerSystemComponent::Activate()
    {
#if (O3DE_EDITOR_CONNECTION_LISTENER_ENABLE)
        m_editorConnectionListener = AZStd::make_unique<MultiplayerEditorConnection>();
#endif

        RegisterMetrics();

        AzFramework::RootSpawnableNotificationBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();
        SessionNotificationBus::Handler::BusConnect();
        AzFramework::LevelLoadBlockerBus::Handler::BusConnect();
        const AZ::Name interfaceName = AZ::Name(MpNetworkInterfaceName);
        m_networkInterface = AZ::Interface<INetworking>::Get()->CreateNetworkInterface(interfaceName, sv_protocol, TrustZone::ExternalClientToServer, *this);

        AZ::Interface<ISessionHandlingClientRequests>::Register(this);

        //! Register our gems multiplayer components to assign NetComponentIds
        RegisterMultiplayerComponents();

        if (auto console = AZ::Interface<AZ::IConsole>::Get())
        {
            m_consoleCommandHandler.Connect(console->GetConsoleCommandInvokedEvent());
        }

        if (bg_captureTransportMetrics)
        {
            m_metricsEvent.Enqueue(bg_captureTransportPeriod, true);
        }

        // Wait for all systems to activate because allowing this server or client to host or connect.
        // Connecting too soon causes a "version mismatch" because all of the system components haven't registered their multiplayer components.
        if (const auto settingsRegistry = AZ::SettingsRegistry::Get())
        {
            AZ::ComponentApplicationLifecycle::RegisterHandler(
                *settingsRegistry,
                m_componentApplicationLifecycleHandler,
                [this](const AZ::SettingsRegistryInterface::NotifyEventArgs&)
                {
                    const auto console = AZ::Interface<AZ::IConsole>::Get();
                    if (!console)
                    {
                        AZ_Assert(false, "Multiplayer system is attempting to register console commands before AZ::Console is available.");
                        return;
                    }

                    // It's now safe to register and execute the "host" and "connect" commands
                    m_hostConsoleCommand = AZStd::make_unique<AZ::ConsoleFunctor<MultiplayerSystemComponent, false>>(
                        "host",
                        "Opens a multiplayer connection as a host for other clients to connect to",
                        AZ::ConsoleFunctorFlags::DontReplicate | AZ::ConsoleFunctorFlags::DontDuplicate,
                        AZ::TypeId{},
                        *this,
                        &MultiplayerSystemComponent::HostConsoleCommand);

                    m_connectConsoleCommand = AZStd::make_unique<AZ::ConsoleFunctor<MultiplayerSystemComponent, false>>(
                        "connect",
                        "Opens a multiplayer connection to a remote host",
                        AZ::ConsoleFunctorFlags::DontReplicate | AZ::ConsoleFunctorFlags::DontDuplicate,
                        AZ::TypeId{},
                        *this,
                        &MultiplayerSystemComponent::ConnectConsoleCommand);

                    // ExecuteDeferredConsoleCommands will execute any previously deferred "host" or "connect" commands now that they have been registered with the AZ Console
                    console->ExecuteDeferredConsoleCommands();

                    // Don't access cvars directly (their values might be stale https://github.com/o3de/o3de/issues/5537)
                    bool isDedicatedServer = false;
                    bool dedicatedServerHostOnStartup = false;
                    if (console->GetCvarValue("sv_isDedicated", isDedicatedServer) != AZ::GetValueResult::Success)
                    {
                        AZLOG_WARN("Multiplayer system failed to access cvar on startup (sv_isDedicated).")
                        return;
                    }

                    if (console->GetCvarValue("sv_dedicated_host_onstartup", dedicatedServerHostOnStartup) != AZ::GetValueResult::Success)
                    {
                        AZLOG_WARN("Multiplayer system failed to access cvar on startup (sv_dedicated_host_onstartup).")
                        return;
                    }

                    // Dedicated servers will automatically begin hosting
                    if (isDedicatedServer && dedicatedServerHostOnStartup)
                    {
                        this->StartHosting(sv_port, /*is dedicated*/ true);
                    }
                },
                "SystemComponentsActivated",
                /*autoRegisterEvent*/ true);
        }
    }

    void MultiplayerSystemComponent::RegisterMetrics()
    {
        DECLARE_PERFORMANCE_STAT_GROUP(MultiplayerGroup_Networking, "Networking");
        DECLARE_PERFORMANCE_STAT(MultiplayerGroup_Networking, MultiplayerStat_EntityCount, "NumEntities");
        DECLARE_PERFORMANCE_STAT(MultiplayerGroup_Networking, MultiplayerStat_FrameTimeUs, "FrameTimeUs");
        DECLARE_PERFORMANCE_STAT(MultiplayerGroup_Networking, MultiplayerStat_ClientConnectionCount, "ClientConnections");
        DECLARE_PERFORMANCE_STAT(MultiplayerGroup_Networking, MultiplayerStat_ApplicationFrameTimeUs, "AppFrameTimeUs");
        DECLARE_PERFORMANCE_STAT(MultiplayerGroup_Networking, MultiplayerStat_DesyncCorrections, "DesyncCorrections");

        DECLARE_PERFORMANCE_STAT(MultiplayerGroup_Networking, MultiplayerStat_TotalTimeSpentUpdatingMs, "TotalTimeSpentUpdatingMs");
        DECLARE_PERFORMANCE_STAT(MultiplayerGroup_Networking, MultiplayerStat_TotalSendTimeMs, "TotalSendTimeMs");
        DECLARE_PERFORMANCE_STAT(MultiplayerGroup_Networking, MultiplayerStat_TotalSentPackets, "TotalSentPackets");
        DECLARE_PERFORMANCE_STAT(MultiplayerGroup_Networking, MultiplayerStat_TotalSentBytesAfterCompression, "TotalSentBytesAfterCompression");
        DECLARE_PERFORMANCE_STAT(MultiplayerGroup_Networking, MultiplayerStat_TotalSentBytesBeforeCompression, "TotalSentBytesBeforeCompression");
        DECLARE_PERFORMANCE_STAT(MultiplayerGroup_Networking, MultiplayerStat_TotalResentPacketsDueToPacketLoss, "TotalResentPacketsDueToPacketLoss");
        DECLARE_PERFORMANCE_STAT(MultiplayerGroup_Networking, MultiplayerStat_TotalReceiveTimeInMs, "TotalReceiveTimeInMs");
        DECLARE_PERFORMANCE_STAT(MultiplayerGroup_Networking, MultiplayerStat_TotalReceivedPackets, "TotalReceivedPackets");
        DECLARE_PERFORMANCE_STAT(MultiplayerGroup_Networking, MultiplayerStat_TotalReceivedBytesAfterCompression, "TotalReceivedBytesAfterCompression");
        DECLARE_PERFORMANCE_STAT(MultiplayerGroup_Networking, MultiplayerStat_TotalReceivedBytesBeforeCompression, "TotalReceivedBytesBeforeCompression");
        DECLARE_PERFORMANCE_STAT(MultiplayerGroup_Networking, MultiplayerStat_TotalPacketsDiscardedDueToLoad, "TotalPacketsDiscardedDueToLoad");

        DECLARE_PERFORMANCE_STAT(MultiplayerGroup_Networking, MultiplayerStat_PhysicsFrameTimeUs, "PhysicsFrameTimeUs");        
    }

    void MultiplayerSystemComponent::Deactivate()
    {
        m_hostConsoleCommand.reset();
        m_preSimulateHandler.Disconnect();
        m_postSimulateHandler.Disconnect();

        m_metricsEvent.RemoveFromQueue();
        AZ::Interface<ISessionHandlingClientRequests>::Unregister(this);
        m_consoleCommandHandler.Disconnect();
        const AZ::Name interfaceName = AZ::Name(MpNetworkInterfaceName);
        AZ::Interface<INetworking>::Get()->DestroyNetworkInterface(interfaceName);
        AzFramework::LevelLoadBlockerBus::Handler::BusDisconnect();
        SessionNotificationBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
        AzFramework::RootSpawnableNotificationBus::Handler::BusDisconnect();

        m_networkEntityManager.Reset();

#if (O3DE_EDITOR_CONNECTION_LISTENER_ENABLE)
        m_editorConnectionListener.reset();
#endif
    }

    bool MultiplayerSystemComponent::StartHosting(uint16_t port, bool isDedicated)
    {
        if (IsHosting())
        {
            AZLOG_WARN("Already hosting on port %u, new host request ignored (request is for port %u).",
                m_networkInterface->GetPort(), static_cast<uint32_t>(sv_port));
            return false;
        }

        if (port == UseDefaultHostPort)
        {
            port = sv_port;
        }

        if (port != sv_port)
        {
            sv_port = port;
        }

        const uint16_t maxPort = sv_port + sv_portRange;
        while (sv_port <= maxPort)
        {
            if (m_networkInterface->Listen(sv_port))
            {
                InitializeMultiplayer(isDedicated ? MultiplayerAgentType::DedicatedServer : MultiplayerAgentType::ClientServer);
                return true;
            }
            AZLOG_WARN("Failed to start listening on port %u, port is in use?", static_cast<uint32_t>(sv_port));
            sv_port = sv_port + 1;
        }
        return false;
    }

    bool MultiplayerSystemComponent::Connect(const AZStd::string& remoteAddress, uint16_t port, const AZStd::string& connectionTicket)
    {
        const IpAddress address(remoteAddress.c_str(), port, m_networkInterface->GetType());

        if (!address.IsValid())
        {
            AZLOG_ERROR(
                "Failed to connect. Invalid IP-address (remote address='%s', port=%i). Please provide a valid DNS or IP address.",
                remoteAddress.c_str(),
                port);
            return false;
        }

        if (!connectionTicket.empty())
        {
            m_pendingConnectionTickets.push(connectionTicket);
        }

        InitializeMultiplayer(MultiplayerAgentType::Client);
        return m_networkInterface->Connect(address, cl_clientport) != InvalidConnectionId;
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
        }

        // Clear out all the registered network entities
        GetNetworkEntityManager()->ClearAllEntities();

        InitializeMultiplayer(MultiplayerAgentType::Uninitialized);

        // Signal session management, do this after uninitializing state
        if (agentType == MultiplayerAgentType::DedicatedServer || agentType == MultiplayerAgentType::ClientServer)
        {
            if (AZ::Interface<ISessionHandlingProviderRequests>::Get() != nullptr)
            {
                AZ::Interface<ISessionHandlingProviderRequests>::Get()->HandleDestroySession();
            }
        }
    }

    bool MultiplayerSystemComponent::RequestPlayerJoinSession(const SessionConnectionConfig& config)
    {
        const AZStd::string remoteAddress = config.m_dnsName.empty() ? config.m_ipAddress : config.m_dnsName;
        return Connect(remoteAddress, config.m_port, config.m_playerSessionId);
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

    bool MultiplayerSystemComponent::IsHosting() const
    {
        return (GetAgentType() == MultiplayerAgentType::ClientServer) || (GetAgentType() == MultiplayerAgentType::DedicatedServer);
    }


    bool MultiplayerSystemComponent::OnCreateSessionBegin(const SessionConfig& sessionConfig)
    {
        // Check if session manager has a certificate for us and pass it along if so
        auto console = AZ::Interface<AZ::IConsole>::Get();
        if (console != nullptr)
        {
            bool tcpUseEncryption = false;
            console->GetCvarValue("net_TcpUseEncryption", tcpUseEncryption);
            bool udpUseEncryption = false;
            console->GetCvarValue("net_UdpUseEncryption", udpUseEncryption);
            auto sessionProviderHandler = AZ::Interface<ISessionHandlingProviderRequests>::Get();
            if ((tcpUseEncryption || udpUseEncryption) && sessionProviderHandler != nullptr)
            {
                AZ::CVarFixedString externalCertPath = AZ::CVarFixedString(sessionProviderHandler->GetExternalSessionCertificate().c_str());
                if (!externalCertPath.empty())
                {
                    AZ::CVarFixedString commandString = "net_SslExternalCertificateFile " + externalCertPath;
                    console->PerformCommand(commandString.c_str());
                }
            }
        }

        Multiplayer::MultiplayerAgentType serverType = sv_isDedicated ? MultiplayerAgentType::DedicatedServer : MultiplayerAgentType::ClientServer;
        InitializeMultiplayer(serverType);

        // Load a multiplayer level if there's a session property called the "level"...
        if (const auto& levelName = sessionConfig.m_sessionProperties.find("level");
            console != nullptr && levelName != sessionConfig.m_sessionProperties.end())
        {
            AZStd::string loadLevelCommand = "loadlevel " + levelName->second;
            console->PerformCommand(loadLevelCommand.c_str());
        }

        return m_networkInterface->Listen(sessionConfig.m_port);
    }

    void MultiplayerSystemComponent::OnCreateSessionEnd()
    {
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
        }
        InitializeMultiplayer(MultiplayerAgentType::Uninitialized);

        return true;
    }

    void MultiplayerSystemComponent::OnDestroySessionEnd()
    {
    }

    void MultiplayerSystemComponent::OnUpdateSessionBegin(const SessionConfig& sessionConfig, const AZStd::string& updateReason)
    {
        AZ_UNUSED(sessionConfig);
        AZ_UNUSED(updateReason);
    }

    void MultiplayerSystemComponent::OnUpdateSessionEnd()
    {
    }

    void MultiplayerSystemComponent::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        AZ_PROFILE_SCOPE(MULTIPLAYER, "MultiplayerSystemComponent: OnTick");
        SET_PERFORMANCE_STAT(MultiplayerStat_ApplicationFrameTimeUs, AZ::SecondsToTimeUs(deltaTime));

        const AZStd::chrono::steady_clock::time_point startMultiplayerTickTime = AZStd::chrono::steady_clock::now();

        if (bg_multiplayerDebugDraw)
        {
            m_networkEntityManager.DebugDraw();
        }

        const AZ::TimeMs deltaTimeMs = aznumeric_cast<AZ::TimeMs>(static_cast<int32_t>(deltaTime * 1000.0f));
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
        
        // Metrics calculation, as update calls are threaded.
        UpdatedMetricsConnectionCount();

        // Send out the game state update to all connections
        UpdateConnections();

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
            AZ_PROFILE_SCOPE(MULTIPLAYER, "MultiplayerSystemComponent: OnTick - SendReliablePackets");
            m_networkInterface->GetConnectionSet().VisitConnections(visitor);
        }

        const auto duration =
            AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(AZStd::chrono::steady_clock::now() - startMultiplayerTickTime);
        stats.RecordFrameTime(AZ::TimeUs{ duration.count() });
    }

    void MultiplayerSystemComponent::UpdatedMetricsConnectionCount()
    {
        MultiplayerStats& stats = GetStats();
        auto updateMetrics = [&stats](IConnection& connection)
        {
            if (connection.GetUserData() != nullptr)
            {
                IConnectionData* connectionData = reinterpret_cast<IConnectionData*>(connection.GetUserData());
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
        m_networkInterface->GetConnectionSet().VisitConnections(updateMetrics);
    }

    void MultiplayerSystemComponent::UpdateConnections()
    {
        if (sv_multithreadedConnectionUpdates && (GetAgentType() == MultiplayerAgentType::ClientServer ||
                                                  GetAgentType() == MultiplayerAgentType::DedicatedServer))
        {
            // Threaded update calls.
            AZ_PROFILE_SCOPE(MULTIPLAYER, "MultiplayerSystemComponent: UpdateConnections");

            AZ::JobCompletion jobCompletion;

            auto sendNetworkUpdates = [&jobCompletion](IConnection& connection)
            {
                AZ::Job* job = AZ::CreateJobFunction([&connection]()
                    {
                        if (connection.GetUserData() != nullptr)
                        {
                            IConnectionData* connectionData = static_cast<IConnectionData*>(connection.GetUserData());
                            connectionData->Update();
                        }
                    }, true /*auto delete*/, nullptr);

                job->SetDependent(&jobCompletion);
                job->Start();
            };

            m_networkInterface->GetConnectionSet().VisitConnections(sendNetworkUpdates);
            jobCompletion.StartAndWaitForCompletion();
        }
        else // On clients (including the Editor) run in a single threaded mode to avoid issues in UI asset loading
        {
            AZ_PROFILE_SCOPE(MULTIPLAYER, "MultiplayerSystemComponent: OnTick - SendOutGameStateUpdate");

            auto sendNetworkUpdates = [](IConnection& connection)
            {
                if (connection.GetUserData() != nullptr)
                {
                    IConnectionData* connectionData = reinterpret_cast<IConnectionData*>(connection.GetUserData());
                    connectionData->Update();
                }
            };

            m_networkInterface->GetConnectionSet().VisitConnections(sendNetworkUpdates);
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

    bool MultiplayerSystemComponent::IsHandshakeComplete(AzNetworking::IConnection* connection) const
    {
        return reinterpret_cast<IConnectionData*>(connection->GetUserData())->DidHandshake();
    }

    bool MultiplayerSystemComponent::AttemptPlayerConnect(AzNetworking::IConnection* connection, MultiplayerPackets::Connect& packet)
    {
        reinterpret_cast<ServerToClientConnectionData*>(connection->GetUserData())->SetProviderTicket(packet.GetTicket().c_str());

        const char* levelName = AZ::Interface<AzFramework::ILevelSystemLifecycle>::Get()->GetCurrentLevelName();
        if (!levelName || *levelName == '\0')
        {
            AZLOG_WARN(
                "Server does not have a multiplayer level loaded! Make sure the server has a level loaded before accepting clients.");
            m_noServerLevelLoadedEvent.Signal();

            connection->Disconnect(DisconnectReason::ServerNoLevelLoaded, TerminationEndpoint::Local);
            return true;
        }

        // Hosts will handle spawning for a player on connect
        if (GetAgentType() == MultiplayerAgentType::ClientServer
            || GetAgentType() == MultiplayerAgentType::DedicatedServer)
        {
            // We use a temporary userId over the clients address so we can maintain client lookups even in the event of wifi handoff
            IMultiplayerSpawner* spawner = AZ::Interface<IMultiplayerSpawner>::Get();
            NetworkEntityHandle controlledEntity;

            // Check rejoin data first
            const auto node = m_playerRejoinData.find(packet.GetTemporaryUserId());
            if (node != m_playerRejoinData.end())
            {
                controlledEntity = m_networkEntityManager.GetNetworkEntityTracker()->Get(node->second);
            }
            else if (spawner)
            {
                // Route to spawner implementation
                MultiplayerAgentDatum datum;
                datum.m_agentType = MultiplayerAgentType::Client;
                datum.m_id = connection->GetConnectionId();
                const uint64_t userId = packet.GetTemporaryUserId();

                controlledEntity = spawner->OnPlayerJoin(userId, datum);
                if (controlledEntity.Exists())
                {
                    EnableAutonomousControl(controlledEntity, connection->GetConnectionId());
                    StartServerToClientReplication(userId, controlledEntity, connection);
                }
                else
                {
                    // If there wasn't a player entity available, wait until a level loads and check again.
                    // This can happen if IMultiplayerSpawn depends on a level being loaded, but the client connects to the server before the server has started a level.
                    m_playersWaitingToBeSpawned.emplace_back(userId, datum, connection);
                }
            }
            else
            {
                // There's no player spawner, maybe the level's entities aren't finished activating
                if (!m_levelEntitiesActivated)
                {
                    // Remember this player, and spawn it once the level entities finish activating
                    MultiplayerAgentDatum datum;
                    datum.m_agentType = MultiplayerAgentType::Client;
                    datum.m_id = connection->GetConnectionId();
                    const uint64_t userId = packet.GetTemporaryUserId();
                    m_playersWaitingToBeSpawned.emplace_back(userId, datum, connection);
                }
                else
                {
                    AZLOG_ERROR("No IMultiplayerSpawner was available. Ensure that one is registered for usage on PlayerJoin.");
                }
            }
        }

        if (connection->SendReliablePacket(MultiplayerPackets::Accept(levelName)))
        {
            reinterpret_cast<ServerToClientConnectionData*>(connection->GetUserData())->SetDidHandshake(true);

            if (packet.GetTemporaryUserId() == 0)
            {
                // Sync our console
                ConsoleReplicator consoleReplicator(connection);
                AZ::Interface<AZ::IConsole>::Get()->VisitRegisteredFunctors([&consoleReplicator](AZ::ConsoleFunctorBase* functor) { consoleReplicator.Visit(functor); });
            }
            return true;
        }
        return false;
    }

    bool MultiplayerSystemComponent::HandleRequest
    (
        AzNetworking::IConnection* connection,
        [[maybe_unused]] const IPacketHeader& packetHeader,
        MultiplayerPackets::Connect& packet
    )
    {
        PlayerConnectionConfig config;
        config.m_playerConnectionId = aznumeric_cast<uint32_t>(connection->GetConnectionId());
        config.m_playerSessionId = packet.GetTicket();

        // Validate our session with the provider if any
        ISessionHandlingProviderRequests* sessionRequests = AZ::Interface<ISessionHandlingProviderRequests>::Get();
        if (sessionRequests != nullptr)
        {
            if (!sessionRequests->ValidatePlayerJoinSession(config))
            {
                auto visitor = [](IConnection& connection) { connection.Disconnect(DisconnectReason::TerminatedByUser, TerminationEndpoint::Local); };
                m_networkInterface->GetConnectionSet().VisitConnections(visitor);
                return true;
            }
        }

        // Make sure the client that's trying to connect has the same multiplayer components
        if (sv_versionMismatch_check_enabled && GetMultiplayerComponentRegistry()->GetSystemVersionHash() != packet.GetSystemVersionHash())
        {
            // There's a multiplayer component mismatch. Send the server's component information back to the client so they can compare.
            if (sv_versionMismatch_sendManifestToClient)
            {
                MultiplayerPackets::VersionMismatch versionMismatchPacket(GetMultiplayerComponentRegistry()->GetMultiplayerComponentVersionHashes());
                connection->SendReliablePacket(versionMismatchPacket);                
            }
            else
            {
                // sv_versionMismatch_sendManifestToClient is false; don't send any individual components, just let the client know there was a mismatch.
                MultiplayerPackets::VersionMismatch versionMismatchPacket;
                connection->SendReliablePacket(versionMismatchPacket);
            }

            m_originalConnectPackets[connection->GetConnectionId()] = packet;
            return true;
        }

        return AttemptPlayerConnect(connection, packet);
    }

    bool MultiplayerSystemComponent::HandleRequest
    (
        [[maybe_unused]] AzNetworking::IConnection* connection,
        [[maybe_unused]] const IPacketHeader& packetHeader,
        [[maybe_unused]] MultiplayerPackets::Accept& packet
    )
    {
        reinterpret_cast<IConnectionData*>(connection->GetUserData())->SetDidHandshake(true);
        if (m_temporaryUserIdentifier == 0)
        {
            sv_map = packet.GetMap().c_str();
            AZ::CVarFixedString loadLevelString = "LoadLevel " + packet.GetMap();
            m_blockClientLoadLevel = false;
            AZ::Interface<AZ::IConsole>::Get()->PerformCommand(loadLevelString.c_str());
            m_blockClientLoadLevel = true;
        }
        else
        {
            // Bypass map loading and immediately ready the connection for updates
            IConnectionData* connectionData = reinterpret_cast<IConnectionData*>(connection->GetUserData());
            if (connectionData)
            {
                connectionData->SetCanSendUpdates(true);

                // @nt: TODO - delete once dropped RPC problem fixed
                // Connection has migrated, we are now waiting for the autonomous entity replicator to be created
                connectionData->GetReplicationManager().AddAutonomousEntityReplicatorCreatedHandler(m_autonomousEntityReplicatorCreatedHandler);
            }
        }

        m_serverAcceptanceReceivedEvent.Signal();
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
        if (connection->GetUserData() == nullptr)
        {
            AZLOG_WARN("Missing connection data, likely due to a connection in the process of closing, entity updates size %u", aznumeric_cast<uint32_t>(packet.GetEntityRpcs().size()));
            return true;
        }

        EntityReplicationManager& replicationManager = reinterpret_cast<IConnectionData*>(connection->GetUserData())->GetReplicationManager();
        return replicationManager.HandleEntityRpcMessages(connection, packet.ModifyEntityRpcs());
    }

    bool MultiplayerSystemComponent::HandleRequest
    (
        [[maybe_unused]] AzNetworking::IConnection* connection,
        [[maybe_unused]] const IPacketHeader& packetHeader,
        [[maybe_unused]] MultiplayerPackets::RequestReplicatorReset& packet
    )
    {
        if (connection->GetUserData() == nullptr)
        {
            AZLOG_WARN("Missing connection data, likely due to a connection in the process of closing");
            return true;
        }

        EntityReplicationManager& replicationManager = reinterpret_cast<IConnectionData*>(connection->GetUserData())->GetReplicationManager();
        return replicationManager.HandleEntityResetMessages(connection, packet.GetEntityIds());
    }

    bool MultiplayerSystemComponent::HandleRequest
    (
        [[maybe_unused]] AzNetworking::IConnection* connection,
        [[maybe_unused]] const IPacketHeader& packetHeader,
        [[maybe_unused]] MultiplayerPackets::ClientMigration& packet
    )
    {
        if (GetAgentType() != MultiplayerAgentType::Client)
        {
            // Only clients are allowed to migrate from one server to another
            return false;
        }

        // Store the temporary user identifier so we can transmit it with our next Connect packet
        // The new server will use this to re-attach our set of autonomous entities
        m_temporaryUserIdentifier = packet.GetTemporaryUserIdentifier();

        // Disconnect our existing server connection
        auto visitor = [](IConnection& connection) { connection.Disconnect(DisconnectReason::ClientMigrated, TerminationEndpoint::Local); };
        m_networkInterface->GetConnectionSet().VisitConnections(visitor);
        AZLOG_INFO("Migrating to new server shard");
        m_clientMigrationStartEvent.Signal(packet.GetLastClientInputId());
        if (m_networkInterface->Connect(packet.GetRemoteServerAddress()) == AzNetworking::InvalidConnectionId)
        {
            AZLOG_ERROR("Failed to connect to new host during client migration event");
        }
        return true;
    }

    bool MultiplayerSystemComponent::HandleRequest(
        IConnection* connection,
        [[maybe_unused]] const IPacketHeader& packetHeader,
        MultiplayerPackets::VersionMismatch& packet)
    {
        // Iterate over each component and see what's been added, missing, or modified
        for (const auto& [theirComponentName, theirComponentHash] : packet.GetComponentVersions())
        {
            // Check for modified components
            AZ::HashValue64 localComponentHash;
            if (GetMultiplayerComponentRegistry()->FindComponentVersionHashByName(theirComponentName, localComponentHash))
            {
                
                if (theirComponentHash != localComponentHash)
                {
                    AZLOG_ERROR(
                        "Multiplayer component mismatch! %s has a different version hash. Please make sure both client and server have "
                        "matching multiplayer components.",
                        theirComponentName.GetCStr());
                }
            }
            else
            {
                // Connected application is using a multiplayer component that doesn't exist in this application
                AZLOG_ERROR(
                    "Multiplayer component mismatch! This application is missing a component with version hash 0x%llx. "
                    "Because this component is missing, the name isn't available, only its hash. "
                    "To find the missing component go to the other machine and search for 's_versionHash = AZ::HashValue64{ 0x%llx }' "
                    "inside the generated multiplayer auto-component build folder.",
                    static_cast<AZ::u64>(theirComponentHash),
                    static_cast<AZ::u64>(theirComponentHash));
            }
        }

        // One last iteration over our components this time to check if we have a component the connected app is missing.
        if (!packet.GetComponentVersions().empty())
        {
            for (const auto& ourComponent : GetMultiplayerComponentRegistry()->GetMultiplayerComponentVersionHashes())
            {
                AZ::Name ourComponentName = ourComponent.first;

                bool theyHaveComponent = false;
                for (const auto& theirComponent : packet.GetComponentVersions())
                {
                    if (ourComponentName == theirComponent.first)
                    {
                        theyHaveComponent = true;
                        break;
                    }
                }

                if (!theyHaveComponent)
                {
                    AZLOG_ERROR(
                        "Multiplayer component mismatch! This application has a component named %s which the connected application is missing!",
                        ourComponentName.GetCStr());
                }
            }
        }
        // The client receives this packet first from the server, and then the client sends a packet back
        if (connection->GetConnectionRole() == ConnectionRole::Connector)
        {
            // If this is the connector (client), send all our component information back to the acceptor (server).
            MultiplayerPackets::VersionMismatch versionMismatchPacket(GetMultiplayerComponentRegistry()->GetMultiplayerComponentVersionHashes());
            connection->SendReliablePacket(versionMismatchPacket);
        }
        else if (connection->GetConnectionRole() == ConnectionRole::Acceptor)
        {
            // If this is the server, that means the client has also received all the component version information by this time.
            // Now either disconnect, or accept the connection even though there's a mismatch.
            if (sv_versionMismatch_autoDisconnect)
            {
                // Disconnect from the connector
                connection->Disconnect(DisconnectReason::VersionMismatch, TerminationEndpoint::Local);
            }
            else
            {
                if (m_originalConnectPackets.contains(connection->GetConnectionId()))
                {
                    // DANGER: Accepting the player connection even though there's a component mismatch
                    AZLOG_WARN("Multiplayer component mismatch was found. Server configured to allow the player to connect anyways. Please set "
                               "sv_versionMismatch_autoDisconnect=true if this is undesired behavior!");
                    AttemptPlayerConnect(connection, m_originalConnectPackets[connection->GetConnectionId()]);
                    m_originalConnectPackets.erase(connection->GetConnectionId());
                }
                else
                {
                    AZ_Assert(false, "Multiplayer component mismatch finished comparing components; "
                                "failed to accept connection because the original connection packet is missing. This should not happen.");
                }   
            }
        }

        m_versionMismatchEvent.Signal();
        return true;
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
        AZStd::string providerTicket;
        if (connection->GetConnectionRole() == ConnectionRole::Connector)
        {
            AZLOG_INFO("New outgoing connection to remote address: %s", connection->GetRemoteAddress().GetString().c_str());
            if (!m_pendingConnectionTickets.empty())
            {
                providerTicket = m_pendingConnectionTickets.front();
                m_pendingConnectionTickets.pop();
            }
            
            connection->SendReliablePacket(MultiplayerPackets::Connect(
                0,
                m_temporaryUserIdentifier,
                providerTicket.c_str(),
                GetMultiplayerComponentRegistry()->GetSystemVersionHash()));
        }
        else
        {
            AZLOG_INFO("New incoming connection from remote address: %s", connection->GetRemoteAddress().GetString().c_str())

            MultiplayerAgentDatum datum;
            datum.m_id = connection->GetConnectionId();
            datum.m_isInvited = false;
            datum.m_agentType = MultiplayerAgentType::Client;
            m_connectionAcquiredEvent.Signal(datum);
        }

        if (GetAgentType() == MultiplayerAgentType::ClientServer
         || GetAgentType() == MultiplayerAgentType::DedicatedServer)
        {
            connection->SetUserData(new ServerToClientConnectionData(connection, *this));
        }
        else
        {
            connection->SetUserData(new ClientToServerConnectionData(connection, *this, providerTicket));
            AZStd::unique_ptr<IReplicationWindow> window = AZStd::make_unique<NullReplicationWindow>(connection);
            reinterpret_cast<ClientToServerConnectionData*>(connection->GetUserData())->GetReplicationManager().SetReplicationWindow(AZStd::move(window));
        }
    }

    AzNetworking::PacketDispatchResult MultiplayerSystemComponent::OnPacketReceived(AzNetworking::IConnection* connection, const IPacketHeader& packetHeader, ISerializer& serializer)
    {
        return MultiplayerPackets::DispatchPacket(connection, packetHeader, serializer, *this);
    }

    void MultiplayerSystemComponent::OnPacketLost([[maybe_unused]] IConnection* connection, [[maybe_unused]] PacketId packetId)
    {
        ;
    }

    void MultiplayerSystemComponent::OnDisconnect(AzNetworking::IConnection* connection, DisconnectReason reason, TerminationEndpoint endpoint)
    {
        const char* endpointString = (endpoint == TerminationEndpoint::Local) ? "Disconnecting" : "Remotely disconnected";
        const AZStd::string reasonString = ToString(reason);
        AZLOG_INFO("%s from remote address %s due to %s", endpointString, connection->GetRemoteAddress().GetString().c_str(), reasonString.c_str());

        // The client is disconnecting
        if (m_agentType == MultiplayerAgentType::Client)
        {
            AZ_Assert(connection->GetConnectionRole() == ConnectionRole::Connector, "Client connection role should only ever be Connector");

            if (reason == DisconnectReason::ServerNoLevelLoaded)
            {
                AZLOG_WARN("Server did not provide a valid level to load! Make sure the server has a level loaded before connecting.");
                m_noServerLevelLoadedEvent.Signal();
            }
        }
        else if (m_agentType == MultiplayerAgentType::DedicatedServer || m_agentType == MultiplayerAgentType::ClientServer)
        {
            // Signal to session management that a user has left the server
            if (connection->GetConnectionRole() == ConnectionRole::Acceptor)
            {
                IMultiplayerSpawner* spawner = AZ::Interface<IMultiplayerSpawner>::Get();
                if (spawner)
                {
                    // Check if this disconnected player was waiting to be spawned, and therefore, doesn't have a controlled player entity yet.
                    bool playerSpawned = true;
                    for (auto it = m_playersWaitingToBeSpawned.begin(); it != m_playersWaitingToBeSpawned.end(); ++it)
                    {
                        if (it->connection && it->connection->GetConnectionId() == connection->GetConnectionId())
                        {
                            m_playersWaitingToBeSpawned.erase(it);
                            playerSpawned = false;
                            break;
                        }
                    }

                    // Alert IMultiplayerSpawner that our spawned player has left.
                    if (playerSpawned)
                    {
                        if (auto connectionData = reinterpret_cast<ServerToClientConnectionData*>(connection->GetUserData()))
                        {
                            if (IReplicationWindow* replicationWindow = connectionData->GetReplicationManager().GetReplicationWindow())
                            {
                                const ReplicationSet& replicationSet = replicationWindow->GetReplicationSet();
                                spawner->OnPlayerLeave(connectionData->GetPrimaryPlayerEntity(), replicationSet, reason);
                            }
                            else
                            {
                                AZLOG_ERROR("No IReplicationWindow found OnPlayerDisconnect.");
                            } 
                        }
                        else
                        {
                            AZLOG_ERROR("No ServerToClientConnectionData found OnPlayerDisconnect.");
                        }
                    }
                }
                else
                {
                    AZLOG_ERROR("No IMultiplayerSpawner found OnPlayerDisconnect. Ensure one is registered.");
                }

                if (AZ::Interface<ISessionHandlingProviderRequests>::Get() != nullptr)
                {
                    PlayerConnectionConfig config;
                    config.m_playerConnectionId = aznumeric_cast<uint32_t>(connection->GetConnectionId());
                    config.m_playerSessionId =
                        reinterpret_cast<ServerToClientConnectionData*>(connection->GetUserData())->GetProviderTicket();
                    AZ::Interface<ISessionHandlingProviderRequests>::Get()->HandlePlayerLeaveSession(config);
                }
            }
        }

        m_endpointDisconnectedEvent.Signal(m_agentType);

        // Clean up any multiplayer connection data we've bound to this connection instance
        if (connection->GetUserData() != nullptr)
        {
            auto connectionData = reinterpret_cast<IConnectionData*>(connection->GetUserData());
            delete connectionData;
            connection->SetUserData(nullptr);
        }

        // Signal to session management when there are no remaining players in a dedicated server for potential cleanup
        // We avoid this for client server as the host itself is a user and dedicated servers that do not terminate when all players have exited
        if (sv_terminateOnPlayerExit && m_agentType == MultiplayerAgentType::DedicatedServer && connection->GetConnectionRole() == ConnectionRole::Acceptor)
        {   
            if (m_networkInterface->GetConnectionSet().GetActiveConnectionCount() == 0)
            {
                AZLOG_INFO("Server exiting due to zero active connections (sv_terminateOnPlayerExit=true)");
                Terminate(DisconnectReason::TerminatedByServer);
                AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::ExitMainLoop);
            }
        }
    }

    MultiplayerAgentType MultiplayerSystemComponent::GetAgentType() const
    {
        return m_agentType;
    }

    void MultiplayerSystemComponent::InitializeMultiplayer(MultiplayerAgentType multiplayerType)
    {
        bool sessionStarted = false;

        if (bg_capturePhysicsTickMetric)
        {
            if (auto* physXSystem = PhysX::GetPhysXSystem())
            {
                m_preSimulateHandler.Disconnect();
                physXSystem->RegisterPreSimulateEvent(m_preSimulateHandler);
                m_postSimulateHandler.Disconnect();
                physXSystem->RegisterPostSimulateEvent(m_postSimulateHandler);
            }
        }

        m_lastReplicatedHostFrameId = HostFrameId{0};

        if (m_agentType == multiplayerType)
        {
            return;
        }

        m_playersWaitingToBeSpawned.clear();

        if (m_agentType != MultiplayerAgentType::Uninitialized && multiplayerType != MultiplayerAgentType::Uninitialized)
        {
            AZLOG_WARN("Attemping to InitializeMultiplayer from one initialized type to another. Your session may not have been properly torn down. Please call the 'disconnect' console command to terminated the current multiplayer simulation before switching to a new multiplayer role.");
        }

        if (m_agentType == MultiplayerAgentType::Uninitialized)
        {
            m_spawnNetboundEntities = false;
            if (multiplayerType == MultiplayerAgentType::ClientServer || multiplayerType == MultiplayerAgentType::DedicatedServer)
            {
                sessionStarted = true;
                m_spawnNetboundEntities = true;
                if (!m_networkEntityManager.IsInitialized())
                {
                    const AZ::CVarFixedString serverAddr = cl_serveraddr;
                    const uint16_t serverPort = cl_serverport;
                    const AzNetworking::ProtocolType serverProtocol = sv_protocol;
                    const AzNetworking::IpAddress hostId = AzNetworking::IpAddress(serverAddr.c_str(), serverPort, serverProtocol);
                    // Set up a full ownership domain if we didn't construct a domain during the initialize event
                    m_networkEntityManager.Initialize(hostId, AZStd::make_unique<FullOwnershipEntityDomain>());
                }
            }
            else if (multiplayerType == MultiplayerAgentType::Client)
            {
                m_networkEntityManager.Initialize(AzNetworking::IpAddress(), AZStd::make_unique<NullEntityDomain>());
            }
        }
        m_agentType = multiplayerType;

        // Spawn the default player for this host since the host is also a player (not a dedicated server)
        if (m_agentType == MultiplayerAgentType::ClientServer)
        {
            MultiplayerAgentDatum datum;
            datum.m_agentType = MultiplayerAgentType::ClientServer;
            datum.m_id = InvalidConnectionId; //< no network connection: the client is hosting itself.
            constexpr uint64_t userId = 0; //< user id 0: the client hosting in client-server is always the first player.

            NetworkEntityHandle controlledEntity;
            if (IMultiplayerSpawner* spawner = AZ::Interface<IMultiplayerSpawner>::Get())
            {
                // Route to spawner implementation
                controlledEntity = spawner->OnPlayerJoin(userId, datum);
            }

            // A controlled player entity likely doesn't exist at this time.
            // Unless IMultiplayerSpawner has a way to return a player without being inside a level (for example using a system component), the client-server's player won't be
            // spawned until the level is loaded.
            if (controlledEntity.Exists())
            {
                EnableAutonomousControl(controlledEntity, InvalidConnectionId);
            }
            else
            {
                // If there wasn't any player entity, wait until a level loads and check again
                m_playersWaitingToBeSpawned.emplace_back(userId, datum, nullptr);
            }
        }
        AZLOG_INFO("Multiplayer operating in %s mode", GetEnumString(m_agentType));

        if (auto* statSystem = AZ::Interface<IMultiplayerStatSystem>::Get())
        {
            statSystem->Register();
        }

        if (sessionStarted)
        {
            m_networkInitEvent.Signal(m_networkInterface);
        }
    }

    void MultiplayerSystemComponent::AddClientMigrationStartEventHandler(ClientMigrationStartEvent::Handler& handler)
    {
        handler.Connect(m_clientMigrationStartEvent);
    }

    void MultiplayerSystemComponent::AddClientMigrationEndEventHandler(ClientMigrationEndEvent::Handler& handler)
    {
        handler.Connect(m_clientMigrationEndEvent);
    }

    void MultiplayerSystemComponent::AddEndpointDisconnectedHandler(EndpointDisconnectedEvent::Handler& handler)
    {
        handler.Connect(m_endpointDisconnectedEvent);
    }

    void MultiplayerSystemComponent::AddNotifyClientMigrationHandler(NotifyClientMigrationEvent::Handler& handler)
    {
        handler.Connect(m_notifyClientMigrationEvent);
    }

    void MultiplayerSystemComponent::AddNotifyEntityMigrationEventHandler(NotifyEntityMigrationEvent::Handler& handler)
    {
        handler.Connect(m_notifyEntityMigrationEvent);
    }

    void MultiplayerSystemComponent::AddConnectionAcquiredHandler(ConnectionAcquiredEvent::Handler& handler)
    {
        handler.Connect(m_connectionAcquiredEvent);
    }

    void MultiplayerSystemComponent::AddNetworkInitHandler(NetworkInitEvent::Handler& handler)
    {
        handler.Connect(m_networkInitEvent);
    }

    void MultiplayerSystemComponent::AddServerAcceptanceReceivedHandler(ServerAcceptanceReceivedEvent::Handler& handler)
    {
        handler.Connect(m_serverAcceptanceReceivedEvent);
    }

    void MultiplayerSystemComponent::AddLevelLoadBlockedHandler(LevelLoadBlockedEvent::Handler& handler)
    {
        handler.Connect(m_levelLoadBlockedEvent);
    }

    void MultiplayerSystemComponent::AddNoServerLevelLoadedHandler(NoServerLevelLoadedEvent::Handler& handler)
    {
        handler.Connect(m_noServerLevelLoadedEvent);
    }

    void MultiplayerSystemComponent::AddVersionMismatchHandler(NoServerLevelLoadedEvent::Handler& handler)
    {
        handler.Connect(m_versionMismatchEvent);
    }

    void MultiplayerSystemComponent::SendNotifyClientMigrationEvent(AzNetworking::ConnectionId connectionId, const HostId& hostId, uint64_t userIdentifier, ClientInputId lastClientInputId, NetEntityId controlledEntityId)
    {
        m_notifyClientMigrationEvent.Signal(connectionId, hostId, userIdentifier, lastClientInputId, controlledEntityId);
    }

    void MultiplayerSystemComponent::SendNotifyEntityMigrationEvent(const ConstNetworkEntityHandle& entityHandle, const HostId& remoteHostId)
    {
        m_notifyEntityMigrationEvent.Signal(entityHandle, remoteHostId);
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

    void MultiplayerSystemComponent::RegisterPlayerIdentifierForRejoin(uint64_t temporaryUserIdentifier, NetEntityId controlledEntityId)
    {
        m_playerRejoinData[temporaryUserIdentifier] = controlledEntityId;
    }

    void MultiplayerSystemComponent::CompleteClientMigration(uint64_t temporaryUserIdentifier, AzNetworking::ConnectionId connectionId, const HostId& publicHostId, ClientInputId migratedClientInputId)
    {
        IConnection* connection = m_networkInterface->GetConnectionSet().GetConnection(connectionId);
        if (connection != nullptr) // Make sure the player has not disconnected since the start of migration
        {
            // Tell the client who to join
            MultiplayerPackets::ClientMigration clientMigration(publicHostId, temporaryUserIdentifier, migratedClientInputId);
            connection->SendReliablePacket(clientMigration);
        }
    }

    void MultiplayerSystemComponent::SetShouldSpawnNetworkEntities(bool value)
    {
        m_spawnNetboundEntities = value;
    }

    bool MultiplayerSystemComponent::GetShouldSpawnNetworkEntities() const
    {
        return m_spawnNetboundEntities;
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
        AZ_PROFILE_SCOPE(MULTIPLAYER, "MultiplayerSystemComponent: TickVisibleNetworkEntities");

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

#if AZ_TRAIT_CLIENT
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
            AZ::Interface<AzFramework::IVisibilitySystem>::Get()->GetDefaultVisibilityScene()->Enumerate(viewFrustum,
                [this, &gatheredEntities](const AzFramework::IVisibilityScene::NodeData& nodeData)
            {
                gatheredEntities.reserve(gatheredEntities.size() + nodeData.m_entries.size());
                for (AzFramework::VisibilityEntry* visEntry : nodeData.m_entries)
                {
                    if (visEntry->m_typeFlags & AzFramework::VisibilityEntry::TypeFlags::TYPE_Entity)
                    {
                        AZ::Entity* entity = static_cast<AZ::Entity*>(visEntry->m_userData);
                        NetBindComponent* netBindComponent = m_networkEntityManager.GetNetworkEntityTracker()->GetNetBindComponent(entity);
                        if (netBindComponent != nullptr)
                        {
                            AZ_Assert(netBindComponent->GetEntity() != nullptr, "Null entity for this component");
                            gatheredEntities.push_back(netBindComponent);
                        }
                    }
                }
            });

            if (bg_parallelNotifyPreRender)
            {
                AZ::JobCompletion jobCompletion;
                for (NetBindComponent* netBindComponent : gatheredEntities)
                {
                    AZ::Job* job = AZ::CreateJobFunction([netBindComponent = netBindComponent, deltaTime]()
                        {
                            AZ_PROFILE_SCOPE(AzCore, "OnPreRenderJob");
                            netBindComponent->NotifyPreRender(deltaTime);
                        }, true, nullptr);

                    job->SetDependent(&jobCompletion);
                    job->Start();
                }

                jobCompletion.StartAndWaitForCompletion();
            }
            else
            {
                for (NetBindComponent* netBindComponent : gatheredEntities)
                {
                    netBindComponent->NotifyPreRender(deltaTime);
                }
            }
        }
        else
#endif // on servers update all net entities
        {
            // If there's no camera, fall back to updating all net entities
            for (auto& iter : *(m_networkEntityManager.GetNetworkEntityTracker()))
            {
                AZ::Entity* entity = iter.second;
                NetBindComponent* netBindComponent = m_networkEntityManager.GetNetworkEntityTracker()->GetNetBindComponent(entity);
                if (netBindComponent != nullptr)
                {
                    netBindComponent->NotifyPreRender(deltaTime);
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

    void MultiplayerSystemComponent::OnAutonomousEntityReplicatorCreated()
    {
        m_autonomousEntityReplicatorCreatedHandler.Disconnect();
        m_clientMigrationEndEvent.Signal();
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

    void MultiplayerSystemComponent::EnableAutonomousControl(NetworkEntityHandle entityHandle, AzNetworking::ConnectionId ownerConnectionId)
    {
        if (!entityHandle.Exists())
        {
            AZLOG_WARN("Attempting to enable autonomous control for an invalid multiplayer entity");
            return;
        }

        entityHandle.GetNetBindComponent()->SetOwningConnectionId(ownerConnectionId);

        // An invalid connection id means this player is controlled by us (the host); not controlled by some connected client.
        if (ownerConnectionId == InvalidConnectionId)
        {
            entityHandle.GetNetBindComponent()->EnablePlayerHostAutonomy(true);
        }

        if (auto* hierarchyComponent = entityHandle.FindComponent<NetworkHierarchyRootComponent>())
        {
            for (AZ::Entity* subEntity : hierarchyComponent->GetHierarchicalEntities())
            {
                NetworkEntityHandle subEntityHandle = NetworkEntityHandle(subEntity);
                NetBindComponent* subEntityNetBindComponent = subEntityHandle.GetNetBindComponent();

                if (subEntityNetBindComponent != nullptr)
                {
                    subEntityNetBindComponent->SetOwningConnectionId(ownerConnectionId);

                    // An invalid connection id means this player is controlled by us (the host); not controlled by some connected client.
                    if (ownerConnectionId == InvalidConnectionId)
                    {
                        subEntityNetBindComponent->EnablePlayerHostAutonomy(true);
                    }
                }
            }
        }
    }
    void MultiplayerSystemComponent::OnRootSpawnableAssigned(
        [[maybe_unused]] AZ::Data::Asset<AzFramework::Spawnable> rootSpawnable, [[maybe_unused]] uint32_t generation)
    {
        m_levelEntitiesActivated = false;
    }

    void MultiplayerSystemComponent::OnRootSpawnableReady(
        [[maybe_unused]] AZ::Data::Asset<AzFramework::Spawnable> rootSpawnable, [[maybe_unused]] uint32_t generation)
    {
        m_levelEntitiesActivated = true;

        // Ignore level loads if not in multiplayer mode
        if (m_agentType == MultiplayerAgentType::Uninitialized)
        {
            return;
        }

        // Spawn players waiting to be spawned. This can happen when a player connects before a level is loaded, so there isn't any player spawner components registered
        IMultiplayerSpawner* spawner = AZ::Interface<IMultiplayerSpawner>::Get();
        if (!spawner)
        {
            AZLOG_ERROR("Attempting to spawn players on level load failed. No IMultiplayerSpawner found. Ensure one is registered.");
            return;
        }

        for (const auto& playerWaitingToBeSpawned : m_playersWaitingToBeSpawned)
        {
            NetworkEntityHandle controlledEntity = spawner->OnPlayerJoin(playerWaitingToBeSpawned.userId, playerWaitingToBeSpawned.agent);
            if (controlledEntity.Exists())
            {
                EnableAutonomousControl(controlledEntity, playerWaitingToBeSpawned.agent.m_id);
            }
            else
            {
                AZLOG_WARN("Attempting to spawn network player on level load failed. IMultiplayerSpawner did not return a controlled entity.");
                return;
            }

            if ((GetAgentType() == MultiplayerAgentType::ClientServer || GetAgentType() == MultiplayerAgentType::DedicatedServer)
                && playerWaitingToBeSpawned.agent.m_agentType == MultiplayerAgentType::Client)
            {
                StartServerToClientReplication(playerWaitingToBeSpawned.userId, controlledEntity, playerWaitingToBeSpawned.connection);
            }
        }

        m_playersWaitingToBeSpawned.clear();
    }

    void MultiplayerSystemComponent::OnRootSpawnableReleased([[maybe_unused]] uint32_t generation)
    {
        m_levelEntitiesActivated = false;
    }

    bool MultiplayerSystemComponent::ShouldBlockLevelLoading(const char* levelName)
    {
        bool blockLevelLoad = false;
        switch (m_agentType)
        {
        case MultiplayerAgentType::Uninitialized:
        {
            // replace .spawnable with .network.spawnable
            AZStd::string networkSpawnablePath(levelName);
            networkSpawnablePath.erase(networkSpawnablePath.size() - strlen(AzFramework::Spawnable::DotFileExtension));
            networkSpawnablePath += NetworkSpawnableFileExtension;

            AZ::Data::AssetId networkSpawnableAssetId;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                networkSpawnableAssetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, networkSpawnablePath.c_str(), azrtti_typeid<AzFramework::Spawnable>(), false);

            if (networkSpawnableAssetId.IsValid())
            {
                AZLOG_WARN("MultiplayerSystemComponent blocked loading a network level. Your multiplayer agent is uninitialized; did you forget to host before loading a network level?")
                blockLevelLoad = true;
            }
            break;
        }
        case MultiplayerAgentType::Client:
            if (m_blockClientLoadLevel)
            {
                AZLOG_WARN("MultiplayerSystemComponent blocked this client from loading a new level. Clients should only attempt to load level when instructed by their server. Disconnect from server before calling LoadLevel.")
                blockLevelLoad = true;
            }
            break;
        case MultiplayerAgentType::ClientServer:
            if (m_playersWaitingToBeSpawned.empty())
            {
                AZLOG_WARN("MultiplayerSystemComponent blocked this host from loading a new level because you already have a player. Loading a new level could destroy the existing network player entity. Disconnect from the multiplayer simulation before changing levels.")
                blockLevelLoad = true;
            }
            break;
        case MultiplayerAgentType::DedicatedServer:
            if (m_networkInterface->GetConnectionSet().GetConnectionCount() > 0)
            {
                AZLOG_WARN("MultiplayerSystemComponent blocked this host from loading a new level because clients are connected. Loading a new level would destroy the existing clients' network player entity.")
                blockLevelLoad = true;
            }
            break;
        default:
            AZLOG_WARN("MultiplayerSystemComponent::ShouldBlockLevelLoading called with unsupported agent type. Please update code to support agent type: %s.", GetEnumString(m_agentType));
        }

        if (blockLevelLoad)
        {
            m_levelLoadBlockedEvent.Signal();
        }

        return blockLevelLoad;
    }

    void MultiplayerSystemComponent::StartServerToClientReplication(uint64_t userId, NetworkEntityHandle controlledEntity, IConnection* connection)
    {
        if (auto connectionData = reinterpret_cast<ServerToClientConnectionData*>(connection->GetUserData()))
        {
            AZStd::unique_ptr<IReplicationWindow> window = AZStd::make_unique<ServerToClientReplicationWindow>(controlledEntity, connection);
            connectionData->GetReplicationManager().SetReplicationWindow(AZStd::move(window));
            connectionData->SetControlledEntity(controlledEntity);

            // If this is a migrate or rejoin, immediately ready the connection for updates
            if (userId != 0)
            {
                connectionData->SetCanSendUpdates(true);
            }
        }
    }

    void MultiplayerSystemComponent::MetricsEvent()
    {
        const auto& networkInterfaces = AZ::Interface<AzNetworking::INetworking>::Get()->GetNetworkInterfaces();
        for (const auto& networkInterface : networkInterfaces)
        {
            if (networkInterface.second->GetType() != bg_captureTransportType)
            {
                continue;
            }
            
            if (networkInterface.second->GetTrustZone() != TrustZone::ExternalClientToServer)
            {
                continue;
            }

            const NetworkInterfaceMetrics& metrics = networkInterface.second->GetMetrics();
            SET_PERFORMANCE_STAT(MultiplayerStat_TotalTimeSpentUpdatingMs, metrics.m_updateTimeMs);            
            SET_PERFORMANCE_STAT(MultiplayerStat_TotalSendTimeMs, metrics.m_sendTimeMs);
            SET_PERFORMANCE_STAT(MultiplayerStat_TotalSentPackets, metrics.m_sendPackets);
            SET_PERFORMANCE_STAT(MultiplayerStat_TotalSentBytesAfterCompression, metrics.m_sendBytes);
            SET_PERFORMANCE_STAT(MultiplayerStat_TotalSentBytesBeforeCompression, metrics.m_sendBytesUncompressed);
            SET_PERFORMANCE_STAT(MultiplayerStat_TotalResentPacketsDueToPacketLoss, metrics.m_resentPackets);
            SET_PERFORMANCE_STAT(MultiplayerStat_TotalReceiveTimeInMs, metrics.m_recvTimeMs);
            SET_PERFORMANCE_STAT(MultiplayerStat_TotalReceivedPackets, metrics.m_recvPackets);
            SET_PERFORMANCE_STAT(MultiplayerStat_TotalReceivedBytesAfterCompression, metrics.m_recvBytes);
            SET_PERFORMANCE_STAT(MultiplayerStat_TotalReceivedBytesBeforeCompression, metrics.m_recvBytesUncompressed);
            SET_PERFORMANCE_STAT(MultiplayerStat_TotalPacketsDiscardedDueToLoad, metrics.m_discardedPackets);

            break; // Assuming there is only one network interface for communicating with clients
        }
    }

    void MultiplayerSystemComponent::OnPhysicsPreSimulate([[maybe_unused]] float dt)
    {
        m_startPhysicsTickTime = AZStd::chrono::steady_clock::now();
    }

    void MultiplayerSystemComponent::OnPhysicsPostSimulate([[maybe_unused]] float dt)
    {
        const auto duration = AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(
            AZStd::chrono::steady_clock::now() - m_startPhysicsTickTime);
        SET_PERFORMANCE_STAT(MultiplayerStat_PhysicsFrameTimeUs, AZ::TimeUs{ duration.count() });
    }

    void MultiplayerSystemComponent::HostConsoleCommand([[maybe_unused]] const AZ::ConsoleCommandContainer& arguments)
    {
        StartHosting(sv_port, sv_isDedicated);
    }

    void sv_launch_local_client([[maybe_unused]] const AZ::ConsoleCommandContainer& arguments)
    {
        // Try finding the game launcher from the executable folder where this server was launched from.
        AZ::IO::FixedMaxPath gameLauncherPath = AZ::Utils::GetExecutableDirectory();
        gameLauncherPath /= AZStd::string_view(AZ::Utils::GetProjectName() + ".GameLauncher" + AZ_TRAIT_OS_EXECUTABLE_EXTENSION);
        if (!AZ::IO::SystemFile::Exists(gameLauncherPath.c_str()))
        {
            AZLOG_ERROR("Could not find GameLauncher executable (%s)", gameLauncherPath.c_str());
            return;
        }

        const auto multiplayerInterface = AZ::Interface<IMultiplayer>::Get();
        if (!multiplayerInterface)
        {
            AZLOG_ERROR("Sv_launch_local_client failed. MultiplayerSystemComponent hasn't been constructed yet.");
            return;
        }

        // Only allow hosts to launch a client, otherwise there's nothing for the client to connect to.
        if (multiplayerInterface->GetAgentType() != MultiplayerAgentType::DedicatedServer &&
            multiplayerInterface->GetAgentType() != MultiplayerAgentType::ClientServer)
        {
            AZLOG_ERROR("Cannot sv_launch_local_client. This program isn't hosting, please call 'host' command.");
            return;
        }
        
        AzFramework::ProcessLauncher::ProcessLaunchInfo processLaunchInfo;
        processLaunchInfo.m_commandlineParameters = AZStd::string::format("%s +connect", gameLauncherPath.c_str());
        processLaunchInfo.m_processPriority = AzFramework::ProcessPriority::PROCESSPRIORITY_NORMAL;
        
        // Launch GameLauncher and connect to this server
        const bool launchSuccess = AzFramework::ProcessLauncher::LaunchUnwatchedProcess(processLaunchInfo);
        if (!launchSuccess)
        {
            AZLOG_ERROR("Failed to launch the local client process.");
            return;
        }
    }
    AZ_CONSOLEFREEFUNC(sv_launch_local_client, AZ::ConsoleFunctorFlags::DontReplicate, "Launches a local client and connects to this host server (only works if currently hosting)");


    void MultiplayerSystemComponent::ConnectConsoleCommand(const AZ::ConsoleCommandContainer& arguments)
    {
        // Handle possible connect arguments:
        // 1. connect
        // 2. connect <ip_address>
        // 3. connect <ip_address>:<port>
        // 4. connect <ip_address>:<port>:<connection_ticket>

        if (arguments.empty() || arguments.front().empty())
        {
            // 1. connect
            // no arguments, use default cvar for ip-address and port
            AZ::CVarFixedString remoteAddress = cl_serveraddr;
            Connect(remoteAddress.c_str(), cl_serverport);
            return;
        }

        AZ::CVarFixedString firstArgument{ arguments.front() };
        const AZStd::size_t portSeparator = firstArgument.find_first_of(':');
        if (portSeparator == AZStd::string::npos)
        {
            // 2. connect <ip_address>, use default cvar for port
            Connect(firstArgument.c_str(), cl_serverport);
            return;
        }

        char* mutableAddress = firstArgument.data();
        mutableAddress[portSeparator] = '\0';
        const char* portStr = &(mutableAddress[portSeparator + 1]);

        const AZStd::size_t ticketSeparator = firstArgument.find_first_of(':', portSeparator + 1);
        if (ticketSeparator == AZStd::string::npos)
        {
            // 3. connect <ip_address>:<port>
            uint16_t portNumber = aznumeric_cast<uint16_t>(atol(portStr)); 
            Connect(mutableAddress, portNumber);
            return;
        }

        // Remaining case:
        // 4. connect <ip_address>:<port>:<connection_ticket>
        mutableAddress[ticketSeparator] = '\0';

        const AZStd::string connectionTicket = &(mutableAddress[ticketSeparator + 1]);
        const uint16_t portNumber = aznumeric_cast<uint16_t>(atol(portStr));

        Connect(mutableAddress, portNumber, connectionTicket);
    }

    void disconnect([[maybe_unused]] const AZ::ConsoleCommandContainer& arguments)
    {
        AZ::Interface<IMultiplayer>::Get()->Terminate(DisconnectReason::TerminatedByUser);
    }
    AZ_CONSOLEFREEFUNC(disconnect, AZ::ConsoleFunctorFlags::DontReplicate, "Disconnects any open multiplayer connections");
} // namespace Multiplayer
