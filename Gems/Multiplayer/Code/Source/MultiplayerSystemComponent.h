/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Multiplayer/IMultiplayer.h>
#include <Editor/MultiplayerEditorConnection.h>
#include <NetworkTime/NetworkTime.h>
#include <NetworkEntity/NetworkEntityManager.h>
#include <Source/AutoGen/Multiplayer.AutoPacketDispatcher.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ILogger.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/Threading/ThreadSafeDeque.h>
#include <AzCore/std/string/string.h>
#include <AzNetworking/ConnectionLayer/IConnectionListener.h>

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
        , public AzNetworking::IConnectionListener
        , public IMultiplayer
    {
    public:
        AZ_COMPONENT(MultiplayerSystemComponent, "{7C99C4C1-1103-43F9-AD62-8B91CF7C1981}");

        static void Reflect(AZ::ReflectContext* context);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        MultiplayerSystemComponent();
        ~MultiplayerSystemComponent() override = default;

        //! AZ::Component overrides.
        //! @{
        void Activate() override;
        void Deactivate() override;
        //! @}

        //! AZ::TickBus::Handler overrides.
        //! @{
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        int GetTickOrder() override;
        //! @}

        bool HandleRequest(AzNetworking::IConnection* connection, const AzNetworking::IPacketHeader& packetHeader, MultiplayerPackets::Connect& packet);
        bool HandleRequest(AzNetworking::IConnection* connection, const AzNetworking::IPacketHeader& packetHeader, MultiplayerPackets::Accept& packet);
        bool HandleRequest(AzNetworking::IConnection* connection, const AzNetworking::IPacketHeader& packetHeader, MultiplayerPackets::ReadyForEntityUpdates& packet);
        bool HandleRequest(AzNetworking::IConnection* connection, const AzNetworking::IPacketHeader& packetHeader, MultiplayerPackets::SyncConsole& packet);
        bool HandleRequest(AzNetworking::IConnection* connection, const AzNetworking::IPacketHeader& packetHeader, MultiplayerPackets::ConsoleCommand& packet);
        bool HandleRequest(AzNetworking::IConnection* connection, const AzNetworking::IPacketHeader& packetHeader, MultiplayerPackets::EntityUpdates& packet);
        bool HandleRequest(AzNetworking::IConnection* connection, const AzNetworking::IPacketHeader& packetHeader, MultiplayerPackets::EntityRpcs& packet);
        bool HandleRequest(AzNetworking::IConnection* connection, const AzNetworking::IPacketHeader& packetHeader, MultiplayerPackets::ClientMigration& packet);
    
        //! IConnectionListener interface
        //! @{
        AzNetworking::ConnectResult ValidateConnect(const AzNetworking::IpAddress& remoteAddress, const AzNetworking::IPacketHeader& packetHeader, AzNetworking::ISerializer& serializer) override;
        void OnConnect(AzNetworking::IConnection* connection) override;
        bool OnPacketReceived(AzNetworking::IConnection* connection, const AzNetworking::IPacketHeader& packetHeader, AzNetworking::ISerializer& serializer) override;
        void OnPacketLost(AzNetworking::IConnection* connection, AzNetworking::PacketId packetId) override;
        void OnDisconnect(AzNetworking::IConnection* connection, AzNetworking::DisconnectReason reason, AzNetworking::TerminationEndpoint endpoint) override;
        //! @}

        //! IMultiplayer interface
        //! @{
        MultiplayerAgentType GetAgentType() const override;
        void InitializeMultiplayer(MultiplayerAgentType state) override;
        void AddConnectionAcquiredHandler(ConnectionAcquiredEvent::Handler& handler) override;
        void AddSessionInitHandler(SessionInitEvent::Handler& handler) override;
        void AddSessionShutdownHandler(SessionShutdownEvent::Handler& handler) override;
        void SendReadyForEntityUpdates(bool readyForEntityUpdates) override;
        AZ::TimeMs GetCurrentHostTimeMs() const override;
        INetworkTime* GetNetworkTime() override;
        INetworkEntityManager* GetNetworkEntityManager() override;
        //! @}

        //! Console commands.
        //! @{
        void DumpStats(const AZ::ConsoleCommandContainer& arguments);
        //! @}

    private:

        void TickVisibleNetworkEntities(float deltaTime, float serverRateSeconds);
        void OnConsoleCommandInvoked(AZStd::string_view command, const AZ::ConsoleCommandContainer& args, AZ::ConsoleFunctorFlags flags, AZ::ConsoleInvokedFrom invokedFrom);
        void ExecuteConsoleCommandList(AzNetworking::IConnection* connection, const AZStd::fixed_vector<Multiplayer::LongNetworkString, 32>& commands);
        NetworkEntityHandle SpawnDefaultPlayerPrefab();
        
        AZ_CONSOLEFUNC(MultiplayerSystemComponent, DumpStats, AZ::ConsoleFunctorFlags::Null, "Dumps stats for the current multiplayer session");

        AzNetworking::INetworkInterface* m_networkInterface = nullptr;
        AzNetworking::INetworkInterface* m_networkEditorInterface = nullptr;
        AZ::ConsoleCommandInvokedEvent::Handler m_consoleCommandHandler;
        AZ::ThreadSafeDeque<AZStd::string> m_cvarCommands;

        NetworkEntityManager m_networkEntityManager;
        NetworkTime m_networkTime;
        MultiplayerAgentType m_agentType = MultiplayerAgentType::Uninitialized;

        SessionInitEvent m_initEvent;
        SessionShutdownEvent m_shutdownEvent;
        ConnectionAcquiredEvent m_connAcquiredEvent;

        AZ::TimeMs m_lastReplicatedHostTimeMs = AZ::TimeMs{ 0 };
        HostFrameId m_lastReplicatedHostFrameId = InvalidHostFrameId;

        double m_serverSendAccumulator = 0.0;
        float m_renderBlendFactor = 0.0f;

#if !defined(AZ_RELEASE_BUILD)
        MultiplayerEditorConnection m_editorConnectionListener;
#endif
    };
}
