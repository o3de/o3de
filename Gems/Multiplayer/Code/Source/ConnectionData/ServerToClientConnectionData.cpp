/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/ConnectionData/ServerToClientConnectionData.h>
#include <Source/AutoGen/Multiplayer.AutoPackets.h>
#include <Multiplayer/Components/LocalPredictionPlayerInputComponent.h>
#include <Multiplayer/IMultiplayer.h>
#include <AzNetworking/Utilities/EncryptionCommon.h>

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
        AzNetworking::IConnectionListener& connectionListener
    )
        : m_connection(connection)
        , m_controlledEntityRemovedHandler([this](const ConstNetworkEntityHandle&) { OnControlledEntityRemove(); })
        , m_controlledEntityMigrationHandler([this](const ConstNetworkEntityHandle& entityHandle, const HostId& remoteHostId)
            {
                OnControlledEntityMigration(entityHandle, remoteHostId);
            })
        , m_entityReplicationManager(*connection, connectionListener, EntityReplicationManager::Mode::LocalServerToRemoteClient)
    {
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

    void ServerToClientConnectionData::SetControlledEntity(NetworkEntityHandle primaryPlayerEntity)
    {
        m_controlledEntityRemovedHandler.Disconnect();
        m_controlledEntityMigrationHandler.Disconnect();

        m_controlledEntity = primaryPlayerEntity;
        NetBindComponent* netBindComponent = m_controlledEntity.GetNetBindComponent();
        if (netBindComponent != nullptr)
        {
            netBindComponent->AddEntityStopEventHandler(m_controlledEntityRemovedHandler);
            netBindComponent->AddEntityServerMigrationEventHandler(m_controlledEntityMigrationHandler);
        }
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

    void ServerToClientConnectionData::Update()
    {
        m_entityReplicationManager.ActivatePendingEntities();

        if (CanSendUpdates())
        {
            NetBindComponent* netBindComponent = m_controlledEntity.GetNetBindComponent();
            // potentially false if we just migrated the player, if that is the case, don't send any more updates
            if (netBindComponent != nullptr && (netBindComponent->GetNetEntityRole() == NetEntityRole::Authority))
            {
                m_entityReplicationManager.SendUpdates();
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
        const HostId& remoteHostId
    )
    {
        ClientInputId migratedClientInputId = ClientInputId{ 0 };
        if (m_controlledEntity != nullptr)
        {
            auto controller = m_controlledEntity.FindController<LocalPredictionPlayerInputComponentController>();
            if (controller != nullptr)
            {
                migratedClientInputId = controller->GetLastInputId();
            }
        }

        // Generate crypto-rand user identifier, send to both server and client so they can negotiate the autonomous entity to assume predictive control over after migration
        const uint64_t temporaryUserIdentifier = AzNetworking::CryptoRand64();

        // Tell the new host that a client is about to (re)join
        GetMultiplayer()->SendNotifyClientMigrationEvent(GetConnection()->GetConnectionId(), remoteHostId, temporaryUserIdentifier, migratedClientInputId, m_controlledEntity.GetNetEntityId());
        // We need to send a MultiplayerPackets::ClientMigration packet to complete this process
        // This happens inside MultiplayerSystemComponent, once we're certain the remote host has appropriately prepared

        m_controlledEntity = NetworkEntityHandle();
        m_canSendUpdates = false;
    }

    void ServerToClientConnectionData::OnGameplayStarted()
    {
        m_entityReplicationManager.SetMaxRemoteEntitiesPendingCreationCount(sv_ClientMaxRemoteEntitiesPendingCreationCountPostInit);
    }
}
