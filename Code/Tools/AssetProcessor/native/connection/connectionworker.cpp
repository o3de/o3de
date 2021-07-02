/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "connectionworker.h"
#include "native/utilities/assetUtils.h"
#include <native/utilities/ByteArrayStream.h>
#include <QThread>

#include <QTimer>
#include <QCoreApplication>
#include <QThread>

#include <AzFramework/API/ApplicationAPI.h>


// enable this to debug negotiation - it enables a huge delay so that when a debugger attaches we don't fail.
//#define DEBUG_NEGOTIATION

#undef SendMessage
namespace AssetProcessor {
ConnectionWorker::ConnectionWorker(qintptr /*socketDescriptor*/, QObject* parent)
    : QObject(parent)
    , m_terminate(false)
{
#ifdef DEBUG_NEGOTIATION
    m_waitDelay = 60 * 10 * 1000; // 10 min in debug, in ms
#endif
    connect(&m_engineSocket, &QTcpSocket::stateChanged, this, &ConnectionWorker::EngineSocketStateChanged, Qt::QueuedConnection);
#if defined(DEBUG_NEGOTIATION)
    AZ_TracePrintf(AssetProcessor::DebugChannel, "Connection::ConnectionWorker created for socket %p: %p", socketDescriptor, this);
#endif
}

ConnectionWorker::~ConnectionWorker()
{
#if defined(DEBUG_NEGOTIATION)
    AZ_TracePrintf(AssetProcessor::DebugChannel, "ConnectionWorker::~:  %p", this);
#endif
    thread()->quit();
}

bool ConnectionWorker::ReadMessage(QTcpSocket& socket, AssetProcessor::Message& message)
{
    const qint64 sizeOfHeader = static_cast<qint64>(sizeof(AssetProcessor::MessageHeader));
    qint64 bytesAvailable = socket.bytesAvailable();
    if (bytesAvailable == 0 || bytesAvailable < sizeOfHeader)
    {
        return false;
    }

    // read header
    if (!ReadData(socket, (char*)&message.header, sizeOfHeader))
    {
        DisconnectSockets();
        return false;
    }

    // Prepare the payload buffer
    message.payload.resize(message.header.size);

    // read payload
    if (!ReadData(socket, message.payload.data(), message.header.size))
    {
        DisconnectSockets();
        return false;
    }

    return true;
}

bool ConnectionWorker::ReadData(QTcpSocket& socket, char* buffer, qint64 size)
{
    qint64 bytesRemaining = size;
    while (bytesRemaining > 0)
    {
            // check first, or Qt will throw a warning if we try to do this on an already-disconnected-socket
            if (socket.state() != QAbstractSocket::ConnectedState)
            {
                return false;
            }

        qint64 bytesRead = socket.read(buffer, bytesRemaining);
        if (bytesRead == -1)
        {
            return false;
        }
        buffer += bytesRead;
        bytesRemaining -= bytesRead;
        if (bytesRemaining > 0)
        {
            socket.waitForReadyRead();
        }
    }
    return true;
}

bool ConnectionWorker::WriteMessage(QTcpSocket& socket, const AssetProcessor::Message& message)
{
    const qint64 sizeOfHeader = static_cast<qint64>(sizeof(AssetProcessor::MessageHeader));
    AZ_Assert(message.header.size == aznumeric_cast<decltype(message.header.size)>(message.payload.size()), "Message header size does not match payload size");

    // Write header
    if (!WriteData(socket, (char*)&message.header, sizeOfHeader))
    {
        DisconnectSockets();
        return false;
    }

    // write payload
    if (!WriteData(socket, message.payload.data(), message.payload.size()))
    {
        DisconnectSockets();
        return false;
    }

    return true;
}

bool ConnectionWorker::WriteData(QTcpSocket& socket, const char* buffer, qint64 size)
{
    qint64 bytesRemaining = size;
    while (bytesRemaining > 0)
    {
            // check first, or Qt will throw a warning if we try to do this on an already-disconnected-socket
            if (socket.state() != QAbstractSocket::ConnectedState)
            {
                return false;
            }

        qint64 bytesWritten = socket.write(buffer, bytesRemaining);
        if (bytesWritten == -1)
        {
            return false;
        }

        buffer += bytesWritten;
        bytesRemaining -= bytesWritten;
    }

    return true;
}

void ConnectionWorker::EngineSocketHasData()
{
    if (m_terminate)
    {
        return;
    }
    while (m_engineSocket.bytesAvailable() > 0)
    {
        AssetProcessor::Message message;
        if (ReadMessage(m_engineSocket, message))
        {
            Q_EMIT ReceiveMessage(message.header.type, message.header.serial, message.payload);
        }
        else
        {
            break;
        }
    }
}


void ConnectionWorker::SendMessage(unsigned int type, unsigned int serial, QByteArray payload)
{
    AssetProcessor::Message message;
    message.header.type = type;
    message.header.serial = serial;
    message.header.size = payload.size();
    message.payload = payload;
    WriteMessage(m_engineSocket, message);
}

namespace Detail
{
    template <class N>
    bool WriteNegotiation(ConnectionWorker* worker, QTcpSocket& socket, const N& negotiation, unsigned int serial = AzFramework::AssetSystem::NEGOTIATION_SERIAL)
    {
        AssetProcessor::Message message;

        bool packed = AssetProcessor::PackMessage(negotiation, message.payload);
        if (packed)
        {
            message.header.type = negotiation.GetMessageType();
            message.header.serial = serial;
            message.header.size = message.payload.size();
            return worker->WriteMessage(socket, message);
        }
        return false;
    }

