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

