/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Multiplayer/ConnectionData/IConnectionData.h>
#include <Multiplayer/NetworkEntity/EntityReplication/EntityReplicationManager.h>

namespace Multiplayer
{
    class ServerToClientConnectionData final
        : public IConnectionData
    {
    public:
        ServerToClientConnectionData
        (
            AzNetworking::IConnection* connection,
            AzNetworking::IConnectionListener& connectionListener
        );
        ~ServerToClientConnectionData() override;

        void SetControlledEntity(NetworkEntityHandle primaryPlayerEntity);

        //! IConnectionData interface
        //! @{
        ConnectionDataType GetConnectionDataType() const override;
        AzNetworking::IConnection* GetConnection() const override;
        EntityReplicationManager& GetReplicationManager() override;
        void Update() override;
        bool CanSendUpdates() const override;
        void SetCanSendUpdates(bool canSendUpdates) override;
        bool DidHandshake() const override;
        void SetDidHandshake(bool didHandshake) override;
        //! @}

        NetworkEntityHandle GetPrimaryPlayerEntity();
        const NetworkEntityHandle& GetPrimaryPlayerEntity() const;
        const AZStd::string& GetProviderTicket() const;
        void SetProviderTicket(const AZStd::string&);

    private:
        void OnControlledEntityRemove();
        void OnControlledEntityMigration(const ConstNetworkEntityHandle& entityHandle, const HostId& remoteHostId);
        void OnGameplayStarted();

        EntityReplicationManager m_entityReplicationManager;
        NetworkEntityHandle m_controlledEntity;
        EntityStopEvent::Handler m_controlledEntityRemovedHandler;
        EntityServerMigrationEvent::Handler m_controlledEntityMigrationHandler;
        AZStd::string m_providerTicket;
        AzNetworking::IConnection* m_connection = nullptr;
        bool m_canSendUpdates = false;
        bool m_didHandshake = false;
    };
}

#include <Source/ConnectionData/ServerToClientConnectionData.inl>
