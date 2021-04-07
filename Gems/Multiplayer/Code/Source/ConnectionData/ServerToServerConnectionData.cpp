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

#include <Source/ConnectionData/ServerToServerConnectionData.h>

namespace Multiplayer
{
    AZ_CVAR(AZ::TimeMs, sv_DefaultNetworkEntityActivationTimeSliceMs, AZ::TimeMs{ 0 }, nullptr, AZ::ConsoleFunctorFlags::Null, "Max Ms to use to activate entities coming from the network, 0 means instantiate everything");
    AZ_CVAR(AZ::TimeMs, sv_ServerToServerReconnectDelayMs, AZ::TimeMs{ 1000 }, nullptr, AZ::ConsoleFunctorFlags::Null, "Number of milliseconds for delaying reconnecting that is based on sv_ServerNonceTimeoutMs");
    AZ_CVAR(uint32_t, sv_ServerMaxRemoteEntitiesPendingCreationCount, 512, nullptr, AZ::ConsoleFunctorFlags::Null, "Maximum number of entities that we have sent to the remote server, but have not had a confirmation back from the remote server");

    ServerToServerConnectionData::ServerToServerConnectionData
    (
        AzNetworking::IConnection* connection,
        AzNetworking::IConnectionListener& connectionListener,
        const AzNetworking::IpAddress& serverAddress
    )
        : m_connection(connection)
        , m_serverAddress(serverAddress)
        , m_entityReplicationManager(*connection, connectionListener, EntityReplicationManager::Mode::LocalServerToRemoteServer)
        , m_connectEvent([this]() { OnConnectTimeout(); }, AZ::Name("Server to server connection timeout event"))
    {
        m_entityReplicationManager.SetRemoteHostId(InvalidHostId);// serverAddrInfo.GetServerAddrInfo().GetShardId());
        m_entityReplicationManager.SetEntityActivationTimeSliceMs(sv_DefaultNetworkEntityActivationTimeSliceMs);
        m_entityReplicationManager.SetMaxRemoteEntitiesPendingCreationCount(sv_ServerMaxRemoteEntitiesPendingCreationCount);

        if (connection->GetConnectionRole() == AzNetworking::ConnectionRole::Connector)
        {
            m_connectEvent.Enqueue(sv_ServerToServerReconnectDelayMs);
        }
    }

    ServerToServerConnectionData::~ServerToServerConnectionData()
    {
        m_entityReplicationManager.Clear(false);
    }

    ConnectionDataType ServerToServerConnectionData::GetConnectionDataType() const
    {
        return ConnectionDataType::ServerToServer;
    }

    AzNetworking::IConnection* ServerToServerConnectionData::GetConnection() const
    {
        return m_connection;
    }

    EntityReplicationManager& ServerToServerConnectionData::GetReplicationManager()
    {
        return m_entityReplicationManager;
    }

    void ServerToServerConnectionData::Update(AZ::TimeMs serverGameTimeMs)
    {
        if (IsReady())
        {
            m_entityReplicationManager.SendUpdates(serverGameTimeMs);
        }
    }

    HostId ServerToServerConnectionData::GetHostId() const
    {
        return InvalidHostId; // GetServerToServerAddrInfo().GetServerAddrInfo().GetShardId();
    }

    void ServerToServerConnectionData::OnConnectTimeout()
    {
        AZ_Assert(m_connection->GetConnectionRole() == AzNetworking::ConnectionRole::Connector, "Timeout should only be queued for connectors");

        if (m_connection->GetConnectionState() == AzNetworking::ConnectionState::Connecting)
        {
            //NovaGameHubServer::RequestNewNoncesToReconnect::Request request;
            //request.SetReconnectingServerShardId(GetShardId());
            //gNovaGame->GetNovaServiceAgent().DispatchRequest(request, 0, &gNovaGame->GetNovaServiceAgent());
            //AZLOG(Debug_UdpServerConnect, "Sent RequestNewNoncesToReconnect shardId:%u", static_cast<uint32_t>(GetShardId()));
            //
            //// Requeue in case we need to request additional nonces
            //m_ConnectTimedEvent.Enqueue(TimeMs(sv_ServerToServerReconnectDelayMs + sv_ServerNonceTimeoutMs));
        }
    }
}
