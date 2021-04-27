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

namespace Multiplayer
{
    class ServerToClientConnectionData final
        : public IConnectionData
    {
    public:
        ServerToClientConnectionData
        (
            AzNetworking::IConnection* connection,
            AzNetworking::IConnectionListener& connectionListener,
            NetworkEntityHandle controlledEntity
        );
        ~ServerToClientConnectionData() override;

        //! IConnectionData interface
        //! @{
        ConnectionDataType GetConnectionDataType() const override;
        AzNetworking::IConnection* GetConnection() const override;
        EntityReplicationManager& GetReplicationManager() override;
        void Update(AZ::TimeMs serverGameTimeMs) override;
        bool CanSendUpdates() const override;
        void SetCanSendUpdates(bool canSendUpdates) override;
        //! @}

        NetworkEntityHandle GetPrimaryPlayerEntity();
        const NetworkEntityHandle& GetPrimaryPlayerEntity() const;

    private:
        void OnControlledEntityRemove();
        void OnControlledEntityMigration(const ConstNetworkEntityHandle& entityHandle, HostId remoteHostId, AzNetworking::ConnectionId connectionId);
        void OnGameplayStarted();

        EntityReplicationManager m_entityReplicationManager;
        NetworkEntityHandle m_controlledEntity;
        EntityStopEvent::Handler m_controlledEntityRemovedHandler;
        EntityMigrationEvent::Handler m_controlledEntityMigrationHandler;
        AzNetworking::IConnection* m_connection = nullptr;
        bool m_canSendUpdates = false;
    };
}

#include <Source/ConnectionData/ServerToClientConnectionData.inl>
