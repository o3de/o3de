/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "native/utilities/ApplicationServer.h"
#include <QTcpSocket>

const char ApplicationServer::RandomListeningPortOption[] = "randomListeningPort";

ApplicationServer::ApplicationServer(QObject* parent)
    : QTcpServer(parent)
{
}

ApplicationServer::~ApplicationServer()
{
    ApplicationServerBus::Handler::BusDisconnect();
}

void ApplicationServer::incomingConnection(qintptr socketDescriptor)
{
    if (m_isShuttingDown)
    {
        // deny the connection and return.
        QTcpSocket sock;
        sock.setSocketDescriptor(socketDescriptor, QAbstractSocket::ConnectedState, QIODevice::ReadWrite);
        sock.close();
        return;
    }

    Q_EMIT newIncomingConnection(socketDescriptor);
}

int ApplicationServer::GetServerListeningPort() const
{
    return m_serverListeningPort;
}

void ApplicationServer::QuitRequested()
{
    // stop accepting messages and close the connection immediately.
    pauseAccepting();
    close();
    Q_EMIT ReadyToQuit(this);
}

