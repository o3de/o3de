/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Communication/LocalServer.h>
#include <AzCore/Debug/Trace.h>
#include <QString>

namespace AtomToolsFramework
{
    LocalServer::LocalServer()
    {
        AZ_TracePrintf("AtomToolsFramework::LocalServer", "Creating local server\n");
        m_server.setSocketOptions(QLocalServer::WorldAccessOption);

        QObject::connect(&m_server, &QLocalServer::newConnection, this, [this]() { AddConnection(m_server.nextPendingConnection()); });
    }

    LocalServer::~LocalServer()
    {
        Disconnect();
    }

    bool LocalServer::Connect(const QString& serverName)
    {
        Disconnect();

        m_serverName = serverName;

        AZ_TracePrintf("AtomToolsFramework::LocalServer", "Starting: %s\n", m_serverName.toUtf8().constData());
        if (m_server.listen(m_serverName))
        {
            AZ_TracePrintf("AtomToolsFramework::LocalServer", "Started: %s\n", m_serverName.toUtf8().constData());
            return true;
        }

        if (m_server.serverError() == QAbstractSocket::AddressInUseError)
        {
            AZ_TracePrintf("AtomToolsFramework::LocalServer", "Restarting: %s\n", m_serverName.toUtf8().constData());
            Disconnect();

            AZ_TracePrintf("AtomToolsFramework::LocalServer", "Starting: %s\n", m_serverName.toUtf8().constData());
            if (m_server.listen(m_serverName))
            {
                return true;
            }
        }

        AZ_TracePrintf("AtomToolsFramework::LocalServer", "Starting failed: %s\n", m_serverName.toUtf8().constData());
        Disconnect();
        return false;
    }

    void LocalServer::Disconnect()
    {
        AZ_TracePrintf("AtomToolsFramework::LocalServer", "Disconnecting: %s\n", m_serverName.toUtf8().constData());
        QLocalServer::removeServer(m_serverName);
    }

    bool LocalServer::IsConnected() const
    {
        return m_server.isListening();
    }

    void LocalServer::SetReadHandler(LocalServer::ReadHandler handler)
    {
        m_readHandler = handler;
    }

    void LocalServer::AddConnection(QLocalSocket* connection)
    {
        AZ_TracePrintf("AtomToolsFramework::LocalServer", "Connection added: %s\n", m_serverName.toUtf8().constData());
        QObject::connect(connection, &QLocalSocket::readyRead, this, [this, connection]() { ReadFromConnection(connection); });
        QObject::connect(connection, &QLocalSocket::disconnected, this, [this, connection]() { DeleteConnection(connection); });
    }

    void LocalServer::ReadFromConnection(QLocalSocket* connection)
    {
        if (connection)
        {
            AZ_TracePrintf("AtomToolsFramework::LocalServer", "Data received: %s\n", m_serverName.toUtf8().constData());
            QByteArray buffer = connection->readAll();
            if (m_readHandler)
            {
                m_readHandler(buffer);
            }
        }
    }

    void LocalServer::DeleteConnection(QLocalSocket* connection)
    {
        if (connection)
        {
            AZ_TracePrintf("AtomToolsFramework::LocalServer", "Deleting connection: %s\n", m_serverName.toUtf8().constData());
            connection->deleteLater();
        }
    }
} // namespace AtomToolsFramework

#include <AtomToolsFramework/Communication/moc_LocalServer.cpp>
