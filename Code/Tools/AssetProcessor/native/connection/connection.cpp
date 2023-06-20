/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "connection.h"
#include "native/connection/connectionworker.h"
#include "native/utilities/ByteArrayStream.h"

#include <QSettings>

Connection::Connection(qintptr socketDescriptor, QObject* parent)
    : Connection(false, socketDescriptor, parent)
{
}

Connection::Connection(bool isUserCreatedConnection, qintptr socketDescriptor, QObject* parent)
    : QObject(parent)
    , m_userCreatedConnection(isUserCreatedConnection)
{
    m_runElapsed = true;

    //metrics
    m_numOpenRequests = 0;
    m_numCloseRequests = 0;
    m_numOpened = 0;
    m_numClosed = 0;
    m_numReadRequests = 0;
    m_numWriteRequests = 0;
    m_numTellRequests = 0;
    m_numSeekRequests = 0;
    m_numEofRequests = 0;
    m_numIsReadOnlyRequests = 0;
    m_numIsDirectoryRequests = 0;
    m_numSizeRequests = 0;
    m_numModificationTimeRequests = 0;
    m_numExistsRequests = 0;
    m_numFlushRequests = 0;
    m_numCreatePathRequests = 0;
    m_numDestroyPathRequests = 0;
    m_numRemoveRequests = 0;
    m_numCopyRequests = 0;
    m_numRenameRequests = 0;
    m_numFindFileNamesRequests = 0;
    m_bytesRead = 0;
    m_bytesWritten = 0;
    m_bytesSent = 0;
    m_bytesReceived = 0;
    m_numOpenFiles = 0;

    //connection
    m_identifier = "";//empty
    m_ipAddress = "127.0.0.1";// default is loopback address
    m_port = 22229;//default port number
    m_status = Disconnected;//default status
    m_autoConnect = false;//default status
    m_connectionId = 0; //default
    m_connectionWorker = new AssetProcessor::ConnectionWorker(socketDescriptor);
    m_connectionWorker->moveToThread(&m_connectionWorkerThread);
    m_connectionWorker->GetSocket().moveToThread(&m_connectionWorkerThread);

    connect(this, &Connection::TerminateConnection, m_connectionWorker, &AssetProcessor::ConnectionWorker::RequestTerminate, Qt::DirectConnection);
    connect(this, &Connection::NormalConnectionRequested, m_connectionWorker, &AssetProcessor::ConnectionWorker::ConnectToEngine);

    connect(m_connectionWorker, &AssetProcessor::ConnectionWorker::Identifier, this, [this](QString identifier) {
        // For user created connections, the id is user generated (either because they've manually entered some text
        // this session, or because the id was loaded from a session previously saved where the user entered it).
        // As such, when the connection worker reports a new id from after the connection occurs,
        // we only pay attention to it when it is not a user created connection.
        if (!m_userCreatedConnection)
        {
            SetIdentifier(identifier);
        }
    });

    connect(m_connectionWorker, &AssetProcessor::ConnectionWorker::AssetPlatformsString, this, &Connection::SetAssetPlatformsString);
    connect(m_connectionWorker, &AssetProcessor::ConnectionWorker::ConnectionDisconnected, this, &Connection::OnConnectionDisconnect, Qt::QueuedConnection);
    // the blocking queued connection is here because the worker calls OnConnectionEstablished and then immediately starts emitting messages about
    // data coming in.  We want to immediately establish connectivity this way and we don't want it to proceed with message delivery until then.
    connect(m_connectionWorker, &AssetProcessor::ConnectionWorker::ConnectionEstablished, this, &Connection::OnConnectionEstablished, Qt::BlockingQueuedConnection);
    connect(m_connectionWorker, &AssetProcessor::ConnectionWorker::ErrorMessage, this, &Connection::ErrorMessage);

    connect(m_connectionWorker, &AssetProcessor::ConnectionWorker::IsAddressInAllowedList, this, &Connection::IsAddressInAllowedList);
    connect(this, &Connection::AddressIsInAllowedList, m_connectionWorker, &AssetProcessor::ConnectionWorker::AddressIsInAllowedList);
}

