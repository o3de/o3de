/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzNetworking/ConnectionLayer/IConnection.h>
#include <AzNetworking/DataStructures/ByteBuffer.h>
#include <Multiplayer/NetworkEntity/IFilterEntityManager.h>
#include <Multiplayer/Components/MultiplayerComponentRegistry.h>
#include <Multiplayer/NetworkEntity/INetworkEntityManager.h>
#include <Multiplayer/NetworkTime/INetworkTime.h>
#include <Multiplayer/MultiplayerStats.h>

namespace AzNetworking
{
    class INetworkInterface;
}

namespace Multiplayer
{
    //! Collection of types of Multiplayer Connections
    enum class MultiplayerAgentType
    {
        Uninitialized,   //!< Agent is uninitialized.
        Client,          //!< A Client connected to either a server or host.
        ClientServer,    //!< A Client that also hosts and is the authority of the session
        DedicatedServer  //!< A Dedicated Server which does not locally host any clients
    };

    //! @brief Payload detailing aspects of a Connection other services may be interested in
    struct MultiplayerAgentDatum
    {
        bool m_isInvited;
        MultiplayerAgentType m_agentType;
        AzNetworking::ConnectionId m_id;
        AzNetworking::ByteBuffer<2048> m_userData;
    };

    using ClientMigrationStartEvent = AZ::Event<ClientInputId>;
    using ClientMigrationEndEvent = AZ::Event<>;
    using ClientDisconnectedEvent = AZ::Event<>;
    using NotifyClientMigrationEvent = AZ::Event<AzNetworking::ConnectionId, const HostId&, uint64_t, ClientInputId, NetEntityId>;
    using NotifyEntityMigrationEvent = AZ::Event<const ConstNetworkEntityHandle&, const HostId&>;
    using ConnectionAcquiredEvent = AZ::Event<MultiplayerAgentDatum>;
    using ServerAcceptanceReceivedEvent = AZ::Event<>;
    using SessionInitEvent = AZ::Event<AzNetworking::INetworkInterface*>;
    using SessionShutdownEvent = AZ::Event<AzNetworking::INetworkInterface*>;

    //! @class IMultiplayer
    //! @brief IMultiplayer provides insight into the Multiplayer session and its Agents
    //!
    //! IMultiplayer is an AZ::Interface<T> that provides applications access to
    //! multiplayer session information and events.  IMultiplayer is implemented on the
    //! MultiplayerSystemComponent and is used to define and access information about
    //! the type of session and the role held by the current agent. An Agent is defined
    //! here as an actor in a session. Types of Agents included by default are a Client,
    //! a Client Server and a Dedicated Server.
    //! 
    //! IMultiplayer also provides events to allow developers to receive and respond to
    //! notifications relating to the session. These include Session Init and Shutdown
    //! and on acquisition of a new connection. These events are only fired on Client
    //! Server or Dedicated Server. These events are useful for services that talk to
    //! matchmaking services that may run in an entirely different layer which may need
    //! insight to the gameplay session.
    class IMultiplayer
    {
    public:
        AZ_RTTI(IMultiplayer, "{90A001DD-AD31-46C7-9FBE-1059AFB7F5E9}");

        virtual ~IMultiplayer() = default;

        //! Gets the type of Agent this IMultiplayer impl represents.
        //! @return The type of agents represented
        virtual MultiplayerAgentType GetAgentType() const = 0;

        //! Sets the type of this Multiplayer connection and calls any related callback.
        //! @param state The state of this connection
        virtual void InitializeMultiplayer(MultiplayerAgentType state) = 0;

        //! Starts hosting a server.
        //! @param port The port to listen for connection on
        //! @param isDedicated Whether the server is dedicated or client hosted
        //! @return if the application successfully started hosting
        virtual bool StartHosting(uint16_t port, bool isDedicated = true) = 0;

        //! Connects to the specified IP as a Client.
        //! @param remoteAddress The domain or IP to connect to
        //! @param port The port to connect to
        //! @result if a connection was successfully created
        virtual bool Connect(const AZStd::string& remoteAddress, uint16_t port) = 0;

        // Disconnects all multiplayer connections, stops listening on the server and invokes handlers appropriate to network context.
        //! @param reason The reason for terminating connections
        virtual void Terminate(AzNetworking::DisconnectReason reason) = 0;

        //! Adds a ClientMigrationStartEvent Handler which is invoked at the start of a client migration.
        //! @param handler The ClientMigrationStartEvent Handler to add
        virtual void AddClientMigrationStartEventHandler(ClientMigrationStartEvent::Handler& handler) = 0;

        //! Adds a ClientMigrationEndEvent Handler which is invoked when a client completes migration.
        //! @param handler The ClientMigrationEndEvent Handler to add
        virtual void AddClientMigrationEndEventHandler(ClientMigrationEndEvent::Handler& handler) = 0;

