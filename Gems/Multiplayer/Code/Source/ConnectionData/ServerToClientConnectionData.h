/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Multiplayer/ConnectionData/IConnectionData.h>
#include <Source/NetworkEntity/EntityReplication/EntityReplicationManager.h>

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
        void Update(AZ::TimeMs hostTimeMs) override;
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
        EntityServerMigrationEvent::Handler m_controlledEntityMigrationHandler;
        AzNetworking::IConnection* m_connection = nullptr;
        bool m_canSendUpdates = false;
    };
}

#include <Source/ConnectionData/ServerToClientConnectionData.inl>
