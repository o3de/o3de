/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Multiplayer/NetworkEntity/INetworkEntityManager.h>
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
        virtual void Update() = 0;

        //! Returns whether update messages can be sent to the connection.
        //! @return true if update messages can be sent
        virtual bool CanSendUpdates() const = 0;

        //! Sets the state of connection whether update messages can be sent or not.
        //! @param canSendUpdates the state value
        virtual void SetCanSendUpdates(bool canSendUpdates) = 0;

        //! Fetches the state of connection whether handshake logic has completed
        //! @return true if handshake has completed
        virtual bool DidHandshake() const = 0;

        //! Sets the state of connection whether handshake logic has completed
        //! @param didHandshake if handshake logic has completed
        virtual void SetDidHandshake(bool didHandshake) = 0;
    };
}