        //! Adds a ClientDisconnectedEvent Handler which is invoked on the client when a disconnection occurs.
        //! @param handler The ClientDisconnectedEvent Handler to add
        virtual void AddClientDisconnectedHandler(ClientDisconnectedEvent::Handler& handler) = 0;

        //! Adds a NotifyClientMigrationEvent Handler which is invoked when a client migrates from one host to another.
        //! @param handler The NotifyClientMigrationEvent Handler to add
        virtual void AddNotifyClientMigrationHandler(NotifyClientMigrationEvent::Handler& handler) = 0;

        //! Adds a NotifyEntityMigrationEvent Handler which is invoked when an entity migrates from one host to another.
        //! @param handler The NotifyEntityMigrationEvent Handler to add
        virtual void AddNotifyEntityMigrationEventHandler(NotifyEntityMigrationEvent::Handler& handler) = 0;

        //! Adds a ConnectionAcquiredEvent Handler which is invoked when a new endpoint connects to the session.
        //! @param handler The ConnectionAcquiredEvent Handler to add
        virtual void AddConnectionAcquiredHandler(ConnectionAcquiredEvent::Handler& handler) = 0;

        //! Adds a ServerAcceptanceReceived Handler which is invoked when the client receives the accept packet from the server.
        //! @param handler The ServerAcceptanceReceived Handler to add
        virtual void AddServerAcceptanceReceivedHandler(ServerAcceptanceReceivedEvent::Handler& handler) = 0;

        //! Adds a SessionInitEvent Handler which is invoked when a new network session starts.
        //! @param handler The SessionInitEvent Handler to add
        virtual void AddSessionInitHandler(SessionInitEvent::Handler& handler) = 0;

        //! Adds a SessionShutdownEvent Handler which is invoked when the current network session ends.
        //! @param handler The SessionShutdownEvent handler to add
        virtual void AddSessionShutdownHandler(SessionShutdownEvent::Handler& handler) = 0;

        //! Signals a NotifyClientMigrationEvent with the provided parameters.
        //! @param connectionId       the connection id of the client that is migrating
        //! @param hostId             the host id of the host the client is migrating to
        //! @param userIdentifier     the user identifier the client will provide the new host to validate identity
        //! @param lastClientInputId  the last processed clientInputId by the current host
        //! @param controlledEntityId the entityId of the clients autonomous entity
        virtual void SendNotifyClientMigrationEvent(AzNetworking::ConnectionId connectionId, const HostId& hostId, uint64_t userIdentifier, ClientInputId lastClientInputId, NetEntityId controlledEntityId) = 0;

        //! Signals a NotifyEntityMigrationEvent with the provided parameters.
        //! @param entityHandle the network entity handle of the entity being migrated
        //! @param remoteHostId the host id of the host the entity is migrating to
        virtual void SendNotifyEntityMigrationEvent(const ConstNetworkEntityHandle& entityHandle, const HostId& remoteHostId) = 0;

        //! Sends a packet telling if entity update messages can be sent.
        //! @param readyForEntityUpdates Ready for entity updates or not
        virtual void SendReadyForEntityUpdates(bool readyForEntityUpdates) = 0;

        //! Returns the current server time in milliseconds.
        //! This can be one of three possible values:
        //!   1. On the host outside of rewind scope, this will return the latest application elapsed time in ms.
        //!   2. On the host within rewind scope, this will return the rewound time in ms.
        //!   3. On the client, this will return the most recently replicated server time in ms.
        //! @return the current server time in milliseconds
        virtual AZ::TimeMs GetCurrentHostTimeMs() const = 0;

        //! Returns the current blend factor for client side interpolation.
        //! This value is only relevant on the client and is used to smooth between host frames
        //! @return the current blend factor
        virtual float GetCurrentBlendFactor() const = 0;

        //! Returns the network time instance bound to this multiplayer instance.
        //! @return pointer to the network time instance bound to this multiplayer instance
        virtual INetworkTime* GetNetworkTime() = 0;

        //! Returns the network entity manager instance bound to this multiplayer instance.
        //! @return pointer to the network entity manager instance bound to this multiplayer instance
        virtual INetworkEntityManager* GetNetworkEntityManager() = 0;

        //! Sets user-defined filtering manager for entities.
        //! This allows selectively choosing which entities to replicate on a per client connection.
        //! See IFilterEntityManager for details.
        //! @param entityFilter non-owning pointer, the caller is responsible for memory management.
        virtual void SetFilterEntityManager(IFilterEntityManager* entityFilter) = 0;

        //! Returns a pointer to the user-defined filtering manager of entities.
        //! @return pointer to the filtered entity manager, or nullptr if not set
        virtual IFilterEntityManager* GetFilterEntityManager() = 0;

