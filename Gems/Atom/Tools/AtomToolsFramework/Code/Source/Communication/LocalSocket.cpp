/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Communication/LocalSocket.h>
#include <AzCore/Debug/Trace.h>
#include <QByteArray>
#include <QString>

namespace AtomToolsFramework
{
    LocalSocket::LocalSocket()
    {
    }

    LocalSocket::~LocalSocket()
    {
        Disconnect();
    }

    bool LocalSocket::Connect(const QString& serverName)
    {
        Disconnect();

        m_serverName = serverName;

        AZ_TracePrintf("AtomToolsFramework::LocalSocket", "Connecting to: %s\n", m_serverName.toUtf8().constData());
        m_socket.connectToServer(m_serverName);

        if (IsConnected())
        {
            AZ_TracePrintf("AtomToolsFramework::LocalSocket", "Waiting for connection to: %s\n", m_serverName.toUtf8().constData());
            if (m_socket.waitForConnected())
            {
                AZ_TracePrintf("AtomToolsFramework::LocalSocket", "Connected to: %s\n", m_serverName.toUtf8().constData());
                return true;
            }
        }

        AZ_TracePrintf("AtomToolsFramework::LocalSocket", "Connecting failed: %s\n", m_serverName.toUtf8().constData());
        Disconnect();
        return false;
    }

    void LocalSocket::Disconnect()
    {
        if (IsConnected())
        {
            AZ_TracePrintf("AtomToolsFramework::LocalSocket", "Disconnecting from: %s\n", m_serverName.toUtf8().constData());
            m_socket.disconnectFromServer();
            m_socket.waitForDisconnected();

            AZ_TracePrintf("AtomToolsFramework::LocalSocket", "Closing socket\n");
            m_socket.close();
        }
    }

    bool LocalSocket::IsConnected() const
    {
        return m_socket.isOpen();
    }

    bool LocalSocket::Send(const QByteArray& buffer)
    {
        if (IsConnected())
        {
            AZ_TracePrintf("AtomToolsFramework::LocalSocket", "Sending data to: %s\n", m_serverName.toUtf8().constData());
            m_socket.write(buffer);

            AZ_TracePrintf("AtomToolsFramework::LocalSocket", "Waiting for write to: %s\n", m_serverName.toUtf8().constData());
            m_socket.waitForBytesWritten();
            return true;
        }
        return false;
    }
} // namespace AtomToolsFramework

#include <AtomToolsFramework/Communication/moc_LocalSocket.cpp>
