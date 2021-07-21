/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/ConnectionData/ServerToClientConnectionData.h>
#include <Multiplayer/IMultiplayer.h>

namespace Multiplayer
{
    // This can be used to help mitigate client side performance when large numbers of entities are created off the network
    AZ_CVAR(uint32_t, sv_ClientMaxRemoteEntitiesPendingCreationCount, AZStd::numeric_limits<uint32_t>::max(), nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Maximum number of entities that we have sent to the client, but have not had a confirmation back from the client");
    AZ_CVAR(uint32_t, sv_ClientMaxRemoteEntitiesPendingCreationCountPostInit, AZStd::numeric_limits<uint32_t>::max(), nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Maximum number of entities that we will send to clients after gameplay has begun");
    AZ_CVAR(AZ::TimeMs, sv_ClientEntityReplicatorPendingRemovalTimeMs, AZ::TimeMs{ 10000 }, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "How long should wait prior to removing an entity for the client through a change in the replication window, entity deletes are still immediate");
    AZ_CVAR(bool, sv_removeDefaultPlayerSpawnableOnDisconnect, true, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Whether to remove player's default spawnable when a player disconnects");

    ServerToClientConnectionData::ServerToClientConnectionData
    (
        AzNetworking::IConnection* connection,
        AzNetworking::IConnectionListener& connectionListener,
        NetworkEntityHandle controlledEntity
    )
        : m_connection(connection)
        , m_controlledEntityRemovedHandler([this](const ConstNetworkEntityHandle&) { OnControlledEntityRemove(); })
        , m_controlledEntityMigrationHandler([this](const ConstNetworkEntityHandle& entityHandle, HostId remoteHostId, AzNetworking::ConnectionId connectionId) { OnControlledEntityMigration(entityHandle, remoteHostId, connectionId); })
        , m_controlledEntity(controlledEntity)
        , m_entityReplicationManager(*connection, connectionListener, EntityReplicationManager::Mode::LocalServerToRemoteClient)
    {
        NetBindComponent* netBindComponent = m_controlledEntity.GetNetBindComponent();
        if (netBindComponent != nullptr)
        {
            netBindComponent->AddEntityStopEventHandler(m_controlledEntityRemovedHandler);
            netBindComponent->AddEntityServerMigrationEventHandler(m_controlledEntityMigrationHandler);
        }

        m_entityReplicationManager.SetMaxRemoteEntitiesPendingCreationCount(sv_ClientMaxRemoteEntitiesPendingCreationCount);
        m_entityReplicationManager.SetEntityPendingRemovalMs(sv_ClientEntityReplicatorPendingRemovalTimeMs);
    }

    ServerToClientConnectionData::~ServerToClientConnectionData()
    {
        if (sv_removeDefaultPlayerSpawnableOnDisconnect)
        {
            AZ::Interface<IMultiplayer>::Get()->GetNetworkEntityManager()->MarkForRemoval(m_controlledEntity);
        }

        m_entityReplicationManager.Clear(false);
        m_controlledEntityRemovedHandler.Disconnect();
    }

    ConnectionDataType ServerToClientConnectionData::GetConnectionDataType() const
    {
        return ConnectionDataType::ServerToClient;
    }

    AzNetworking::IConnection* ServerToClientConnectionData::GetConnection() const
    {
        return m_connection;
    }

    EntityReplicationManager& ServerToClientConnectionData::GetReplicationManager()
    {
        return m_entityReplicationManager;
    }

    void ServerToClientConnectionData::Update(AZ::TimeMs hostTimeMs)
    {
        m_entityReplicationManager.ActivatePendingEntities();

        if (CanSendUpdates())
        {
            NetBindComponent* netBindComponent = m_controlledEntity.GetNetBindComponent();
            // potentially false if we just migrated the player, if that is the case, don't send any more updates
            if (netBindComponent != nullptr && (netBindComponent->GetNetEntityRole() == NetEntityRole::Authority))
            {
                m_entityReplicationManager.SendUpdates(hostTimeMs);
            }
        }
    }

    void ServerToClientConnectionData::OnControlledEntityRemove()
    {
        m_connection->Disconnect(AzNetworking::DisconnectReason::TerminatedByServer, AzNetworking::TerminationEndpoint::Local);
        m_entityReplicationManager.Clear(false);
        m_controlledEntity.Reset();
    }

    void ServerToClientConnectionData::OnControlledEntityMigration
    (
        [[maybe_unused]] const ConstNetworkEntityHandle& entityHandle,
        [[maybe_unused]] HostId remoteHostId,
        [[maybe_unused]] AzNetworking::ConnectionId connectionId
    )
    {
        //Multiplayer::ServerAddrInfo serverAddr;
        //if (gNovaGame->GetMultiplayerworkAgent().GetServerToServerNetwork().GetServerAddrInfoFromConnectionId(newConnectionId, serverAddr) == false)
        //{
        //    AZLOG_WARN("MigrateClient::Failed to find servershard address, userID:%d", static_cast<uint32_t>(GetUserId()));
        //    return;
        //}
        //
        //Multiplayer::GameTimePoint migratedClientGameTimePoint;
        //
        //if (m_ControlledEntity != nullptr)
        //{
        //    if (const PlayerNetworkInputComponent::Authority* pComponent = Multiplayer::FindController<PlayerNetworkInputComponent::Authority>(m_ControlledEntity))
        //    {
        //        migratedClientGameTimePoint = pComponent->GetLastInputId().GetServerGameTimePoint();
        //    }
        //}
        //
        // generate crypto-rand user identifier, send to both server and client so they can negotiate the autonomous entity to assume predictive control over after migration
        //const uint64_t randomUserIdentifier = 0;
        //
        //// Tell the server a new client is about to join
        //MultiplayerPackets::NotifyClientMigration notifyClientMigration(randomUserIdentifier);
        //gNovaGame->GetMultiplayerworkAgent().GetServerToServerNetwork().SendReliablePacket(newConnectionId, notifyClientMigration);
        //
        //// Tell the client who to join
        //MultiplayerPackets::ClientMigration clientMigration(randomUserIdentifier, serverAddr, migratedClientGameTimePoint);
        //GetConnection()->SendReliablePacket(clientMigration);
        //
        //m_controlledEntity = nullptr;
        //m_canSendUpdates = false;
    }

    void ServerToClientConnectionData::OnGameplayStarted()
    {
        m_entityReplicationManager.SetMaxRemoteEntitiesPendingCreationCount(sv_ClientMaxRemoteEntitiesPendingCreationCountPostInit);
    }
}
