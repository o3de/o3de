/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <native/AssetManager/ControlRequestHandler.h>

#if !defined(Q_MOC_RUN)
#include <QHostAddress>
#include <QTcpSocket>
#include <QTcpServer>
#endif

#include <native/assetprocessor.h>
#include <native/utilities/ApplicationManagerBase.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Debug/Trace.h>

ControlRequestHandler::ControlRequestHandler(ApplicationManagerBase* parent) : QObject(parent),
m_applicationManager(parent)
{
    connect(m_applicationManager, &ApplicationManagerBase::FullIdle, this, &ControlRequestHandler::AssetManagerIdleStateChange);

    StartListening(0);

}

ControlRequestHandler::~ControlRequestHandler()
{

}

bool ControlRequestHandler::StartListening(unsigned short port)
{
    if (!m_tcpServer)
    {
        m_tcpServer = new QTcpServer(this);
    }
    if (!m_tcpServer->isListening())
    {
        if (!m_tcpServer->listen(QHostAddress::LocalHost, port))
        {
            AZ_Error(AssetProcessor::ConsoleChannel, false, "Control Request Handler couldn't listen on requested port %d", port);
            return false;
        }
        port = m_tcpServer->serverPort();
        AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Control Port: %d\n", port);
        connect(m_tcpServer, &QTcpServer::newConnection, this, &ControlRequestHandler::GotConnection);
        AZ_TracePrintf(AssetProcessor::DebugChannel, "Asset Processor Control Request Handler listening on port %d\n", port);
    }
    return true;
}

void ControlRequestHandler::GotConnection()
{
    if (m_tcpServer->hasPendingConnections())
    {
        QTcpSocket* newSocket = m_tcpServer->nextPendingConnection();
        connect(newSocket, &QTcpSocket::stateChanged, this, &ControlRequestHandler::SocketStateUpdate);
        connect(newSocket, &QTcpSocket::readyRead, this, &ControlRequestHandler::DataReceived);
        connect(newSocket, &QTcpSocket::disconnected, this, &ControlRequestHandler::Disconnected);

        m_listenSockets.push_back(newSocket);

        AZ_TracePrintf(AssetProcessor::DebugChannel, "Asset Processor Control Request Handler got new connection\n");
        if (newSocket->bytesAvailable())
        {
            AZ_TracePrintf(AssetProcessor::DebugChannel, "Asset Processor Control Request Handler socket had data available\n");
            ReadData(newSocket);
        }
    }
}

void ControlRequestHandler::SocketStateUpdate(QAbstractSocket::SocketState newState)
{
    if (newState == QAbstractSocket::UnconnectedState)
    {
        m_listenSockets.removeOne(static_cast<QTcpSocket*>(QObject::sender()));
    }
}

void ControlRequestHandler::DataReceived()
{
    QTcpSocket* incoming = static_cast<QTcpSocket*>(QObject::sender());
    ReadData(incoming);
}

void ControlRequestHandler::ReadData(QTcpSocket* incoming)
{
    if (!incoming)
    {
        AZ_Error(AssetProcessor::DebugChannel, false, "Attempting to read from null QTcpSocket in ControlRequestHandler");
        return;
    }
    auto sentMessage = incoming->readAll().toStdString();
    AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Got Control request %s\n", sentMessage.c_str());
    if (sentMessage == "quit")
    {
        QMetaObject::invokeMethod(parent(), "QuitRequested", Qt::QueuedConnection);
    }
    else if (sentMessage == "ping")
    {
        incoming->write("pong");
    }
    else if (sentMessage == "isidle")
    {
        bool isIdle = m_applicationManager->IsAssetProcessorManagerIdle();
        incoming->write(isIdle ? "true" : "false");
    }
    else if (sentMessage == "waitforidle")
    {
        bool isIdle = m_applicationManager->CheckFullIdle();
        if (isIdle)
        {
            AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Control request responding idle\n");
            incoming->write("idle");
        }
        else
        {
            AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Control request adding wait idle waiter\n");
            m_idleWaitSockets.push_back(incoming);
        }
    }
    else if (sentMessage == "signalidle")
    {
        AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Control request adding signal idle waiter\n");
        m_idleWaitSockets.push_back(incoming);
    }
    else if (sentMessage == "windowid")
    {
        incoming->write(AZStd::string::format("%lld", m_applicationManager->GetWindowId()).c_str());
    }
}

void ControlRequestHandler::Disconnected()
{
    QTcpSocket* incoming = static_cast<QTcpSocket*>(QObject::sender());
    m_listenSockets.removeOne(incoming);
    incoming->deleteLater();
}

void ControlRequestHandler::AssetManagerIdleStateChange(bool isIdle)
{
    AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Control Request Got idle state %d with %d waiters\n", isIdle, m_idleWaitSockets.size());
    if (!isIdle)
    {
        // We only currently care when transitioning to idle
        return;
    }

    for (auto& thisConnection : m_idleWaitSockets)
    {
        if (m_listenSockets.indexOf(thisConnection) != -1)
        {
            AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Control request sending idle state to socket\n");
            thisConnection->write("idle");
        }
    }
    m_idleWaitSockets.clear();
}
