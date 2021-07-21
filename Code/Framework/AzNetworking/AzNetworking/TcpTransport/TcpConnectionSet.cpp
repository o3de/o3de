/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/TcpTransport/TcpConnectionSet.h>
#include <AzCore/Console/ILogger.h>

namespace AzNetworking
{
    bool TcpConnectionSet::AddConnection(AZStd::unique_ptr<TcpConnection> connection)
    {
        AZ_Assert(connection, "Adding a nullptr TcpConnection instance to the connection set");
        if (!connection)
        {
            return false;
        }

        AZLOG(TcpConnectionSet, "Adding new Tcp connection (%u : %d)",
            aznumeric_cast<uint32_t>(connection->GetConnectionId()),
            aznumeric_cast<int32_t>(connection->GetTcpSocket()->GetSocketFd())
        );

        // Check for errors here, don't want to clobber an existing connection...
        AZ_Assert(GetConnection(connection->GetConnectionId()) == nullptr, "ConnectionId already exists in connection set");
        AZ_Assert(GetConnection(connection->GetRegisteredSocketFd()) == nullptr, "Socket file descriptor already exists in connection set");
        connection->SetRegisteredSocketFd(connection->GetTcpSocket()->GetSocketFd());
        m_socketFdMap[connection->GetRegisteredSocketFd()] = connection.get();
        m_connectionIdMap[connection->GetConnectionId()] = AZStd::move(connection);
        return true;
    }

    bool TcpConnectionSet::DeleteConnection(SocketFd socketFd)
    {
        AZLOG(TcpConnectionSet, "Deleting Tcp connection by socketId (%u)", socketFd);
        TcpConnection* connection = GetConnection(socketFd);
        if (connection == nullptr)
        {
            return false;
        }
        AZLOG(TcpConnectionSet, "Deleting Tcp connection (%u : %d)",
            aznumeric_cast<uint32_t>(connection->GetConnectionId()),
            aznumeric_cast<int32_t>(connection->GetRegisteredSocketFd())
        );

        AZ_Assert(connection->GetRegisteredSocketFd() == socketFd, "Connection list is corrupt, mismatched socket file descriptors detected");
        m_socketFdMap.erase(connection->GetRegisteredSocketFd());
        connection->SetRegisteredSocketFd(InvalidSocketFd);
        m_connectionIdMap.erase(connection->GetConnectionId());
        return true;
    }

    void TcpConnectionSet::VisitConnections(const ConnectionVisitor& visitor)
    {
        for (auto& connection : m_connectionIdMap)
        {
            visitor(*connection.second);
        }
    }

    bool TcpConnectionSet::DeleteConnection(ConnectionId connectionId)
    {
        AZLOG(TcpConnectionSet, "Deleting Tcp connection by connectionId (%u)", static_cast<uint32_t>(connectionId));
        TcpConnection* connection = static_cast<TcpConnection*>(GetConnection(connectionId));
        if (connection == nullptr)
        {
            return false;
        }
        AZLOG(TcpConnectionSet, "Deleting Tcp connection (%u : %d)",
            aznumeric_cast<uint32_t>(connectionId),
            aznumeric_cast<int32_t>(connection->GetRegisteredSocketFd())
        );

        AZ_Assert(connection->GetConnectionId() == connectionId, "Connection list is corrupt, mismatched connection identifiers detected");
        m_socketFdMap.erase(connection->GetRegisteredSocketFd());
        connection->SetRegisteredSocketFd(InvalidSocketFd);
        m_connectionIdMap.erase(connectionId);
        return true;
    }

    IConnection* TcpConnectionSet::GetConnection(ConnectionId connectionId) const
    {
        ConnectionIdMap::const_iterator lookup = m_connectionIdMap.find(connectionId);
        if (lookup != m_connectionIdMap.end())
        {
            return lookup->second.get();
        }
        return nullptr;
    }

    ConnectionId TcpConnectionSet::GetNextConnectionId()
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

    uint32_t TcpConnectionSet::GetConnectionCount() const
    {
        return aznumeric_cast<uint32_t>(m_connectionIdMap.size());
    }

    TcpConnection* TcpConnectionSet::GetConnection(SocketFd socketFd) const
    {
        SocketFdMap::const_iterator lookup = m_socketFdMap.find(socketFd);
        if (lookup != m_socketFdMap.end())
        {
            return lookup->second;
        }
        return nullptr;
    }

    const TcpConnectionSet::SocketFdMap& TcpConnectionSet::GetSocketFdMap() const
    {
        return m_socketFdMap;
    }
}
