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
    class ClientToServerConnectionData final
        : public IConnectionData
    {
    public:
        ClientToServerConnectionData
        (
            AzNetworking::IConnection* connection,
            AzNetworking::IConnectionListener& connectionListener
        );
        ~ClientToServerConnectionData() override;

        //! IConnectionData interface
        //! @{
        ConnectionDataType GetConnectionDataType() const override;
        AzNetworking::IConnection* GetConnection() const override;
        EntityReplicationManager& GetReplicationManager() override;
        void Update(AZ::TimeMs hostTimeMs) override;
        bool CanSendUpdates() const override;
        void SetCanSendUpdates(bool canSendUpdates) override;
        //! @}

    private:
        EntityReplicationManager m_entityReplicationManager;
        AzNetworking::IConnection* m_connection = nullptr;
        bool m_canSendUpdates = true;
    };
}

#include <Source/ConnectionData/ClientToServerConnectionData.inl>
