/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "native/utilities/UnitTestShaderCompilerServer.h"
#include "native/assetprocessor.h"
#include <QTcpServer>
#include <QTcpSocket>


UnitTestShaderCompilerServer::UnitTestShaderCompilerServer(QObject* parent)
    : QObject(parent)
    , m_incomingPayload("This is a test string")
    , m_outgoingPayload("Test string validated")
    , m_serverAddress(QString())
    , m_serverPort(0)
    , m_server(nullptr)
    , m_socket(nullptr)
    , m_isPayloadSizeKnown(false)
    , m_totalBytesReadInPayload(0)
    , m_bytesRemainingInPayload(0)
    , m_bytesRemainingInPayloadSize(0)
    , m_totalBytesReadInPayloadSize(0)
    , m_payloadSize(0)
{
}

UnitTestShaderCompilerServer::~UnitTestShaderCompilerServer()
{
    if (m_server != nullptr)
    {
        m_server->deleteLater();
    }
}

void UnitTestShaderCompilerServer::Init(QString serverAddress, int serverPort)
{
    m_serverAddress = serverAddress;
    m_serverPort = serverPort;
    m_server = new QTcpServer(this);
    connect(m_server, SIGNAL(newConnection()), this, SLOT(newConnection()));
    startServer();
}

void UnitTestShaderCompilerServer::startServer()
{
    if (!m_server->isListening())
    {
        if (!m_server->listen(QHostAddress(m_serverAddress), static_cast<quint16>(m_serverPort)))
        {
            AZ_TracePrintf(AssetProcessor::DebugChannel, "Server %s could not start.\n", m_serverAddress.toUtf8().data());
            emit errorMessage("Server could not start ");
        }
    }
}

void UnitTestShaderCompilerServer::closeSocket()
{
    m_socket->close();
}

void UnitTestShaderCompilerServer::newConnection()
{
    m_socket = m_server->nextPendingConnection();
    if (m_serverStatus == BadServer_DisconnectAfterConnect)
    {
        closeSocket();
        return;
    }
    connect(m_socket, SIGNAL(readyRead()), this, SLOT(incomingMessage()));
    connect(m_socket, SIGNAL(disconnected()), m_socket, SLOT(deleteLater()));
    m_isPayloadSizeKnown = false;
    m_totalBytesReadInPayload = 0;
    m_bytesRemainingInPayload = 0;
    m_totalBytesReadInPayloadSize = 0;
    m_bytesRemainingInPayloadSize = 0;
    m_payloadSize = 0;
    m_payload.clear();
}

void UnitTestShaderCompilerServer::incomingMessage()
{
    if (m_serverStatus == BadServer_DisconnectAfterConnect || m_socket->bytesAvailable() == 0)
    {
        return;
    }

    if (!m_isPayloadSizeKnown)
    {
        //reading payload size
        m_bytesRemainingInPayloadSize =  static_cast<qint64>(sizeof(qint64)) - m_totalBytesReadInPayloadSize;

        qint64 bytesAvailable = m_socket->bytesAvailable();
        qint64 bytesToread = qMin(bytesAvailable, m_bytesRemainingInPayloadSize);

        qint64 bytesRead = m_socket->read(reinterpret_cast<char*>(&m_payloadSize) + m_totalBytesReadInPayloadSize, bytesToread);
        if (bytesRead < 0)
        {
            AZ_TracePrintf(AssetProcessor::DebugChannel, "Connection Lost : Cannot read from socket.\n");
            emit errorMessage("Connection Lost:Cannot read from socket");
            return;
        }
        m_totalBytesReadInPayloadSize += bytesRead;

        if (m_totalBytesReadInPayloadSize == static_cast<qint64>(sizeof(qint64)))
        {
            m_isPayloadSizeKnown = true;
            m_payload.resize(aznumeric_cast<int>(m_payloadSize));
        }

        if (m_socket->bytesAvailable() > 0)
        {
            QMetaObject::invokeMethod(this, "incomingMessage", Qt::QueuedConnection);
            return;
        }
    }
    else
    {
        // payload size is known,read the payload
        m_bytesRemainingInPayload =  m_payloadSize - m_totalBytesReadInPayload;

        qint64 bytesAvailable = m_socket->bytesAvailable();
        qint64 bytesToread = qMin(bytesAvailable, m_bytesRemainingInPayload);
        qint64 bytesRead = m_socket->read(m_payload.data() + m_totalBytesReadInPayload, bytesToread);
        if (bytesRead < 0)
        {
            AZ_TracePrintf(AssetProcessor::DebugChannel, "Connection Lost:Cannot read from socket.\n");
            emit errorMessage("Connection Lost:Cannot read from socket");
            return;
        }
        m_totalBytesReadInPayload += bytesRead;

        if (m_socket->bytesAvailable() > 0)
        {
            QMetaObject::invokeMethod(this, "incomingMessage", Qt::QueuedConnection);
            return;
        }

        if (m_totalBytesReadInPayload != m_payloadSize)
        {
            return;
        }

        if (m_serverStatus == BadServer_ReadsPayloadAndDisconnect)
        {
            closeSocket();
            return;
        }
        //we have the complete payload here
        //compare it with the expected payload
        if (QString::compare(QString(m_payload), m_incomingPayload) == 0)
        {
            QByteArray payload;
            qint64 messageSize;
            constructPayload(payload);
            messageSize = static_cast<qint64>(payload.size());
            if (m_serverStatus == BadServer_SendsIncompletePayload)
            {
                messageSize -= 5;
            }
            qint64 bytesWritten = 0;
            while (bytesWritten != messageSize)
            {
                qint64 currentWrite = m_socket->write(payload.data() + bytesWritten, messageSize - bytesWritten);
                if (currentWrite < 0)
                {
                    AZ_TracePrintf(AssetProcessor::DebugChannel, "Connection Lost:Cannot write to socket.\n");
                    emit errorMessage("Connection Lost:Cannot write to socket");
                    return;
                }
                bytesWritten += currentWrite;
            }
        }
        else
        {
            AZ_TracePrintf(AssetProcessor::DebugChannel, "Server Payload is corrupt.\n");
            emit errorMessage("Server Payload is corrupt");
            return;
        }
    }
}

void UnitTestShaderCompilerServer::setServerStatus(const ServerStatus& serverStatus)
{
    m_serverStatus = serverStatus;
}


void UnitTestShaderCompilerServer::constructPayload(QByteArray& payload)
{
    //construct test response payload
    quint8 status = 1;
    unsigned int outgoingTextLength = static_cast<unsigned int>(m_outgoingPayload.size());
    payload.resize(outgoingTextLength + sizeof(unsigned int) + sizeof(quint8));
    memcpy(payload.data(), reinterpret_cast<char*>(&outgoingTextLength), sizeof(unsigned int));
    memcpy(payload.data() + sizeof(unsigned int), reinterpret_cast<char*>(&status), sizeof(quint8));
    memcpy(payload.data() + sizeof(unsigned int) + sizeof(quint8), m_outgoingPayload.toStdString().c_str(), outgoingTextLength);
}



