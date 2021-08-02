/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/TypeSafeIntegral.h>
#include <AzNetworking/ConnectionLayer/IConnectionSet.h>
#include <AzNetworking/TcpTransport/TcpConnection.h>
#include <AzCore/std/containers/unordered_map.h>

namespace AzNetworking
{
    //! @class TcpConnectionSet
    //! @brief Tracks current TCP connections and allows fast lookups by socket fd and connection identifier.
    class TcpConnectionSet final
        : public IConnectionSet
    {
    public:

        using ConnectionIdMap = AZStd::unordered_map<ConnectionId, AZStd::unique_ptr<TcpConnection>>;
        using SocketFdMap = AZStd::unordered_map<SocketFd, TcpConnection*>;

        TcpConnectionSet() = default;
        virtual ~TcpConnectionSet() = default;

        //! Adds a new connection to this connection list instance.
        //! @param connection pointer to the connection instance to add
        //! @return boolean true on success
        bool AddConnection(AZStd::unique_ptr<TcpConnection> connection);

        //! Deletes a connection from this connection list instance by socket fd.
        //! @param socketFD socket file descriptor of the connection to delete
        //! @return boolean true on success
        bool DeleteConnection(SocketFd socketFd);

        //! IConnectionSet interface.
        //! @{
        void VisitConnections(const ConnectionVisitor& visitor) override;
        bool DeleteConnection(ConnectionId connectionId) override;
        IConnection* GetConnection(ConnectionId connectionId) const override;
        ConnectionId GetNextConnectionId() override;
        uint32_t GetConnectionCount() const override;
        uint32_t GetActiveConnectionCount() const override;
        //! @}

        //! Retrieves a connection from this connection list instance by socket fd.
        //! @param socketFD socket file descriptor of the connection to retrieve
        //! @return pointer to the requested connection instance on success, nullptr on failure
        TcpConnection* GetConnection(SocketFd socketFd) const;

        //! Returns the set of SocketFds that should be bound to this connection list instance.
        //! @return the set of SocketFds that should be bound to this connection list instance
        const SocketFdMap& GetSocketFdMap() const;

    private:

        ConnectionId    m_nextConnectionId = InvalidConnectionId;
        ConnectionIdMap m_connectionIdMap;
        SocketFdMap     m_socketFdMap;
    };
}
