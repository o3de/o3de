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
    class ClientToServerConnectionData final
        : public IConnectionData
    {
    public:
        ClientToServerConnectionData
        (
            AzNetworking::IConnection* connection,
            AzNetworking::IConnectionListener& connectionListener,
            const AZStd::string& providerTicket = ""
        );
        ~ClientToServerConnectionData() override;

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

        const AZStd::string& GetProviderTicket() const;
        void SetProviderTicket(const AZStd::string&);

    private:
        EntityReplicationManager m_entityReplicationManager;
        AZStd::string m_providerTicket;
        AzNetworking::IConnection* m_connection = nullptr;
        bool m_canSendUpdates = true;
        bool m_didHandshake = false;
    };
}

#include <Source/ConnectionData/ClientToServerConnectionData.inl>
