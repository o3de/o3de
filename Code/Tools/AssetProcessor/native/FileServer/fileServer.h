/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef FILESERVER_H
#define FILESERVER_H

#if !defined(Q_MOC_RUN)
#include <QByteArray>
#include <QDir>
#include <QString>
#include <QHash>

#include <memory>

// currently these headers are there to provide OS 'HANDLE' of the lock-files
#include <AzCore/PlatformIncl.h>
#endif

namespace AZ
{
    namespace IO
    {
        class FileIOBase;
        typedef uint32_t HandleType;
    }
}
class Connection;

class FileServer
    : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString rootFolder MEMBER m_displayRoot NOTIFY RootFolderChanged)
    Q_PROPERTY(bool realtimeMetrics MEMBER m_realtimeMetrics NOTIFY RealtimeMetricsChanged)

    //metrics
    Q_PROPERTY(qint64 numOpenRequests MEMBER m_numOpenRequests NOTIFY NumOpenRequestsChanged)
    Q_PROPERTY(qint64 numCloseRequests MEMBER m_numCloseRequests NOTIFY NumCloseRequestsChanged)
    Q_PROPERTY(qint64 numOpened MEMBER m_numOpened NOTIFY NumOpenedChanged)
    Q_PROPERTY(qint64 numClosed MEMBER m_numClosed NOTIFY NumClosedChanged)
    Q_PROPERTY(qint64 numReadRequests MEMBER m_numReadRequests NOTIFY NumReadRequestsChanged)
    Q_PROPERTY(qint64 numWriteRequests MEMBER m_numWriteRequests NOTIFY NumWriteRequestsChanged)
    Q_PROPERTY(qint64 numSeekRequests MEMBER m_numSeekRequests NOTIFY NumSeekRequestsChanged)
    Q_PROPERTY(qint64 numTellRequests MEMBER m_numTellRequests NOTIFY NumTellRequestsChanged)
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

Q_SIGNALS:
    void RootFolderChanged();
    void RealtimeMetricsChanged();

    //metrics
    void NumOpenRequestsChanged();
    void NumCloseRequestsChanged();
    void NumOpenedChanged();
    void NumClosedChanged();
    void NumReadRequestsChanged();
    void NumWriteRequestsChanged();
    void NumSeekRequestsChanged();
    void NumTellRequestsChanged();
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

    //per connection metrics
    void AddBytesReceived(unsigned int connId, qint64 add, bool update);
    void AddBytesSent(unsigned int connId, qint64 add, bool update);
    void AddBytesRead(unsigned int connId, qint64 add, bool update);
    void AddBytesWritten(unsigned int connId, qint64 add, bool update);
    void AddOpenRequest(unsigned int connId, bool update);
    void AddCloseRequest(unsigned int connId, bool update);
    void AddOpened(unsigned int connId, bool update);
    void AddClosed(unsigned int connId, bool update);
    void AddReadRequest(unsigned int connId, bool update);
    void AddWriteRequest(unsigned int connId, bool update);
    void AddTellRequest(unsigned int connId, bool update);
    void AddSeekRequest(unsigned int connId, bool update);
    void AddIsReadOnlyRequest(unsigned int connId, bool update);
    void AddIsDirectoryRequest(unsigned int connId, bool update);
    void AddSizeRequest(unsigned int connId, bool update);
    void AddModificationTimeRequest(unsigned int connId, bool update);
    void AddExistsRequest(unsigned int connId, bool update);
    void AddFlushRequest(unsigned int connId, bool update);
    void AddCreatePathRequest(unsigned int connId, bool update);
    void AddDestroyPathRequest(unsigned int connId, bool update);
    void AddRemoveRequest(unsigned int connId, bool update);
    void AddCopyRequest(unsigned int connId, bool update);
    void AddRenameRequest(unsigned int connId, bool update);
    void AddFindFileNamesRequest(unsigned int connId, bool update);

    void UpdateBytesReceived(unsigned int connId);
    void UpdateBytesSent(unsigned int connId);
    void UpdateBytesRead(unsigned int connId);
    void UpdateBytesWritten(unsigned int connId);
    void UpdateOpenRequest(unsigned int connId);
    void UpdateCloseRequest(unsigned int connId);
    void UpdateOpened(unsigned int connId);
    void UpdateClosed(unsigned int connId);
    void UpdateReadRequest(unsigned int connId);
    void UpdateWriteRequest(unsigned int connId);
    void UpdateTellRequest(unsigned int connId);
    void UpdateSeekRequest(unsigned int connId);
    void UpdateIsReadOnlyRequest(unsigned int connId);
    void UpdateIsDirectoryRequest(unsigned int connId);
    void UpdateSizeRequest(unsigned int connId);
    void UpdateModificationTimeRequest(unsigned int connId);
    void UpdateExistsRequest(unsigned int connId);
    void UpdateFlushRequest(unsigned int connId);
    void UpdateCreatePathRequest(unsigned int connId);
    void UpdateDestroyPathRequest(unsigned int connId);
    void UpdateRemoveRequest(unsigned int connId);
    void UpdateCopyRequest(unsigned int connId);
    void UpdateRenameRequest(unsigned int connId);
    void UpdateFindFileNamesRequest(unsigned int connId);

    void UpdateConnectionMetrics();

