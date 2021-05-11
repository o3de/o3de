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

#include <Include/INetworkEntityManager.h>
#include <AzNetworking/ConnectionLayer/IConnection.h>

namespace Multiplayer
{
    class EntityReplicationManager;

    enum class ConnectionDataType
    {
        ClientToServer,
        ServerToClient,
        ServerToServer
    };

    class IConnectionData
    {
    public:
        virtual ~IConnectionData() = default;

        //! Returns whether or not this is a ServerToClient or ServerToServer connection data instance.
        //! @return ConnectionDataType::ServerToClient or ConnectionDataType::ServerToServer
        virtual ConnectionDataType GetConnectionDataType() const = 0;

        //! Returns the connection bound to this connection data instance.
        //! @return pointer to the connection bound to this connection data instance
        virtual AzNetworking::IConnection* GetConnection() const = 0;

        //! Returns the EntityReplicationManager for this connection data instance.
        //! @return reference to the EntityReplicationManager for this connection data instance
        virtual EntityReplicationManager& GetReplicationManager() = 0;

        //! Creates and manages sending updates to the remote endpoint.
        //! @param hostTimeMs current server game time in milliseconds
        virtual void Update(AZ::TimeMs hostTimeMs) = 0;

        //! Returns whether update messages can be sent to the connection.
        //! @return true if update messages can be sent
        virtual bool CanSendUpdates() const = 0;

        //! Sets the state of connection whether update messages can be sent or not.
        //! @param canSendUpdates the state value
        virtual void SetCanSendUpdates(bool canSendUpdates) = 0;
    };
}