    template <class N>
    bool ReadNegotiation(ConnectionWorker* worker, int waitDelay, QTcpSocket& socket, N& negotiation, unsigned int* serial = nullptr)
    {
        if (socket.bytesAvailable() == 0)
        {
            socket.waitForReadyRead(waitDelay);
        }
        AssetProcessor::Message message;
        if (!worker->ReadMessage(socket, message))
        {
            return false;
        }
        if (serial)
        {
            *serial = message.header.serial;
        }
        return AssetProcessor::UnpackMessage(message.payload, negotiation);
    }
}

// Negotiation directly with a game or downstream AssetProcessor:
// if the connection is initiated from this end:
// 1) Send AP Info to downstream engine
// 2) Get downstream engine info
// if there is an incoming connection
// 1) Get downstream engine info
// 2) Send AP Info
bool ConnectionWorker::NegotiateDirect(bool initiate)
{
#if defined(DEBUG_NEGOTIATION)
    AZ_TracePrintf(AssetProcessor::DebugChannel, "ConnectionWorker::NegotiateDirect: %p", this);
#endif
    using Detail::ReadNegotiation;
    using Detail::WriteNegotiation;
    using namespace AzFramework::AssetSystem;

    AZStd::string azBranchToken;
    AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::CalculateBranchTokenForEngineRoot, azBranchToken);
    QString branchToken(azBranchToken.c_str());
    QString projectName = AssetUtilities::ComputeProjectName();
    
    NegotiationMessage myInfo;

    char processId[20];
    azsnprintf(processId, 20, "%lld", QCoreApplication::applicationPid());
    myInfo.m_identifier = "ASSETPROCESSOR";
    myInfo.m_negotiationInfoMap.insert(AZStd::make_pair(NegotiationInfo_ProcessId, AZ::OSString(processId)));
    myInfo.m_negotiationInfoMap.insert(AZStd::make_pair(NegotiationInfo_BranchIndentifier, AZ::OSString(azBranchToken.c_str())));
    myInfo.m_negotiationInfoMap.insert(AZStd::make_pair(NegotiationInfo_ProjectName, AZ::OSString(projectName.toUtf8().constData())));
    NegotiationMessage engineInfo;

    if (initiate)
    {
        if (!WriteNegotiation(this, m_engineSocket, myInfo))
        {
            Q_EMIT ErrorMessage("Unable to send negotiation message");
            QTimer::singleShot(0, this, SLOT(DisconnectSockets()));
            return false;
        }

        if (!ReadNegotiation(this, m_waitDelay, m_engineSocket, engineInfo))
        {
            Q_EMIT ErrorMessage("Unable to read negotiation message");
            QTimer::singleShot(0, this, SLOT(DisconnectSockets()));
            return false;
        }
    }
    else
    {
        unsigned int serial = 0;
#if defined(DEBUG_NEGOTIATION)
        AZ_TracePrintf(AssetProcessor::DebugChannel, "ConnectionWorker::NegotiateDirect: Reading negotiation from engine socket %p", this);
#endif
        if (!ReadNegotiation(this, m_waitDelay, m_engineSocket, engineInfo, &serial))
        {
#if defined(DEBUG_NEGOTIATION)
            AZ_TracePrintf(AssetProcessor::DebugChannel, "ConnectionWorker::NegotiateDirect: no negotation arrived %p", this);
#endif
            Q_EMIT ErrorMessage("Unable to read engine negotiation message");
            QTimer::singleShot(0, this, SLOT(DisconnectSockets()));
            return false;
        }

#if defined(DEBUG_NEGOTIATION)
        AZ_TracePrintf(AssetProcessor::DebugChannel, "ConnectionWorker::NegotiateDirect: writing negotiation to engine socket %p", this);
#endif
        if (!WriteNegotiation(this, m_engineSocket, myInfo, serial))
        {
#if defined(DEBUG_NEGOTIATION)
            AZ_TracePrintf(AssetProcessor::DebugChannel, "ConnectionWorker::NegotiateDirect: no negotation sent %p", this);
#endif
            Q_EMIT ErrorMessage("Unable to send negotiation message");
            QTimer::singleShot(0, this, SLOT(DisconnectSockets()));
            return false;
        }
    }

