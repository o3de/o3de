/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/UdpTransport/UdpConnectionSet.h>
#include <AzNetworking/UdpTransport/UdpConnection.h>

namespace AzNetworking
{
    bool UdpConnectionSet::AddConnection(AZStd::unique_ptr<UdpConnection> connection)
    {
        AZ_Assert(connection, "Adding a nullptr UdpConnection instance to the connection set");
        if (!connection)
        {
            return false;
        }

        AZLOG(UdpConnectionSet, "Adding new Udp connection (%u : %s)",
            aznumeric_cast<uint32_t>(connection->GetConnectionId()),
            connection->GetRemoteAddress().GetString().c_str()
        );

        // Check for errors here, don't want to clobber an existing connection...
        AZ_Assert(GetConnection(connection->GetConnectionId()) == nullptr, "ConnectionId already exists in connection set");
        AZ_Assert(GetConnection(connection->GetRemoteAddress()) == nullptr, "Remote address already exists in connection set");
        m_remoteAddressMap[connection->GetRemoteAddress()] = connection.get();
        m_connectionIdMap[connection->GetConnectionId()] = AZStd::move(connection);
        return true;
    }

    bool UdpConnectionSet::DeleteConnection(const IpAddress& address)
    {
        AZLOG(UdpConnectionSet, "Deleting Udp connection by remote address (%s)", address.GetString().c_str());
        UdpConnection* connection = GetConnection(address);
        if (connection == nullptr)
        {
            return false;
        }
        AZLOG(UdpConnectionSet, "Deleting Udp connection (%u : %s)",
            aznumeric_cast<uint32_t>(connection->GetConnectionId()),
            connection->GetRemoteAddress().GetString().c_str()
        );

        AZ_Assert(connection->GetRemoteAddress() == address, "Connection list is corrupt, mismatched remote endpoint addresses detected");
        m_remoteAddressMap.erase(connection->GetRemoteAddress());
        m_connectionIdMap.erase(connection->GetConnectionId());
        return true;
    }

    void UdpConnectionSet::VisitConnections(const ConnectionVisitor& visitor)
    {
        for (auto& connection : m_connectionIdMap)
        {
            visitor(*connection.second);
        }
    }

    bool UdpConnectionSet::DeleteConnection(ConnectionId connectionId)
    {
        AZLOG(UdpConnectionSet, "Deleting Udp connection by connectionId (%u)", aznumeric_cast<uint32_t>(connectionId));
        UdpConnection* connection = static_cast<UdpConnection*>(GetConnection(connectionId));
        if (connection == nullptr)
        {
            return false;
        }
        AZLOG(UdpConnectionSet, "Deleting Udp connection (%u : %s)",
            aznumeric_cast<uint32_t>(connectionId),
            connection->GetRemoteAddress().GetString().c_str()
        );

        AZ_Assert(connection->GetConnectionId() == connectionId, "Connection list is corrupt, mismatched connection identifiers detected");
        m_remoteAddressMap.erase(connection->GetRemoteAddress());
        m_connectionIdMap.erase(connectionId);
        return true;
    }

    IConnection* UdpConnectionSet::GetConnection(ConnectionId connectionId) const
    {
        ConnectionIdMap::const_iterator lookup = m_connectionIdMap.find(connectionId);
        if (lookup != m_connectionIdMap.end())
        {
            return lookup->second.get();
        }
        return nullptr;
    }

    ConnectionId UdpConnectionSet::GetNextConnectionId()
    {
        // In the case of wrap-around, don't return a connectionId that's in-use or is the invalid connection Id
        do
        {
            ++m_nextConnectionId;
            if (m_nextConnectionId == InvalidConnectionId)
            {
                m_nextConnectionId = ConnectionId(0);
            }
        } while (m_connectionIdMap.count(m_nextConnectionId) > 0);
        return m_nextConnectionId;
    }

    uint32_t UdpConnectionSet::GetConnectionCount() const
    {
        return aznumeric_cast<uint32_t>(m_connectionIdMap.size());
    }

    UdpConnection* UdpConnectionSet::GetConnection(const IpAddress& address) const
    {
        RemoteAddressMap::const_iterator lookup = m_remoteAddressMap.find(address);
        if (lookup != m_remoteAddressMap.end())
        {
            return lookup->second;
        }
        return nullptr;
    }
}
