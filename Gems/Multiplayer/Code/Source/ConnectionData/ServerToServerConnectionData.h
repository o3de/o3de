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

#pragma once

#include <Source/ConnectionData/IConnectionData.h>
#include <Source/NetworkEntity/INetworkEntityManager.h>
#include <Source/NetworkEntity/EntityReplication/EntityReplicationManager.h>

namespace Multiplayer
{
    class ServerToServerConnectionData
        : public IConnectionData
    {
    public:

        //! Constructor
        //! @param connection         connection to other server
        //! @param connectionListener the connection listener interface for handling packets
        //! @param serverAddress      the address for the remote server
        ServerToServerConnectionData
        (
            AzNetworking::IConnection* connection,
            AzNetworking::IConnectionListener& connectionListener,
            const AzNetworking::IpAddress& serverAddress
        );
        ~ServerToServerConnectionData() override;

        //! IConnectionData interface
        //! @{
        ConnectionDataType GetConnectionDataType() const override;
        AzNetworking::IConnection* GetConnection() const override;
        EntityReplicationManager& GetReplicationManager() override;
        void Update(AZ::TimeMs serverGameTimeMs) override;
        //! @}

        const AzNetworking::IpAddress& GetServerAddress() const;

        bool IsReady();
        void SetIsReady(bool isReady);

        //! Get my server shard Id
        //! @return return shard Id
        HostId GetHostId() const;

    private:
        void OnConnectTimeout();

        AzNetworking::IpAddress m_serverAddress;
        AzNetworking::IConnection* m_connection = nullptr;
        EntityReplicationManager m_entityReplicationManager;
        AZ::ScheduledEvent m_connectEvent; //< Connection timeout handler
        bool m_isReady = false;

        AZ_DISABLE_COPY_MOVE(ServerToServerConnectionData);
    };
}

#include <Source/ConnectionData/ServerToServerConnectionData.inl>