    // Skip the process Id validation during negotiation if the identifier is UNITTEST
    if (engineInfo.m_identifier != "UNITTEST")
    {
        if (strncmp(engineInfo.m_negotiationInfoMap[NegotiationInfo_ProcessId].c_str(), processId, strlen(processId)) == 0)
        {
            Q_EMIT ErrorMessage("Attempted to negotiate with self");
            QTimer::singleShot(0, this, SLOT(DisconnectSockets()));
            return false;
        }
    }

    if (engineInfo.m_apiVersion != myInfo.m_apiVersion)
    {
        Q_EMIT ErrorMessage("Negotiation Failed.Version Mismatch.");
        QTimer::singleShot(0, this, SLOT(DisconnectSockets()));
        return false;
    }

    QString incomingBranchToken(engineInfo.m_negotiationInfoMap[NegotiationInfo_BranchIndentifier].c_str());
    if (QString::compare(incomingBranchToken, branchToken, Qt::CaseInsensitive) != 0)
    {
        // if we are here it means that the editor/game which is negotiating is running on a different branch
        // note that it could have just read nothing from the engine or a repeat packet, in that case, discard it silently and try again.
        AZ_TracePrintf(AssetProcessor::ConsoleChannel, "ConnectionWorker::NegotiateDirect: branch token mismatch from %s - %p - %s vs %s\n", engineInfo.m_identifier.c_str(), this, incomingBranchToken.toUtf8().data(), branchToken.toUtf8().data());
        AssetProcessor::MessageInfoBus::Broadcast(&AssetProcessor::MessageInfoBus::Events::NegotiationFailed);
        QTimer::singleShot(0, this, SLOT(DisconnectSockets()));
        return false;
    }

    QString incomingProjectName(engineInfo.m_negotiationInfoMap[NegotiationInfo_ProjectName].c_str());
    // Do a case-insensitive compare for the project name because some (case-sensitive) platforms will lower-case the incoming project name
    if(QString::compare(incomingProjectName, projectName, Qt::CaseInsensitive) != 0)
    {
        AZ_TracePrintf(AssetProcessor::ConsoleChannel, "ConnectionWorker::NegotiateDirect: project name mismatch from %s - %p - %s vs %s\n", engineInfo.m_identifier.c_str(), this, incomingProjectName.toUtf8().constData(), projectName.toUtf8().constData());
        AssetProcessor::MessageInfoBus::Broadcast(&AssetProcessor::MessageInfoBus::Events::NegotiationFailed);
        QTimer::singleShot(0, this, SLOT(DisconnectSockets()));
        return false;
    }

    Q_EMIT Identifier(engineInfo.m_identifier.c_str());
    Q_EMIT AssetPlatformsString(engineInfo.m_negotiationInfoMap[NegotiationInfo_Platform].c_str());

#if defined(DEBUG_NEGOTIATION)
    AZ_TracePrintf(AssetProcessor::DebugChannel, "ConnectionWorker::NegotiateDirect: negotation complete %p", this);
#endif

    Q_EMIT ConnectionEstablished(m_engineSocket.peerAddress().toString(), m_engineSocket.peerPort());
    connect(&m_engineSocket, &QTcpSocket::readyRead, this, &ConnectionWorker::EngineSocketHasData);
    // force the socket to evaluate any data recv'd between negotiation and now
    QTimer::singleShot(0, this, SLOT(EngineSocketHasData()));

    return true;
}


// RequestTerminate can be called from anywhere, so we queue the actual
// termination to ensure it happens in the worker's thread
void ConnectionWorker::RequestTerminate()
{
    if (!m_alreadySentTermination)
    {
        m_terminate = true;
        m_alreadySentTermination = true;
        QMetaObject::invokeMethod(this, "TerminateConnection", Qt::BlockingQueuedConnection);
    }
}

void ConnectionWorker::TerminateConnection()
{
    disconnect(&m_engineSocket, &QTcpSocket::stateChanged, this, &ConnectionWorker::EngineSocketStateChanged);
    DisconnectSockets();
    deleteLater();
}

