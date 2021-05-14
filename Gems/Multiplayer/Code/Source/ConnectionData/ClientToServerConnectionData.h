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

#include <Multiplayer/IConnectionData.h>
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