void Connection::Activate(qintptr socketDescriptor)
{
    m_connectionWorkerThread.setObjectName("Connection Worker Thread");
    m_connectionWorkerThread.start();
    //if socketDescriptor is positive it means that it is an incoming connection
    if (socketDescriptor >= 0)
    {
        SetStatus(Connecting);
        // by invoking the ConnectSocket, we cause it to occur in the worker's thread
        QMetaObject::invokeMethod(m_connectionWorker, "ConnectSocket", Q_ARG(qintptr, socketDescriptor));
    }
}

Connection::~Connection()
{
    Q_ASSERT(!m_connectionWorkerThread.isRunning());

    Q_EMIT ConnectionDestroyed(m_connectionId);
}

QString Connection::Identifier() const
{
    return m_identifier;
}

void Connection::SetIdentifier(QString identifier)
{
    if (m_identifier == identifier)
    {
        return;
    }
    m_identifier = identifier;
    Q_EMIT IdentifierChanged();
    Q_EMIT DisplayNameChanged(); // regardless of whether the identifier is empty or not, this always affects the display name.
}

QString Connection::IpAddress() const
{
    return m_ipAddress;
}

QStringList Connection::AssetPlatforms() const
{
    return m_assetPlatforms;
}

QString Connection::AssetPlatformsString() const
{
    return m_assetPlatforms.join(',');
}

void Connection::SetAssetPlatforms(QStringList assetPlatforms)
{
    if (m_assetPlatforms == assetPlatforms)
    {
        return;
    }
    m_assetPlatforms = assetPlatforms;
    Q_EMIT AssetPlatformChanged();
}

QString Connection::DisplayName() const
{
    if (m_identifier.isEmpty())
    {
        return m_ipAddress;
    }

    return m_identifier;
}

QString Connection::Elapsed() const
{
    return m_elapsedDisplay;
}

void Connection::SetIpAddress(QString ipAddress)
{
    if (Status() == Connected)
    {
        AZ_Warning(AssetProcessor::ConsoleChannel, Status() == Connected, "You are not allowed to change the ip address of a connected connection.\n");
        return;
    }

    if (ipAddress == m_ipAddress)
    {
        return;
    }

    m_ipAddress = ipAddress;
    Q_EMIT IpAddressChanged();

    if (m_identifier.isEmpty()) // if the identifier is empty, then the display name is the ip address
    {
        Q_EMIT DisplayNameChanged();
    }
}

int Connection::Port() const
{
    return m_port;
}

void Connection::SetPort(int port)
{
    if (Status() == Connected)
    {
        AZ_Warning(AssetProcessor::ConsoleChannel, Status() == Connected, "You are not allowed to change the port of a connected connection.\n");
        return;
    }

    if (port == m_port)
    {
        return;
    }

    m_port = aznumeric_cast<quint16>(port);
    Q_EMIT PortChanged();
}

Connection::ConnectionStatus Connection::Status() const
{
    return m_status;
}

void Connection::SaveConnection(QSettings& qSettings)
{
    qSettings.setValue("identifier", Identifier());
    qSettings.setValue("ipAddress", IpAddress());
    qSettings.setValue("port", Port());
    qSettings.setValue("assetplatform", AssetPlatforms());
    qSettings.setValue("autoConnect", AutoConnect());
    qSettings.setValue("userConnection", m_userCreatedConnection);
}

void Connection::LoadConnection(QSettings& qSettings)
{
    SetIdentifier(qSettings.value("identifier").toString());
    SetIpAddress(qSettings.value("ipAddress").toString());
    SetPort(qSettings.value("port").toInt());
    SetAssetPlatformsString(qSettings.value("assetplatform").toString());
    SetAutoConnect(qSettings.value("autoConnect").toBool());
    SetStatus(Disconnected);

    m_userCreatedConnection = qSettings.value("userConnection", false).toBool();
}

void Connection::SetStatus(Connection::ConnectionStatus status)
{
    if (status == m_status)
    {
        return;
    }

    m_status = status;
    Q_EMIT StatusChanged(m_connectionId);

    if (status == Connection::Connected)
    {
        AssetProcessor::ConnectionBus::Handler::BusConnect(m_connectionId);
    }
    else if (status == Connection::Disconnected)
    {
        AssetProcessor::ConnectionBus::Handler::BusDisconnect();
    }
}

