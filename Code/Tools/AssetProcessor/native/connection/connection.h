/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef CONNECTION_H
#define CONNECTION_H

#if !defined(Q_MOC_RUN)
#include <QThread>
#include <QElapsedTimer>
#include "native/utilities/AssetUtilEBusHelper.h"
#include <QHostAddress>

#include <QTimer>
#include <QString>
#include <QPointer>
#endif

class QSettings;

namespace AssetProcessor
{
    class ConnectionWorker;
    class PlatformConfiguration;
}

#undef SendMessage

/** This Class contains all the information related to a single connecton
 */

class Connection
    : public QObject
    , public AssetProcessor::ConnectionBus::Handler
{
    Q_OBJECT
    Q_PROPERTY(QString identifier READ Identifier WRITE SetIdentifier NOTIFY IdentifierChanged)
    Q_PROPERTY(QString ipAddress READ IpAddress WRITE SetIpAddress NOTIFY IpAddressChanged)
    Q_PROPERTY(int port READ Port WRITE SetPort NOTIFY PortChanged)
    Q_PROPERTY(ConnectionStatus status READ Status NOTIFY StatusChanged)
    Q_PROPERTY(QStringList assetPlatform READ AssetPlatforms WRITE SetAssetPlatforms NOTIFY AssetPlatformChanged)
    Q_PROPERTY(QString assetPlatformsString READ AssetPlatformsString WRITE SetAssetPlatformsString)
    Q_PROPERTY(bool autoConnect READ AutoConnect WRITE SetAutoConnect NOTIFY AutoConnectChanged)
    Q_PROPERTY(QString displayName READ DisplayName NOTIFY DisplayNameChanged)
    Q_PROPERTY(QString elapsed READ Elapsed NOTIFY ElapsedChanged)

    //metrics
    Q_PROPERTY(qint64 numOpenRequests MEMBER m_numOpenRequests NOTIFY NumOpenRequestsChanged)
    Q_PROPERTY(qint64 numCloseRequests MEMBER m_numCloseRequests NOTIFY NumCloseRequestsChanged)
    Q_PROPERTY(qint64 numOpened MEMBER m_numOpened NOTIFY NumOpenedChanged)
    Q_PROPERTY(qint64 numClosed MEMBER m_numClosed NOTIFY NumClosedChanged)
    Q_PROPERTY(qint64 numReadRequests MEMBER m_numReadRequests NOTIFY NumReadRequestsChanged)
    Q_PROPERTY(qint64 numWriteRequests MEMBER m_numWriteRequests NOTIFY NumWriteRequestsChanged)
    Q_PROPERTY(qint64 numSeekRequests MEMBER m_numSeekRequests NOTIFY NumSeekRequestsChanged)
    Q_PROPERTY(qint64 numTellRequests MEMBER m_numTellRequests NOTIFY NumTellRequestsChanged)
    Q_PROPERTY(qint64 numEofRequests MEMBER m_numEofRequests NOTIFY NumEofRequestsChanged)
    Q_PROPERTY(qint64 numIsReadOnlyRequests MEMBER m_numIsReadOnlyRequests NOTIFY NumIsReadOnlyRequestsChanged)
    Q_PROPERTY(qint64 numIsDirectoryRequests MEMBER m_numIsDirectoryRequests NOTIFY NumIsDirectoryRequestsChanged)
    Q_PROPERTY(qint64 numSizeRequests MEMBER m_numSizeRequests NOTIFY NumSizeRequestsChanged)
    Q_PROPERTY(qint64 numModificationTimeRequests MEMBER m_numModificationTimeRequests NOTIFY NumModificationTimeRequestsChanged)
    Q_PROPERTY(qint64 numExistsRequests MEMBER m_numExistsRequests NOTIFY NumExistsRequestsChanged)
    Q_PROPERTY(qint64 numFlushRequests MEMBER m_numFlushRequests NOTIFY NumFlushRequestsChanged)
    Q_PROPERTY(qint64 numCreatePathRequests MEMBER m_numCreatePathRequests NOTIFY NumCreatePathRequestsChanged)
    Q_PROPERTY(qint64 numDestroyPathRequests MEMBER m_numDestroyPathRequests NOTIFY NumDestroyPathRequestsChanged)
    Q_PROPERTY(qint64 numRemoveRequests MEMBER m_numRemoveRequests NOTIFY NumRemoveRequestsChanged)
    Q_PROPERTY(qint64 numCopyRequests MEMBER m_numCopyRequests NOTIFY NumCopyRequestsChanged)
    Q_PROPERTY(qint64 numRenameRequests MEMBER m_numRenameRequests NOTIFY NumRenameRequestsChanged)
    Q_PROPERTY(qint64 numFindFileNamesRequests MEMBER m_numFindFileNamesRequests NOTIFY NumFindFileNamesRequestsChanged)
    Q_PROPERTY(qint64 bytesRead MEMBER m_bytesRead NOTIFY BytesReadChanged)
    Q_PROPERTY(qint64 bytesWritten MEMBER m_bytesWritten NOTIFY BytesWrittenChanged)
    Q_PROPERTY(qint64 bytesSent MEMBER m_bytesSent NOTIFY BytesSentChanged)
    Q_PROPERTY(qint64 bytesReceived MEMBER m_bytesReceived NOTIFY BytesReceivedChanged)
    Q_PROPERTY(qint64 numOpenFiles MEMBER m_numOpenFiles NOTIFY NumOpenFilesChanged)

public:
    explicit Connection(qintptr socketDescriptor = -1, QObject* parent = 0);
    explicit Connection(bool isUserCreatedConnection, qintptr socketDescriptor = -1, QObject* parent = 0);
    virtual ~Connection();

    enum ConnectionStatus
    {
        Disconnected, Connected, Connecting
    };

    Q_ENUMS(ConnectionStatus)

    void Activate(qintptr socketDescriptor);

    QString Identifier() const;
    QString IpAddress() const;
    int Port() const;
    ConnectionStatus Status() const;
    QStringList AssetPlatforms() const;
    QString AssetPlatformsString() const;
    void SaveConnection(QSettings& qSettings);
    void LoadConnection(QSettings& qSettings);
    bool AutoConnect() const;
    QString DisplayName() const;
    QString Elapsed() const;

    bool InitiatedConnection() const;

    bool UserCreatedConnection() const;

    void Disconnect();
    unsigned int ConnectionId() const;
    void SetConnectionId(unsigned int ConnectionId);
    void Terminate();
    void SendMessageToWorker(unsigned int type, unsigned int serial, QByteArray payload);

    void AddBytesReceived(qint64 add, bool update);
    void AddBytesSent(qint64 add, bool update);
    void AddBytesRead(qint64 add, bool update);
    void AddBytesWritten(qint64 add, bool update);
    void AddOpenRequest(bool update);
    void AddCloseRequest(bool update);
    void AddOpened(bool update);
    void AddClosed(bool update);
    void AddReadRequest(bool update);
    void AddWriteRequest(bool update);
    void AddTellRequest(bool update);
    void AddSeekRequest(bool update);
    void AddEofRequest(bool update);
    void AddIsReadOnlyRequest(bool update);
    void AddIsDirectoryRequest(bool update);
    void AddSizeRequest(bool update);
    void AddModificationTimeRequest(bool update);
    void AddExistsRequest(bool update);
    void AddFlushRequest(bool update);
    void AddCreatePathRequest(bool update);
    void AddDestroyPathRequest(bool update);
    void AddRemoveRequest(bool update);
    void AddCopyRequest(bool update);
    void AddRenameRequest(bool update);
    void AddFindFileNamesRequest(bool update);

    void UpdateBytesReceived();
    void UpdateBytesSent();
    void UpdateBytesRead();
    void UpdateBytesWritten();
    void UpdateOpenRequest();
    void UpdateCloseRequest();
    void UpdateOpened();
    void UpdateClosed();
    void UpdateReadRequest();
    void UpdateWriteRequest();
    void UpdateTellRequest();
    void UpdateSeekRequest();
    void UpdateEofRequest();
    void UpdateIsReadOnlyRequest();
    void UpdateIsDirectoryRequest();
    void UpdateSizeRequest();
    void UpdateModificationTimeRequest();
    void UpdateExistsRequest();
    void UpdateFlushRequest();
    void UpdateCreatePathRequest();
    void UpdateDestroyPathRequest();
    void UpdateRemoveRequest();
    void UpdateCopyRequest();
    void UpdateRenameRequest();
    void UpdateFindFileNamesRequest();

    void UpdateMetrics();

    // AssetProcessor::ConnectionBus interface
    size_t Send(unsigned int serial, const AzFramework::AssetSystem::BaseAssetProcessorMessage& message) override;
    size_t SendRaw(unsigned int type, unsigned int serial, const QByteArray& data) override;
    size_t SendPerPlatform(unsigned int serial, const AzFramework::AssetSystem::BaseAssetProcessorMessage& message, const QString& platform) override;
    size_t SendRawPerPlatform(unsigned int type, unsigned int serial, const QByteArray& data, const QString& platform) override;
    //! callback runs on the main thread, be sure to keep the work to an absolute minimum
    unsigned int SendRequest(const AzFramework::AssetSystem::BaseAssetProcessorMessage& message, const AssetProcessor::ConnectionBusTraits::ResponseCallback& callback) override;
    size_t SendResponse(unsigned int serial, const AzFramework::AssetSystem::BaseAssetProcessorMessage& message) override;
    void RemoveResponseHandler(unsigned int serial) override;

    void InvokeResponseHandler(AZ::u32 serial, AZ::u32 type, QByteArray data);

Q_SIGNALS:
    void IdentifierChanged();
    void IpAddressChanged();
    void PortChanged();
    void StatusChanged(unsigned int connId);
    void AssetPlatformChanged();
    void AutoConnectChanged();
    void DisplayNameChanged();
    void ElapsedChanged();
    void NormalConnectionRequested(QString IpAddress, quint16 Port);
    void ConnectionReady(unsigned int connId, QStringList platforms);

    void connectionEnded();
    void TerminateConnection();
    void SendMessage(unsigned int type, unsigned int serial, QByteArray payload);
    void DeliverMessage(unsigned int connId, unsigned int type, unsigned int serial, QByteArray payload);
    void ConnectionDestroyed(unsigned int connId);
    void DisconnectConnection(unsigned int connId);
    void AddGameMessageToOutgoingQueue();
    void Error(unsigned int connId, QString errorString);

    // the token is just any identifier to identify a particular connection, potentially from the same host.
    // the response (AddressIsInAllowedList) will have the same token as was sent.
    void IsAddressInAllowedList(QHostAddress hostAddress, void* token);
    void AddressIsInAllowedList(void* token, bool result);

    //metrics
    void NumOpenRequestsChanged();
    void NumCloseRequestsChanged();
    void NumOpenedChanged();
    void NumClosedChanged();
    void NumReadRequestsChanged();
    void NumWriteRequestsChanged();
    void NumSeekRequestsChanged();
    void NumTellRequestsChanged();
    void NumEofRequestsChanged();
    void NumIsReadOnlyRequestsChanged();
    void NumIsDirectoryRequestsChanged();
    void NumSizeRequestsChanged();
    void NumModificationTimeRequestsChanged();
    void NumExistsRequestsChanged();
    void NumFlushRequestsChanged();
    void NumCreatePathRequestsChanged();
    void NumDestroyPathRequestsChanged();
    void NumRemoveRequestsChanged();
    void NumCopyRequestsChanged();
    void NumRenameRequestsChanged();
    void NumFindFileNamesRequestsChanged();
    void BytesReadChanged();
    void BytesWrittenChanged();
    void BytesSentChanged();
    void BytesReceivedChanged();
    void NumOpenFilesChanged();

public Q_SLOTS:
    void SetIdentifier(QString Identifier);
    void SetIpAddress(QString IpAddress);
    void SetPort(int Port);
    void SetStatus(ConnectionStatus Status);
    void SetAssetPlatforms(QStringList assetPlatform);
    void SetAssetPlatformsString(QString assetPlatforms);
    void SetAutoConnect(bool AutoConnect);
    void OnConnectionDisconnect();
    void OnConnectionEstablished(QString ipAddress, quint16 port);
    void ReceiveMessage(unsigned int type, unsigned int serial, QByteArray payload);
    void ErrorMessage(QString negotiateFailure);
    void UpdateElapsed();
    void Connect();

private:

    AZ::u32 GetNextSerial();

    unsigned int m_connectionId;
    QString m_identifier;
    QString m_ipAddress;
    quint16 m_port;
    ConnectionStatus m_status;
    QStringList m_assetPlatforms;
    bool m_autoConnect;
    QThread m_connectionWorkerThread;
    QPointer<AssetProcessor::ConnectionWorker> m_connectionWorker;
    bool m_runElapsed;
    QElapsedTimer m_elapsedTimer;
    qint64 m_elapsed;
    QString m_elapsedDisplay;
    bool m_queuedReconnect = false;
    bool m_userCreatedConnection = false;

    AZStd::mutex m_responseHandlerMutex;
    AZStd::unordered_map<AZ::u32, AssetProcessor::ConnectionBusTraits::ResponseCallback> m_responseHandlerMap;

    //metrics
    qint64 m_numOpenRequests;
    qint64 m_numCloseRequests;
    qint64 m_numOpened;
    qint64 m_numClosed;
    qint64 m_numReadRequests;
    qint64 m_numWriteRequests;
    qint64 m_numTellRequests;
    qint64 m_numSeekRequests;
    qint64 m_numEofRequests;
    qint64 m_numIsReadOnlyRequests;
    qint64 m_numIsDirectoryRequests;
    qint64 m_numSizeRequests;
    qint64 m_numModificationTimeRequests;
    qint64 m_numExistsRequests;
    qint64 m_numFlushRequests;
    qint64 m_numCreatePathRequests;
    qint64 m_numDestroyPathRequests;
    qint64 m_numRemoveRequests;
    qint64 m_numCopyRequests;
    qint64 m_numRenameRequests;
    qint64 m_numFindFileNamesRequests;
    qint64 m_bytesRead;
    qint64 m_bytesWritten;
    qint64 m_bytesSent;
    qint64 m_bytesReceived;
    qint64 m_numOpenFiles;
    Q_DISABLE_COPY(Connection)
};


#endif // CONNECTION_H