public:
    explicit FileServer(QObject* parent = 0);
    virtual ~FileServer();

    void SetSystemRoot(const QDir& systemRoot);

    Q_INVOKABLE void setRealTimeMetrics(bool enable);

public Q_SLOTS:
    void ProcessOpenRequest(unsigned int connId, unsigned int type, unsigned int serial, QByteArray payload);
    void ProcessCloseRequest(unsigned int connId, unsigned int type, unsigned int serial, QByteArray payload);
    void ProcessReadRequest(unsigned int connId, unsigned int type, unsigned int serial, QByteArray payload);
    void ProcessWriteRequest(unsigned int connId, unsigned int type, unsigned int serial, QByteArray payload);
    void ProcessTellRequest(unsigned int connId, unsigned int type, unsigned int serial, QByteArray payload);
    void ProcessSeekRequest(unsigned int connId, unsigned int type, unsigned int serial, QByteArray payload);
    void ProcessIsReadOnlyRequest(unsigned int connId, unsigned int type, unsigned int serial, QByteArray payload);
    void ProcessIsDirectoryRequest(unsigned int connId, unsigned int type, unsigned int serial, QByteArray payload);
    void ProcessSizeRequest(unsigned int connId, unsigned int type, unsigned int serial, QByteArray payload);
    void ProcessModificationTimeRequest(unsigned int connId, unsigned int type, unsigned int serial, QByteArray payload);
    void ProcessExistsRequest(unsigned int connId, unsigned int type, unsigned int serial, QByteArray payload);
    void ProcessFlushRequest(unsigned int connId, unsigned int type, unsigned int serial, QByteArray payload);
    void ProcessCreatePathRequest(unsigned int connId, unsigned int type, unsigned int serial, QByteArray payload);
    void ProcessDestroyPathRequest(unsigned int connId, unsigned int type, unsigned int serial, QByteArray payload);
    void ProcessRemoveRequest(unsigned int connId, unsigned int type, unsigned int serial, QByteArray payload);
    void ProcessCopyRequest(unsigned int connId, unsigned int type, unsigned int serial, QByteArray payload);
    void ProcessRenameRequest(unsigned int connId, unsigned int type, unsigned int serial, QByteArray payload);
    void ProcessFindFileNamesRequest(unsigned int connId, unsigned int type, unsigned int serial, QByteArray payload);

    void ProcessFileTreeRequest(unsigned int connId, unsigned int type, unsigned int serial, QByteArray payload);

    void UpdateMetrics();

    void ConnectionAdded(unsigned int connId, Connection* connection);
    void ConnectionRemoved(unsigned int connId);

protected:
    template <class R>
    void Send(unsigned int connId, unsigned int serial, const R& response);

    template <class R>
    bool Recv(unsigned int connId, QByteArray payload, R& request);

    void RecordFileOp(AZ::IO::FileIOBase* fileIO, const char* op, const AZ::IO::HandleType& fileHandle, const char* moreInfo);
    void RecordFileOp(AZ::IO::FileIOBase* fileIO, const char* op, const char* filePath, const char* moreInfo);
    void RecordFileOp(AZ::IO::FileIOBase* fileIO, const char* op, const char* sourceFile, const char* destFile, const char* moreInfo);

    //! This makes sure that the cache folder exists but is conservative, it only should do this if the game actually makes file requests
    //! So we only create a cache folder for VFS-based runs.
    void EnsureCacheFolderExists(int connId);

private:
    //metrics
    qint64 m_numOpenRequests;
    qint64 m_numCloseRequests;
    qint64 m_numOpened;
    qint64 m_numClosed;
    qint64 m_numReadRequests;
    qint64 m_numWriteRequests;
    qint64 m_numTellRequests;
    qint64 m_numSeekRequests;
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

    //root
    QString m_displayRoot;
    QDir m_systemRoot;
    bool m_realtimeMetrics;

    // maps connection ID -> LocalFileIO
    QHash<unsigned int, std::shared_ptr<AZ::IO::FileIOBase> > m_fileIOs;

#if defined(AZ_PLATFORM_WINDOWS)
    QHash<unsigned int, HANDLE> m_locks;
#endif // lockFiles.  do NOT use QLockFile, it won't work if other platforms are locking it, it only works for other users of QLockFile
};

#endif // FILESERVER_H