bool Connection::AutoConnect() const
{
    return m_autoConnect;
}

void Connection::Connect()
{
    m_queuedReconnect = false;
    if (!m_connectionWorker)
    {
        // this can happen if you queued a connect but in the interim, we were deleteLater'd due to removal.
        return;
    }
    m_connectionWorker->Reset();

    Q_EMIT NormalConnectionRequested(m_ipAddress, m_port);
}

void Connection::Disconnect()
{
    Q_EMIT DisconnectConnection(m_connectionId);
}

void Connection::Terminate()
{
    Q_EMIT TerminateConnection();
    if (m_connectionWorkerThread.isRunning())
    {
        m_connectionWorkerThread.quit();
        m_connectionWorkerThread.wait();
    }
    deleteLater();
}

void Connection::SetAutoConnect(bool autoConnect)
{
    if (autoConnect == m_autoConnect)
    {
        return;
    }

    m_autoConnect = autoConnect;
    if (m_autoConnect)
    {
        SetStatus(Connecting);
        Connect();
    }
    else
    {
        SetStatus(Disconnected);
        Disconnect();
    }
    Q_EMIT AutoConnectChanged();
}

void Connection::OnConnectionDisconnect()
{
    if (m_connectionWorker)
    {
        disconnect(this, &Connection::SendMessage, m_connectionWorker, &AssetProcessor::ConnectionWorker::SendMessage);
        disconnect(m_connectionWorker, &AssetProcessor::ConnectionWorker::ReceiveMessage, this, &Connection::ReceiveMessage);
    }

    // For user created connections, the id is user generated (either because they've manually entered some text
    // this session, or because the id was loaded from a session previously saved where the user entered it).
    // As such, when a connection disconnects, we only want to clear the id when the connection was triggered
    // from something other than the user (i.e. like when an automatic connection from Editor or a job worker
    // disconnects).
    if (!m_userCreatedConnection)
    {
        SetIdentifier(QString());
    }

    SetAssetPlatforms(QStringList());
    if (m_autoConnect)
    {
        if (!m_queuedReconnect)
        {
            m_queuedReconnect = true;
            SetStatus(Connecting);
            QTimer::singleShot(500, this, SLOT(Connect()));
        }
    }
    else
    {
        Disconnect();
        SetStatus(Disconnected);
        SetAssetPlatforms(QStringList());

        // if we did not initiate the connection, we should erase it when it disappears.
        if (!InitiatedConnection())
        {
            Terminate();
        }
    }
}

void Connection::OnConnectionEstablished(QString ipAddress, quint16 port)
{
    connect(this, &Connection::SendMessage, m_connectionWorker, &AssetProcessor::ConnectionWorker::SendMessage, Qt::UniqueConnection);
    connect(m_connectionWorker, &AssetProcessor::ConnectionWorker::ReceiveMessage, this, &Connection::ReceiveMessage, Qt::UniqueConnection);

    m_elapsed = 0;
    m_elapsedTimer.start();
    m_runElapsed = true;
    UpdateElapsed();

    SetIpAddress(ipAddress);
    SetPort(port);
    SetStatus(Connected);

    Q_EMIT ConnectionReady(ConnectionId(), AssetPlatforms());
}

void Connection::ReceiveMessage(unsigned int type, unsigned int serial, QByteArray payload)
{
    Q_EMIT DeliverMessage(m_connectionId, type, serial, payload);
}

void Connection::ErrorMessage(QString errorString)
{
    Q_EMIT Error(m_connectionId, errorString);
}

void Connection::UpdateElapsed()
{
    if (m_runElapsed)
    {
        m_elapsed += m_elapsedTimer.restart();
        int seconds = aznumeric_cast<int>(m_elapsed / 1000);
        int hours = seconds / (60 * 60);
        seconds -= hours * (60 * 60);
        int minutes = seconds / 60;
        seconds -= minutes * 60;
        m_elapsedDisplay.clear();
        if (hours < 10)
        {
            m_elapsedDisplay = "0";
        }
        m_elapsedDisplay += QString::number(hours) + ":";
        if (minutes < 10)
        {
            m_elapsedDisplay += "0";
        }
        m_elapsedDisplay += QString::number(minutes) + ":";
        if (seconds < 10)
        {
            m_elapsedDisplay += "0";
        }
        m_elapsedDisplay += QString::number(seconds);

        Q_EMIT ElapsedChanged();
        QTimer::singleShot(1000, this, SLOT(UpdateElapsed()));
    }
}


