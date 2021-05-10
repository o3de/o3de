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

#include <Source/ConnectionData/ClientToServerConnectionData.h>

namespace Multiplayer
{
    // This can be used to help mitigate client side performance when large numbers of entities are created off the network
    AZ_CVAR(uint32_t, cl_ClientMaxRemoteEntitiesPendingCreationCount, AZStd::numeric_limits<uint32_t>::max(), nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Maximum number of entities that we have sent to the client, but have not had a confirmation back from the client");
    AZ_CVAR(AZ::TimeMs, cl_ClientEntityReplicatorPendingRemovalTimeMs, AZ::TimeMs{ 10000 }, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "How long should wait prior to removing an entity for the client through a change in the replication window, entity deletes are still immediate");

    ClientToServerConnectionData::ClientToServerConnectionData
    (
        AzNetworking::IConnection* connection,
        AzNetworking::IConnectionListener& connectionListener
    )
        : m_connection(connection)
        , m_entityReplicationManager(*connection, connectionListener, EntityReplicationManager::Mode::LocalClientToRemoteServer)
    {
        m_entityReplicationManager.SetMaxRemoteEntitiesPendingCreationCount(cl_ClientMaxRemoteEntitiesPendingCreationCount);
        m_entityReplicationManager.SetEntityPendingRemovalMs(cl_ClientEntityReplicatorPendingRemovalTimeMs);
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

    void ClientToServerConnectionData::Update(AZ::TimeMs hostTimeMs)
    {
        m_entityReplicationManager.ActivatePendingEntities();
        m_entityReplicationManager.SendUpdates(hostTimeMs);
    }
}
