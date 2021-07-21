/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzNetworking/ConnectionLayer/IConnection.h>

namespace AzNetworking
{
    //! @class IConnectionSet
    //! @brief interface class for managing a set of connections.
    //!
    //! IConnectionSet defines a simple interface for working with an abstract set of IConnections bound to an
    //! INetworkInterface.  Generally users of AzNetworking will not have reason to interact directly with the IConnectionSet,
    //! as its interface is completely wrapped by INetworkInterface.

    class IConnectionSet
    {
    public:

        using ConnectionVisitor = AZStd::function<void(IConnection&)>;

        virtual ~IConnectionSet() = default;

        //! Will visit each active connection in the connection set and invoke the provided connection visitor.
        //! @param visitor the visitor to visit each connection with
        virtual void VisitConnections(const ConnectionVisitor& visitor) = 0;

        //! Deletes a connection from this connection list instance by connection identifier.
        //! @param connectionId connection identifier of the connection to delete
        //! @return boolean true on success
        virtual bool DeleteConnection(ConnectionId connectionId) = 0;

        //! Retrieves a connection from this connection set by connection identifier.
        //! @param connectionId connection identifier of the connection to retrieve
        //! @return pointer to the requested connection instance on success, nullptr on failure
        virtual IConnection* GetConnection(ConnectionId connectionId) const = 0;

        //! Returns the next valid connection identifier for this connection list instance.
        //! @return a valid connection identifier to give a new connection instance, or InvalidConnectionId on failure
        virtual ConnectionId GetNextConnectionId() = 0;

        //! Returns the current total connection count for this connection set
        //! @return the current total connection count for this connection set
        virtual uint32_t GetConnectionCount() const = 0;
    };
}