unsigned int Connection::ConnectionId() const
{
    return m_connectionId;
}

void Connection::SetConnectionId(unsigned int connectionId)
{
    m_connectionId = connectionId;
}

void Connection::SendMessageToWorker(unsigned int type, unsigned int serial, QByteArray payload)
{
    Q_EMIT SendMessage(type, serial, payload);
}

void Connection::AddBytesReceived(qint64 add, bool update)
{
    m_bytesReceived += add;
    if (update)
    {
        Q_EMIT BytesReceivedChanged();
    }
}

void Connection::AddBytesSent(qint64 add, bool update)
{
    m_bytesSent += add;
    if (update)
    {
        Q_EMIT BytesSentChanged();
    }
}

void Connection::AddBytesRead(qint64 add, bool update)
{
    m_bytesRead += add;
    if (update)
    {
        Q_EMIT BytesReadChanged();
    }
}

void Connection::AddBytesWritten(qint64 add, bool update)
{
    m_bytesWritten += add;
    if (update)
    {
        Q_EMIT BytesWrittenChanged();
    }
}

void Connection::AddOpenRequest(bool update)
{
    m_numOpenRequests++;
    if (update)
    {
        Q_EMIT NumOpenRequestsChanged();
    }
}

void Connection::AddCloseRequest(bool update)
{
    m_numCloseRequests++;
    if (update)
    {
        Q_EMIT NumCloseRequestsChanged();
    }
}

void Connection::AddOpened(bool update)
{
    m_numOpened++;
    m_numOpenFiles = m_numOpened - m_numClosed;
    if (update)
    {
        Q_EMIT NumOpenedChanged();
        Q_EMIT NumOpenFilesChanged();
    }
}

void Connection::AddClosed(bool update)
{
    m_numClosed++;
    m_numOpenFiles = m_numOpened - m_numClosed;
    if (update)
    {
        Q_EMIT NumClosedChanged();
        Q_EMIT NumOpenFilesChanged();
    }
}

void Connection::AddReadRequest(bool update)
{
    m_numReadRequests++;
    if (update)
    {
        Q_EMIT NumReadRequestsChanged();
    }
}

void Connection::AddWriteRequest(bool update)
{
    m_numWriteRequests++;
    if (update)
    {
        Q_EMIT NumWriteRequestsChanged();
    }
}

void Connection::AddTellRequest(bool update)
{
    m_numTellRequests++;
    if (update)
    {
        Q_EMIT NumTellRequestsChanged();
    }
}

void Connection::AddSeekRequest(bool update)
{
    m_numSeekRequests++;
    if (update)
    {
        Q_EMIT NumSeekRequestsChanged();
    }
}

void Connection::AddEofRequest(bool update)
{
    m_numEofRequests++;
    if (update)
    {
        Q_EMIT NumEofRequestsChanged();
    }
}

void Connection::AddIsReadOnlyRequest(bool update)
{
    m_numIsReadOnlyRequests++;
    if (update)
    {
        Q_EMIT NumIsReadOnlyRequestsChanged();
    }
}

void Connection::AddIsDirectoryRequest(bool update)
{
    m_numIsDirectoryRequests++;
    if (update)
    {
        Q_EMIT NumIsDirectoryRequestsChanged();
    }
}

void Connection::AddSizeRequest(bool update)
{
    m_numSizeRequests++;
    if (update)
    {
        Q_EMIT NumSizeRequestsChanged();
    }
}

void Connection::AddModificationTimeRequest(bool update)
{
    m_numModificationTimeRequests++;
    if (update)
    {
        Q_EMIT NumModificationTimeRequestsChanged();
    }
}

void Connection::AddExistsRequest(bool update)
{
    m_numExistsRequests++;
    if (update)
    {
        Q_EMIT NumExistsRequestsChanged();
    }
}

void Connection::AddFlushRequest(bool update)
{
    m_numFlushRequests++;
    if (update)
    {
        Q_EMIT NumFlushRequestsChanged();
    }
}