        //! Registers a temp userId to allow a host to look up a players controlled entity in the event of a rejoin or migration event.
        //! @param temporaryUserIdentifier the temporary user identifier used to identify a player across hosts
        //! @param controlledEntityId      the controlled entityId of the players autonomous entity
        virtual void RegisterPlayerIdentifierForRejoin(uint64_t temporaryUserIdentifier, NetEntityId controlledEntityId) = 0;

        //! Completes a client migration event by informing the appropriate client to migrate between hosts.
        //! @param temporaryUserIdentifier the temporary user identifier used to identify a player across hosts
        //! @param connectionId            the connection id of the player being migrated
        //! @param publicHostId            the public address of the new host the client should connect to
        //! @param migratedClientInputId   the last clientInputId processed prior to migration
        virtual void CompleteClientMigration(uint64_t temporaryUserIdentifier, AzNetworking::ConnectionId connectionId, const HostId& publicHostId, ClientInputId migratedClientInputId) = 0;

        //! Enables or disables automatic instantiation of netbound entities.
        //! This setting is controlled by the networking layer and should not be touched
        //! If enabled, netbound entities will instantiate as spawnables are loaded into the game world, generally true for the server
        //! If disabled, netbound entities will only stream from a host, always true for a client
        //! @param value boolean value controlling netbound entity instantiation behaviour
        virtual void SetShouldSpawnNetworkEntities(bool value) = 0;

        //! Retrieves the current network entity instantiation behaviour.
        //! @return boolean true if netbound entities should be auto instantiated, false if not
        virtual bool GetShouldSpawnNetworkEntities() const = 0;

        //! Retrieve the stats object bound to this multiplayer instance.
        //! @return the stats object bound to this multiplayer instance
        MultiplayerStats& GetStats() { return m_stats; }

    private:
        MultiplayerStats m_stats;
    };

    // Convenience helpers
    inline IMultiplayer* GetMultiplayer()
    {
        return AZ::Interface<IMultiplayer>::Get();
    }

    inline INetworkEntityManager* GetNetworkEntityManager()
    {
        IMultiplayer* multiplayer = GetMultiplayer();
        return (multiplayer != nullptr) ? multiplayer->GetNetworkEntityManager() : nullptr;
    }

    inline NetworkEntityTracker* GetNetworkEntityTracker()
    {
        INetworkEntityManager* networkEntityManager = GetNetworkEntityManager();
        return (networkEntityManager != nullptr) ? networkEntityManager->GetNetworkEntityTracker() : nullptr;
    }

    inline NetworkEntityAuthorityTracker* GetNetworkEntityAuthorityTracker()
    {
        INetworkEntityManager* networkEntityManager = GetNetworkEntityManager();
        return (networkEntityManager != nullptr) ? networkEntityManager->GetNetworkEntityAuthorityTracker() : nullptr;
    }

    inline MultiplayerComponentRegistry* GetMultiplayerComponentRegistry()
    {
        INetworkEntityManager* networkEntityManager = GetNetworkEntityManager();
        return (networkEntityManager != nullptr) ? networkEntityManager->GetMultiplayerComponentRegistry() : nullptr;
    }

    //! @class ScopedAlterTime
    //! @brief This is a wrapper that temporarily adjusts global program time for backward reconciliation purposes.
    class ScopedAlterTime final
    {
    public:
        inline ScopedAlterTime(HostFrameId frameId, AZ::TimeMs timeMs, float blendFactor, AzNetworking::ConnectionId connectionId)
        {
            INetworkTime* time = GetNetworkTime();
            m_previousHostFrameId = time->GetHostFrameId();
            m_previousHostTimeMs = time->GetHostTimeMs();
            m_previousRewindConnectionId = time->GetRewindingConnectionId();
            m_previousBlendFactor = time->GetHostBlendFactor();
            time->AlterTime(frameId, timeMs, blendFactor, connectionId);
        }
        inline ~ScopedAlterTime()
        {
            INetworkTime* time = GetNetworkTime();
            time->AlterTime(m_previousHostFrameId, m_previousHostTimeMs, m_previousBlendFactor, m_previousRewindConnectionId);
        }
    private:
        HostFrameId m_previousHostFrameId = InvalidHostFrameId;
        AZ::TimeMs m_previousHostTimeMs = AZ::TimeMs{ 0 };
        AzNetworking::ConnectionId m_previousRewindConnectionId = AzNetworking::InvalidConnectionId;
        float m_previousBlendFactor = DefaultBlendFactor;
    };

    inline const char* GetEnumString(MultiplayerAgentType value)
    {
        switch (value)
        {
        case MultiplayerAgentType::Uninitialized:
            return "Uninitialized";
        case MultiplayerAgentType::Client:
            return "Client";
        case MultiplayerAgentType::ClientServer:
            return "ClientServer";
        case MultiplayerAgentType::DedicatedServer:
            return "DedicatedServer";
        }
        return "INVALID";
    }
}
