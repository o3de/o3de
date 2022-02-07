/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzNetworking/Utilities/IpAddress.h>
#include <AzNetworking/ConnectionLayer/IConnectionSet.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AzNetworking
{
    class UdpConnection;

    //! @class UdpConnectionSet
    //! @brief Tracks current UDP endpoints and allows fast lookups by connection identifier and remote address.
    class UdpConnectionSet final
        : public IConnectionSet
    {
    public:

        using ConnectionIdMap = AZStd::unordered_map<ConnectionId, AZStd::unique_ptr<UdpConnection>>;
        using RemoteAddressMap = AZStd::unordered_map<IpAddress, UdpConnection*>;

        UdpConnectionSet() = default;
        ~UdpConnectionSet() override = default;

        //! Adds a new connection to this connection list instance.
        //! @param connection pointer to the connection instance to add
        //! @return boolean true on success
        bool AddConnection(AZStd::unique_ptr<UdpConnection> connection);

        //! Deletes a connection from this connection list instance by endpoint remote address.
        //! @param address address of the remote endpoint to delete
        //! @return boolean true on success
        bool DeleteConnection(const IpAddress& address);

        //! IConnectionSet interface.
        //! @{
        void VisitConnections(const ConnectionVisitor& visitor) override;
        bool DeleteConnection(ConnectionId connectionId) override;
        IConnection* GetConnection(ConnectionId connectionId) const override;
        ConnectionId GetNextConnectionId() override;
        uint32_t GetConnectionCount() const override;
        uint32_t GetActiveConnectionCount() const override;
        //! @}

        //! Retrieves a connection from this connection list instance by endpoint remote address
        //! @param address address of the remote endpoint of the connection to retrieve
        //! @return pointer to the requested connection instance on success, nullptr on failure
        UdpConnection* GetConnection(const IpAddress& address) const;

    private:

        ConnectionId     m_nextConnectionId = InvalidConnectionId;
        ConnectionIdMap  m_connectionIdMap;
        RemoteAddressMap m_remoteAddressMap;
    };
}