void Connection::AddCreatePathRequest(bool update)
{
    m_numCreatePathRequests++;
    if (update)
    {
        Q_EMIT NumCreatePathRequestsChanged();
    }
}

void Connection::AddDestroyPathRequest(bool update)
{
    m_numDestroyPathRequests++;
    if (update)
    {
        Q_EMIT NumDestroyPathRequestsChanged();
    }
}

void Connection::AddRemoveRequest(bool update)
{
    m_numRemoveRequests++;
    if (update)
    {
        Q_EMIT NumRemoveRequestsChanged();
    }
}

void Connection::AddCopyRequest(bool update)
{
    m_numCopyRequests++;
    if (update)
    {
        Q_EMIT NumCopyRequestsChanged();
    }
}

void Connection::AddRenameRequest(bool update)
{
    m_numRenameRequests++;
    if (update)
    {
        Q_EMIT NumRenameRequestsChanged();
    }
}

void Connection::AddFindFileNamesRequest(bool update)
{
    m_numFindFileNamesRequests++;
    if (update)
    {
        Q_EMIT NumFindFileNamesRequestsChanged();
    }
}

void Connection::UpdateBytesReceived()
{
    Q_EMIT BytesReceivedChanged();
}

void Connection::UpdateBytesSent()
{
    Q_EMIT BytesSentChanged();
}

void Connection::UpdateBytesRead()
{
    Q_EMIT BytesReadChanged();
}

void Connection::UpdateBytesWritten()
{
    Q_EMIT BytesWrittenChanged();
}

void Connection::UpdateOpenRequest()
{
    Q_EMIT NumOpenRequestsChanged();
}

void Connection::UpdateCloseRequest()
{
    Q_EMIT NumCloseRequestsChanged();
}

void Connection::UpdateOpened()
{
    Q_EMIT NumOpenedChanged();
}

void Connection::UpdateClosed()
{
    Q_EMIT NumClosedChanged();
}

void Connection::UpdateReadRequest()
{
    Q_EMIT NumReadRequestsChanged();
}

void Connection::UpdateWriteRequest()
{
    Q_EMIT NumWriteRequestsChanged();
}

void Connection::UpdateTellRequest()
{
    Q_EMIT NumTellRequestsChanged();
}

void Connection::UpdateSeekRequest()
{
    Q_EMIT NumSeekRequestsChanged();
}

void Connection::UpdateEofRequest()
{
    Q_EMIT NumEofRequestsChanged();
}

void Connection::UpdateIsReadOnlyRequest()
{
    Q_EMIT NumIsReadOnlyRequestsChanged();
}

void Connection::UpdateIsDirectoryRequest()
{
    Q_EMIT NumIsDirectoryRequestsChanged();
}

void Connection::UpdateSizeRequest()
{
    Q_EMIT NumSizeRequestsChanged();
}

void Connection::UpdateModificationTimeRequest()
{
    Q_EMIT NumModificationTimeRequestsChanged();
}

void Connection::UpdateExistsRequest()
{
    Q_EMIT NumExistsRequestsChanged();
}

void Connection::UpdateFlushRequest()
{
    Q_EMIT NumFlushRequestsChanged();
}

void Connection::UpdateCreatePathRequest()
{
    Q_EMIT NumCreatePathRequestsChanged();
}

void Connection::UpdateDestroyPathRequest()
{
    Q_EMIT NumDestroyPathRequestsChanged();
}

void Connection::UpdateRemoveRequest()
{
    Q_EMIT NumRemoveRequestsChanged();
}

void Connection::UpdateCopyRequest()
{
    Q_EMIT NumCopyRequestsChanged();
}

void Connection::UpdateRenameRequest()
{
    Q_EMIT NumRenameRequestsChanged();
}

void Connection::UpdateFindFileNamesRequest()
{
    Q_EMIT NumFindFileNamesRequestsChanged();
}

