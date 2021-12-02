/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/ConnectionData/ClientToServerConnectionData.h>

namespace Multiplayer
{
    // This can be used to help mitigate client side performance when large numbers of entities are created off the network
    AZ_CVAR(uint32_t, cl_ClientMaxRemoteEntitiesPendingCreationCount, AZStd::numeric_limits<uint32_t>::max(), nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Maximum number of entities that we have sent to the client, but have not had a confirmation back from the client");
    AZ_CVAR(AZ::TimeMs, cl_ClientEntityReplicatorPendingRemovalTimeMs, AZ::TimeMs{ 10000 }, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "How long should wait prior to removing an entity for the client through a change in the replication window, entity deletes are still immediate");
    AZ_CVAR(AZ::TimeMs, cl_DefaultNetworkEntityActivationTimeSliceMs, AZ::TimeMs{ 0 }, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Max Ms to use to activate entities coming from the network, 0 means instantiate everything");

    ClientToServerConnectionData::ClientToServerConnectionData
    (
        AzNetworking::IConnection* connection,
        AzNetworking::IConnectionListener& connectionListener,
        const AZStd::string& providerTicket
    )
        : m_connection(connection)
        , m_entityReplicationManager(*connection, connectionListener, EntityReplicationManager::Mode::LocalClientToRemoteServer)
        , m_providerTicket(providerTicket)
    {
        m_entityReplicationManager.SetMaxRemoteEntitiesPendingCreationCount(cl_ClientMaxRemoteEntitiesPendingCreationCount);
        m_entityReplicationManager.SetEntityPendingRemovalMs(cl_ClientEntityReplicatorPendingRemovalTimeMs);
        m_entityReplicationManager.SetEntityActivationTimeSliceMs(cl_DefaultNetworkEntityActivationTimeSliceMs);
    }

    ClientToServerConnectionData::~ClientToServerConnectionData()
    {
        m_entityReplicationManager.Clear(false);
    }

    ConnectionDataType ClientToServerConnectionData::GetConnectionDataType() const
    {
        return ConnectionDataType::ClientToServer;
    }

    AzNetworking::IConnection* ClientToServerConnectionData::GetConnection() const
    {
        return m_connection;
    }

    EntityReplicationManager& ClientToServerConnectionData::GetReplicationManager()
    {
        return m_entityReplicationManager;
    }

    void ClientToServerConnectionData::Update()
    {
        m_entityReplicationManager.ActivatePendingEntities();
        m_entityReplicationManager.SendUpdates();
    }
}