void ConnectionWorker::ConnectSocket(qintptr socketDescriptor)
{
    AZ_Assert(socketDescriptor != -1, "ConectionWorker::ConnectSocket: Supplied socket is invalid");
    if (socketDescriptor != -1)
    {
        // calling setSocketDescriptor will cause it to invoke EngineSocketStateChanged instantly, which we don't want, so disconnect it temporarily.
        disconnect(&m_engineSocket, &QTcpSocket::stateChanged, this, &ConnectionWorker::EngineSocketStateChanged);
        m_engineSocket.setSocketDescriptor(socketDescriptor, QAbstractSocket::ConnectedState, QIODevice::ReadWrite);

        Q_EMIT IsAddressInAllowedList(m_engineSocket.peerAddress(), reinterpret_cast<void*>(this));
    }
}

void ConnectionWorker::AddressIsInAllowedList(void* token, bool result)
{
    if (reinterpret_cast<void*>(this) == token)
    {
        if (result)
        {
            // this address has been approved, connect and proceed
            connect(&m_engineSocket, &QTcpSocket::stateChanged, this, &ConnectionWorker::EngineSocketStateChanged);
            EngineSocketStateChanged(QAbstractSocket::ConnectedState);
        }
        else
        {
            // this address has been rejected, disconnect immediately!!!
            AZ_TracePrintf(AssetProcessor::ConsoleChannel, " A connection attempt was ignored because it is not in the allowed list.  Please consider adding allowed_list=(IP ADDRESS),localhost to the bootstrap.cfg");

            disconnect(&m_engineSocket, &QTcpSocket::readyRead, this, &ConnectionWorker::EngineSocketHasData);

            DisconnectSockets();
        }
    }
}

void ConnectionWorker::ConnectToEngine(QString ipAddress, quint16 port)
{
#if defined(DEBUG_NEGOTIATION)
    AZ_TracePrintf(AssetProcessor::DebugChannel, " ConnectionWorker::ConnectToEngine");
#endif
    m_terminate = false;
    if (m_engineSocket.state() == QAbstractSocket::UnconnectedState)
    {
        m_initiatedConnection = true;
        m_engineSocket.connectToHost(ipAddress, port, QIODevice::ReadWrite);
    }
}



void ConnectionWorker::EngineSocketStateChanged(QAbstractSocket::SocketState socketState)
{
#if defined(DEBUG_NEGOTIATION)
    AZ_TracePrintf(AssetProcessor::DebugChannel, " ConnectionWorker::EngineSocketStateChanged to %i", (int)socketState);
#endif
    if (m_terminate)
    {
        return;
    }

    if (socketState == QAbstractSocket::ConnectedState)
    {
        m_engineSocket.setSocketOption(QAbstractSocket::KeepAliveOption, 1);
        m_engineSocket.setSocketOption(QAbstractSocket::LowDelayOption, 1); //disable nagles algorithm
        m_engineSocketIsConnected = true;
#if defined(DEBUG_NEGOTIATION)
        AZ_TracePrintf(AssetProcessor::DebugChannel, "ConnectionWorker::EngineSocketStateChanged:  %p connected now (%s)", this, m_engineSocketIsConnected ? "True" : "False");

#endif
        QMetaObject::invokeMethod(this, "NegotiateDirect", Qt::QueuedConnection, Q_ARG(bool, m_initiatedConnection));
    }
    else if (socketState == QAbstractSocket::UnconnectedState)
    {
        m_engineSocketIsConnected = false;
#if defined(DEBUG_NEGOTIATION)
        AZ_TracePrintf(AssetProcessor::DebugChannel, "ConnectionWorker::EngineSocketStateChanged:  %p unconnected, now (%s)", this, m_engineSocketIsConnected ? "True" : "False");

#endif
        disconnect(&m_engineSocket, &QTcpSocket::readyRead, 0, 0);
        DisconnectSockets();
    }
}

void ConnectionWorker::DisconnectSockets()
{
#if defined(DEBUG_NEGOTIATION)
    AZ_TracePrintf(AssetProcessor::DebugChannel, " ConnectionWorker::DisconnectSockets");
#endif
    m_engineSocket.abort();
    m_engineSocket.close();
    
    Q_EMIT ConnectionDisconnected();
}


void ConnectionWorker::Reset()
{
    m_terminate = false;
}

bool ConnectionWorker::Terminate()
{
    return m_terminate;
}

QTcpSocket& ConnectionWorker::GetSocket()
{
    return m_engineSocket;
}

bool ConnectionWorker::InitiatedConnection() const
{
    return m_initiatedConnection;
}
} // namespace AssetProcessor