void Connection::UpdateMetrics()
{
    UpdateBytesReceived();
    UpdateBytesSent();
    UpdateBytesRead();
    UpdateBytesWritten();
    UpdateOpenRequest();
    UpdateCloseRequest();
    UpdateOpened();
    UpdateClosed();
    UpdateReadRequest();
    UpdateWriteRequest();
    UpdateTellRequest();
    UpdateSeekRequest();
    UpdateEofRequest();
    UpdateIsReadOnlyRequest();
    UpdateIsDirectoryRequest();
    UpdateSizeRequest();
    UpdateModificationTimeRequest();
    UpdateExistsRequest();
    UpdateFlushRequest();
    UpdateCreatePathRequest();
    UpdateDestroyPathRequest();
    UpdateRemoveRequest();
    UpdateCopyRequest();
    UpdateRenameRequest();
    UpdateFindFileNamesRequest();
}

size_t Connection::Send(unsigned int serial, const AzFramework::AssetSystem::BaseAssetProcessorMessage& message)
{
    QByteArray buffer;
    bool wroteToStream = AssetProcessor::PackMessage(message, buffer);
    AZ_Assert(wroteToStream, "Connection::Send: Could not serialize to stream (type=%u)", message.GetMessageType());
    if (wroteToStream)
    {
        return SendRaw(message.GetMessageType(), serial, buffer);
    }
    return 0;
}

size_t Connection::SendRaw(unsigned int type, unsigned int serial, const QByteArray& data)
{
    SendMessageToWorker(type, serial, data);
    return data.size();
}

size_t Connection::SendPerPlatform(unsigned int serial, const AzFramework::AssetSystem::BaseAssetProcessorMessage& message, const QString& platform)
{
    if (m_assetPlatforms.contains(platform, Qt::CaseInsensitive))
    {
        return Send(serial, message);
    }

    return 0;
}

size_t Connection::SendRawPerPlatform(unsigned int type, unsigned int serial, const QByteArray& data, const QString& platform)
{
    if (m_assetPlatforms.contains(platform, Qt::CaseInsensitive))
    {
        return SendRaw(type, serial, data);
    }

    return 0;
}

AZ::u32 Connection::GetNextSerial()
{
    static AZStd::atomic_uint serial(AzFramework::AssetSystem::DEFAULT_SERIAL);

    AZ::u32 nextSerial = ++serial;

    // Avoid special-case serials
    return (nextSerial & AzFramework::AssetSystem::RESPONSE_SERIAL_FLAG
        || nextSerial == AzFramework::AssetSystem::DEFAULT_SERIAL
        || nextSerial == AzFramework::AssetSystem::NEGOTIATION_SERIAL)
        ? GetNextSerial() // re-roll, we picked a special serial
        : nextSerial;
}

unsigned int Connection::SendRequest(const AzFramework::AssetSystem::BaseAssetProcessorMessage& message, const AssetProcessor::ConnectionBusTraits::ResponseCallback& callback)
{
    AZ::u32 serial = GetNextSerial();

    {
        AZStd::lock_guard<AZStd::mutex> lock(m_responseHandlerMutex);
        m_responseHandlerMap.insert({ serial, callback });
    }

    Send(serial, message);

    return serial;
}

size_t Connection::SendResponse(unsigned int serial, const AzFramework::AssetSystem::BaseAssetProcessorMessage& message)
{
    serial |= AzFramework::AssetSystem::RESPONSE_SERIAL_FLAG; // Set top bit to indicate this is a response
    return Send(serial, message);
}

void Connection::InvokeResponseHandler(AZ::u32 serial, AZ::u32 type, QByteArray data)
{
    AZStd::lock_guard<AZStd::mutex> lock(m_responseHandlerMutex);
    auto itr = m_responseHandlerMap.find(serial);

    if (itr != m_responseHandlerMap.end())
    {
        itr->second(type, data);
        m_responseHandlerMap.erase(itr);
    }
}

void Connection::RemoveResponseHandler(unsigned int serial)
{
    AZStd::lock_guard<AZStd::mutex> lock(m_responseHandlerMutex);
    m_responseHandlerMap.erase(serial);
}

bool Connection::InitiatedConnection() const
{
    if (m_connectionWorker)
    {
        return m_connectionWorker->InitiatedConnection();
    }
    return false;
}

bool Connection::UserCreatedConnection() const
{
    return m_userCreatedConnection;
}

void Connection::SetAssetPlatformsString(QString assetPlatforms)
{
    SetAssetPlatforms(assetPlatforms.split(',', Qt::SkipEmptyParts));
}

