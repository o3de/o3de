/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/PlatformIncl.h>

#include "fileServer.h"
#include <native/connection/connection.h>


#if !defined(APPLE) && !defined(LINUX)
#include <io.h>
#endif

#include "native/utilities/assetUtils.h"

#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>

#include <AzFramework/IO/LocalFileIO.h>

using namespace AZ::IO;
using namespace AzFramework::AssetSystem;

//#define VERBOSE_FILE_OPS

//////////////////////////////////////////////////////////////////////////////////////////
FileServer::FileServer(QObject* parent)
    : QObject(parent)
{
    m_realtimeMetrics = true;
    setRealTimeMetrics(false);

    //metrics
    m_numOpenRequests = 0;
    m_numCloseRequests = 0;
    m_numOpened = 0;
    m_numClosed = 0;
    m_numReadRequests = 0;
    m_numWriteRequests = 0;
    m_numTellRequests = 0;
    m_numSeekRequests = 0;
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
}

FileServer::~FileServer()
{
#ifdef REMOTEFILEIO_USE_PROFILING
    g_profiler.DumpTimerDataToOutput();
    g_profiler.DumpTimerDataToFile("../remotefileio_server_profile.txt");
#endif
}

void FileServer::SetSystemRoot(const QDir& systemRoot)
{
    m_systemRoot = systemRoot;
    m_displayRoot = m_systemRoot.absolutePath();
    Q_EMIT RootFolderChanged();
}

void FileServer::setRealTimeMetrics(bool enable)
{
    if (enable)
    {
        m_realtimeMetrics = true;
    }
    else if (m_realtimeMetrics)
    {
        m_realtimeMetrics = false;
        UpdateMetrics();
    }
}

void FileServer::ConnectionAdded(unsigned int connId, Connection* connection)
{
    Q_UNUSED(connection);

    // Connection has not completed negotiation yet, register to be notified
    // when we know what platform is connected and map the @products@ alias then
    connect(connection, &Connection::AssetPlatformChanged, this, [this, connection]()
        {
            auto fileIO = m_fileIOs[connection->ConnectionId()];
            if ((fileIO) && (!connection->AssetPlatforms().isEmpty())) // when someone disconnects, the asset platform may be cleared before disconnect is set.
            {
                QDir projectCacheRoot;
                // Because the platform based aliases below can only be one platform at a time we need to prefer a single platform in case multiple listening platforms
                // exist on the same connection
                QString assetPlatform = connection->AssetPlatforms().first();
                if (!AssetUtilities::ComputeProjectCacheRoot(projectCacheRoot))
                {
                    projectCacheRoot = m_systemRoot;
                }
                else
                {
                    projectCacheRoot = QDir(projectCacheRoot.absoluteFilePath(assetPlatform));
                }

                fileIO->SetAlias("@products@", projectCacheRoot.absolutePath().toUtf8().data());

                if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
                {
                    AZ::IO::Path projectUserPath;
                    settingsRegistry->Get(projectUserPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectUserPath);
                    fileIO->SetAlias("@user@", projectUserPath.c_str());

                    AZ::IO::Path logUserPath = projectUserPath / "log";
                    fileIO->SetAlias("@log@", logUserPath.c_str());
                }


                // note that the cache folder is auto-created only upon first use of VFS.
            }
        });

    std::shared_ptr<AZ::IO::FileIOBase> fileIO = std::make_shared<AZ::IO::LocalFileIO>();
    m_fileIOs[connId] = fileIO;
}

void FileServer::EnsureCacheFolderExists(int connId)
{
    std::shared_ptr<AZ::IO::FileIOBase> fileIO = m_fileIOs[connId];

    if (!fileIO)
    {
        return;
    }
    if (fileIO->GetAlias("@usercache@"))
    {
        // already created.
        return;
    }

    AZ::IO::FixedMaxPath cacheUserPath;
    auto settingsRegistry = AZ::SettingsRegistry::Get();
    if (settingsRegistry->Get(cacheUserPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectUserPath))
    {
        cacheUserPath /= "Cache";
    }

    auto cacheDir = QString::fromUtf8(cacheUserPath.c_str(), aznumeric_cast<int>(cacheUserPath.Native().size()));
    cacheDir = QDir::toNativeSeparators(cacheDir);
    // the Cache-dir is special in that we don't allow sharing of cache dirs for multiple running
    // apps of the same platform at the same time.
    // we do this through the use of lock-files.  Do not use QLockFile as QLockFile can't be shared
    // with instances of lockfiles created through other means (such as the game itself running without VFS)

#if defined(AZ_PLATFORM_WINDOWS)
    // todo:  Future platforms such as MAC will need to use flock or NS to establish locks on folders

    // note that if we DO support file locking we must ALWAYS create and lock the lock file
    int attemptNumber = 0;
    const int maxAttempts = 16;
    QString originalPath = cacheDir;
    while (attemptNumber < maxAttempts)
    {
        cacheDir = originalPath;
        if (attemptNumber != 0)
        {
            cacheDir = QString("%1%2").arg(originalPath).arg(attemptNumber);
        }
        else
        {
            cacheDir = originalPath;
        }

        ++attemptNumber; // do this here so we don't forget

        QDir checkDir(cacheDir);
        checkDir.mkpath(".");

        // if the directory already exists, check for locked file
        QString finalPath = QDir(cacheDir).absoluteFilePath("lockfile.txt");
        // lock the file!
        // cannot use QLockFile, it depends on everyone else which might lock the file also using QLockFile
        // and actually cares about the files contents (your pid)

        // note, the zero here after GENERIC_READ|GENERIC_WRITE indicates no share access at all!
        std::wstring winFriendly = finalPath.toStdWString();

        HANDLE lockHandle = CreateFileW(winFriendly.data(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, 0);
        if (lockHandle != INVALID_HANDLE_VALUE)
        {
            m_locks[connId] = lockHandle;
            break;
        }
    }

    if (attemptNumber >= maxAttempts)
    {
        // do the best we can.
        AZ_Warning("File Server", false, "Unable to establish a cache folder after %i attempts, using %s", attemptNumber, cacheDir.toUtf8().data());
        cacheDir = originalPath;
    }
#endif

    fileIO->SetAlias("@usercache@", cacheDir.toUtf8().data());
}

void FileServer::ConnectionRemoved(unsigned int connId)
{
#if defined(AZ_PLATFORM_WINDOWS)
    auto it = m_locks.find(connId);
    if (it != m_locks.end())
    {
        if (it.value() != INVALID_HANDLE_VALUE)
        {
            CloseHandle(it.value());
        }
        m_locks.erase(it);
    }
#endif
    m_fileIOs.remove(connId);
}

void FileServer::UpdateMetrics()
{
    if (!m_realtimeMetrics)
    {
        //update server metrics
        Q_EMIT NumOpenRequestsChanged();
        Q_EMIT NumCloseRequestsChanged();
        Q_EMIT NumOpenedChanged();
        Q_EMIT NumClosedChanged();
        Q_EMIT NumReadRequestsChanged();
        Q_EMIT NumWriteRequestsChanged();
        Q_EMIT NumSeekRequestsChanged();
        Q_EMIT NumTellRequestsChanged();
        Q_EMIT NumIsReadOnlyRequestsChanged();
        Q_EMIT NumIsDirectoryRequestsChanged();
        Q_EMIT NumSizeRequestsChanged();
        Q_EMIT NumModificationTimeRequestsChanged();
        Q_EMIT NumExistsRequestsChanged();
        Q_EMIT NumFlushRequestsChanged();
        Q_EMIT NumCreatePathRequestsChanged();
        Q_EMIT NumDestroyPathRequestsChanged();
        Q_EMIT NumRemoveRequestsChanged();
        Q_EMIT NumCopyRequestsChanged();
        Q_EMIT NumRenameRequestsChanged();
        Q_EMIT NumFindFileNamesRequestsChanged();
        Q_EMIT BytesReadChanged();
        Q_EMIT BytesWrittenChanged();
        Q_EMIT BytesSentChanged();
        Q_EMIT BytesReceivedChanged();
        Q_EMIT NumOpenFilesChanged();

        //update connections metrics
        Q_EMIT UpdateConnectionMetrics();

        //schedule another update one second from now
        QTimer::singleShot(1000, this, SLOT(UpdateMetrics()));
    }
}

template <class R>
inline void FileServer::Send(unsigned int connId, unsigned int serial, const R& response)
{
    size_t bytesSent;
    AssetProcessor::ConnectionBus::EventResult(bytesSent, connId, &AssetProcessor::ConnectionBus::Events::SendResponse, serial, response);
    m_bytesSent += bytesSent;
    AddBytesSent(connId, bytesSent, m_realtimeMetrics);
}

template <class R>
inline bool FileServer::Recv(unsigned int connId, QByteArray payload, R& request)
{
    bool readFromStream = AZ::Utils::LoadObjectFromBufferInPlace(payload.data(), payload.size(), request);
    AZ_Assert(readFromStream, "FileServer::Recv: Could not deserialize from stream");
    if (readFromStream)
    {
        m_bytesReceived += payload.size();
        AddBytesReceived(connId, payload.size(), m_realtimeMetrics);
        return true;
    }
    return false;
}

void FileServer::ProcessOpenRequest(unsigned int connId, unsigned int, unsigned int serial, QByteArray payload)
{
    EnsureCacheFolderExists(connId);

    m_numOpenRequests++;

    //get the request
    FileOpenRequest request;
    if (!Recv(connId, payload, request))
    {
        AZ_Warning("FileServer", false, "ProcessOpenRequest: unable to read request");
        // send a failure response
        FileOpenResponse response(AZ::IO::InvalidHandle, static_cast<uint32_t>(ResultCode::Error));
        Send(connId, serial, response);
    }

    const char* filePath = request.m_filePath.c_str();
    AZ::IO::OpenMode mode = static_cast<AZ::IO::OpenMode>(request.m_mode);

    AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
    auto fileIO = m_fileIOs[connId];
    RecordFileOp(fileIO.get(), "OPEN", filePath, (((mode& AZ::IO::OpenMode::ModeWrite) != AZ::IO::OpenMode::Invalid) ? "for write" : "for read"));

    AZ::IO::Result res = fileIO->Open(filePath, mode, fileHandle);
    if (res)
    {
        m_numOpenFiles++;
        m_numOpened++;
    }

    uint32_t resultCode = static_cast<uint32_t>(res.GetResultCode());
    FileOpenResponse response(fileHandle, resultCode);
    Send(connId, serial, response);

    AddOpenRequest(connId, m_realtimeMetrics);
    if (res)
    {
        AddOpened(connId, m_realtimeMetrics);
    }

    if (m_realtimeMetrics)
    {
        Q_EMIT BytesSentChanged();
        Q_EMIT NumOpenRequestsChanged();
        Q_EMIT BytesReceivedChanged();
        Q_EMIT NumOpenFilesChanged();
        Q_EMIT NumOpenedChanged();
    }
}

void FileServer::ProcessCloseRequest(unsigned int connId, unsigned int, unsigned int, QByteArray payload)
{
    m_numCloseRequests++;

    //get the request
    FileCloseRequest request;
    if (!Recv(connId, payload, request))
    {
        AZ_Error("FileServer", false, "Failed to deserialize FileCloseRequest for connection %u", connId);
        return;
    }

    AZ::IO::HandleType fileHandle = request.m_fileHandle;
    auto fileIO = m_fileIOs[connId];
    RecordFileOp(fileIO.get(), "CLOSE", fileHandle, nullptr);

    AZ::IO::Result res = fileIO->Close(fileHandle);
    if (res)
    {
        m_numOpenFiles--;
        m_numClosed++;
        AddClosed(connId, m_realtimeMetrics);
    }

    AddCloseRequest(connId, m_realtimeMetrics);

    if (m_realtimeMetrics)
    {
        Q_EMIT NumCloseRequestsChanged();
        Q_EMIT BytesReceivedChanged();
        Q_EMIT NumOpenFilesChanged();
        Q_EMIT NumClosedChanged();
    }
}

void FileServer::ProcessReadRequest(unsigned int connId, unsigned int, unsigned int serial, QByteArray payload)
{
    m_numReadRequests++;

    //get the request
    FileReadRequest request;
    if (!Recv(connId, payload, request))
    {
        FileReadResponse response(static_cast<uint32_t>(ResultCode::Error), nullptr, 0);
        Send(connId, serial, response);
        return;
    }

    AZ::IO::HandleType fileHandle = request.m_fileHandle;
    uint64_t size = request.m_bytesToRead;
    bool failOnFewerRead = request.m_failOnFewerRead;

    FileReadResponse response;
    response.m_data.resize_no_construct(request.m_bytesToRead);
    AZ::u64 bytesRead = 0;
    auto fileIO = m_fileIOs[connId];
    AZStd::string moreInfo = AZStd::string::format("%llu bytes", static_cast<AZ::u64>(size));
    RecordFileOp(fileIO.get(), "READ", fileHandle, moreInfo.c_str());

    AZ::IO::Result res = fileIO->Read(fileHandle, response.m_data.data(), response.m_data.size(), failOnFewerRead, &bytesRead);
    response.m_resultCode = static_cast<uint32_t>(res.GetResultCode());
    m_bytesRead += bytesRead;

    //if the read resulted in any size other than requested resize to the read size
    if (response.m_data.size() != bytesRead)
    {
        response.m_data.resize(bytesRead);
        AddBytesRead(connId, bytesRead, m_realtimeMetrics);
    }

    Send(connId, serial, response);
    AddReadRequest(connId, m_realtimeMetrics);

    if (m_realtimeMetrics)
    {
        Q_EMIT BytesSentChanged();
        Q_EMIT NumReadRequestsChanged();
        Q_EMIT BytesReceivedChanged();
        Q_EMIT BytesReadChanged();
    }
}

void FileServer::ProcessWriteRequest(unsigned int connId, unsigned int, unsigned int serial, QByteArray payload)
{
    m_numWriteRequests++;

    FileWriteRequest request;
    if (!Recv(connId, payload, request))
    {
        FileWriteResponse response(static_cast<uint32_t>(ResultCode::Error), 0);
        Send(connId, serial, response);
        return;
    }

    AZ::IO::HandleType fileHandle = request.m_fileHandle;

    AZ::u64 bytesWritten = 0;
    auto fileIO = m_fileIOs[connId];
    AZStd::string moreInfo = AZStd::string::format("%zu bytes", request.m_data.size());
    RecordFileOp(fileIO.get(), "WRITE", fileHandle, moreInfo.c_str());

    AZ::IO::Result res = fileIO->Write(fileHandle, request.m_data.data(), static_cast<uint64_t>(request.m_data.size()), &bytesWritten);
    uint32_t resultCode = static_cast<uint32_t>(res.GetResultCode());
    if (res)
    {
        m_bytesWritten += bytesWritten;
        AddBytesWritten(connId, bytesWritten, m_realtimeMetrics);
    }

    // 0 serial means the other side doesn't care about the result
    if (serial != 0)
    {
        FileWriteResponse response(resultCode, bytesWritten);
        Send(connId, serial, response);
        AddWriteRequest(connId, m_realtimeMetrics);
    }

    if (m_realtimeMetrics)
    {
        Q_EMIT BytesSentChanged();
        Q_EMIT NumWriteRequestsChanged();
        Q_EMIT BytesReceivedChanged();
        Q_EMIT BytesWrittenChanged();
    }
}

void FileServer::ProcessTellRequest(unsigned int connId, unsigned int, unsigned int serial, QByteArray payload)
{
    m_numTellRequests++;

    FileTellRequest request;
    if (!Recv(connId, payload, request))
    {
        FileTellResponse response(static_cast<AZ::u32>(ResultCode::Error), 0);
        Send(connId, serial, response);
        return;
    }

    AZ::IO::HandleType fileHandle = request.m_fileHandle;
    AZ::u64 offset = 0;
    auto fileIO = m_fileIOs[connId];
    AZStd::string moreInfo = AZStd::string::format("offset: %llu", offset);
    RecordFileOp(fileIO.get(), "TELL", fileHandle, moreInfo.c_str());

    AZ::IO::Result res = fileIO->Tell(fileHandle, offset);

    uint32_t resultCode = static_cast<uint32_t>(res.GetResultCode());
    FileTellResponse response(resultCode, offset);

    Send(connId, serial, response);
    AddTellRequest(connId, m_realtimeMetrics);

    if (m_realtimeMetrics)
    {
        Q_EMIT BytesSentChanged();
        Q_EMIT NumTellRequestsChanged();
        Q_EMIT BytesReceivedChanged();
    }
}

void FileServer::ProcessSeekRequest(unsigned int connId, unsigned int, unsigned int serial, QByteArray payload)
{
    m_numSeekRequests++;

    FileSeekRequest request;
    if (!Recv(connId, payload, request))
    {
        FileSeekResponse response(static_cast<AZ::u32>(ResultCode::Error));
        Send(connId, serial, response);
        return;
    }

    AZ::IO::HandleType fileHandle = request.m_fileHandle;
    AZ::IO::SeekType seekType = static_cast<AZ::IO::SeekType>(request.m_seekMode);
    int64_t offset = request.m_offset;

    auto fileIO = m_fileIOs[connId];
    AZStd::string moreInfo = AZStd::string::format("offset: %lld, mode: %d", static_cast<AZ::s64>(offset), static_cast<AZ::u32>(seekType));
    RecordFileOp(fileIO.get(), "SEEK", fileHandle, moreInfo.c_str());

    AZ::IO::Result res = fileIO->Seek(fileHandle, offset, seekType);

    uint32_t resultCode = static_cast<uint32_t>(res.GetResultCode());
    FileSeekResponse response(resultCode);
    Send(connId, serial, response);
    AddSeekRequest(connId, m_realtimeMetrics);

    if (m_realtimeMetrics)
    {
        Q_EMIT BytesSentChanged();
        Q_EMIT NumSeekRequestsChanged();
        Q_EMIT BytesReceivedChanged();
    }
}


void FileServer::ProcessIsReadOnlyRequest(unsigned int connId, unsigned int, unsigned int serial, QByteArray payload)
{
    EnsureCacheFolderExists(connId);
    m_numIsReadOnlyRequests++;

    FileIsReadOnlyRequest request;
    if (!Recv(connId, payload, request))
    {
        FileIsReadOnlyResponse response(false);
        Send(connId, serial, response);
        return;
    }

    const char* filePath = request.m_filePath.c_str();
    auto fileIO = m_fileIOs[connId];
    RecordFileOp(fileIO.get(), "ISREADONLY", filePath, nullptr);
    bool isReadOnly = fileIO->IsReadOnly(filePath);
    FileIsReadOnlyResponse response(isReadOnly);

    Send(connId, serial, response);
    AddIsReadOnlyRequest(connId, m_realtimeMetrics);

    if (m_realtimeMetrics)
    {
        Q_EMIT BytesSentChanged();
        Q_EMIT NumIsReadOnlyRequestsChanged();
        Q_EMIT BytesReceivedChanged();
    }
}

void FileServer::ProcessIsDirectoryRequest(unsigned int connId, unsigned int, unsigned int serial, QByteArray payload)
{
    EnsureCacheFolderExists(connId);
    m_numIsDirectoryRequests++;

    PathIsDirectoryRequest request;
    if (!Recv(connId, payload, request))
    {
        PathIsDirectoryResponse response(false);
        Send(connId, serial, response);
        return;
    }

    const char* filePath = request.m_path.c_str();
    auto fileIO = m_fileIOs[connId];
    RecordFileOp(fileIO.get(), "ISDIR", filePath, nullptr);

    bool isDirectory = fileIO->IsDirectory(filePath);
    PathIsDirectoryResponse response(isDirectory);
    Send(connId, serial, response);

    AddIsDirectoryRequest(connId, m_realtimeMetrics);

    if (m_realtimeMetrics)
    {
        Q_EMIT BytesSentChanged();
        Q_EMIT NumIsDirectoryRequestsChanged();
        Q_EMIT BytesReceivedChanged();
    }
}

void FileServer::ProcessSizeRequest(unsigned int connId, unsigned int, unsigned int serial, QByteArray payload)
{
    EnsureCacheFolderExists(connId);
    m_numSizeRequests++;

    FileSizeRequest request;
    if (!Recv(connId, payload, request))
    {
        FileSizeResponse response(static_cast<AZ::u32>(ResultCode::Error), 0);
        Send(connId, serial, response);
        return;
    }

    const char* filePath = request.m_filePath.c_str();
    AZ::u64 size = 0;
    auto fileIO = m_fileIOs[connId];
    RecordFileOp(fileIO.get(), "SIZE", filePath, nullptr);

    AZ::IO::Result res = fileIO->Size(filePath, size);

    uint32_t resultCode = static_cast<uint32_t>(res.GetResultCode());
    FileSizeResponse response(resultCode, size);

    Send(connId, serial, response);
    AddSizeRequest(connId, m_realtimeMetrics);

    if (m_realtimeMetrics)
    {
        Q_EMIT BytesSentChanged();
        Q_EMIT NumSizeRequestsChanged();
        Q_EMIT BytesReceivedChanged();
    }
}

void FileServer::ProcessModificationTimeRequest(unsigned int connId, unsigned int, unsigned int serial, QByteArray payload)
{
    EnsureCacheFolderExists(connId);

    m_numModificationTimeRequests++;

    FileModTimeRequest request;
    if (!Recv(connId, payload, request))
    {
        FileModTimeResponse response(0);
        Send(connId, serial, response);
        return;
    }

    const char* filePath = request.m_filePath.c_str();
    auto fileIO = m_fileIOs[connId];
    RecordFileOp(fileIO.get(), "MODTIME", filePath, nullptr);

    uint64_t modTime = fileIO->ModificationTime(filePath);
    FileModTimeResponse response(modTime);

    Send(connId, serial, response);
    AddModificationTimeRequest(connId, m_realtimeMetrics);

    if (m_realtimeMetrics)
    {
        Q_EMIT BytesSentChanged();
        Q_EMIT NumModificationTimeRequestsChanged();
        Q_EMIT BytesReceivedChanged();
    }
}

void FileServer::ProcessExistsRequest(unsigned int connId, unsigned int, unsigned int serial, QByteArray payload)
{
    EnsureCacheFolderExists(connId);
    m_numExistsRequests++;

    FileExistsRequest request;
    if (!Recv(connId, payload, request))
    {
        FileExistsResponse response(false);
        Send(connId, serial, response);
        return;
    }

    const char* filePath = request.m_filePath.c_str();
    auto fileIO = m_fileIOs[connId];
    RecordFileOp(fileIO.get(), "EXISTS", filePath, nullptr);

    bool exists = fileIO->Exists(filePath);
    FileExistsResponse response(exists);

    Send(connId, serial, response);
    AddExistsRequest(connId, m_realtimeMetrics);

    if (m_realtimeMetrics)
    {
        Q_EMIT BytesSentChanged();
        Q_EMIT NumExistsRequestsChanged();
        Q_EMIT BytesReceivedChanged();
    }
}

void FileServer::ProcessFlushRequest(unsigned int connId, unsigned int, unsigned int serial, QByteArray payload)
{
    m_numFlushRequests++;

    FileFlushRequest request;
    if (!Recv(connId, payload, request))
    {
        if (serial != 0)
        {
            FileFlushResponse response(static_cast<AZ::u32>(ResultCode::Error));
            Send(connId, serial, response);
        }
        return;
    }

    AZ::IO::HandleType fileHandle = request.m_fileHandle;
    auto fileIO = m_fileIOs[connId];
    RecordFileOp(fileIO.get(), "FLUSH", fileHandle, nullptr);

    AZ::IO::Result res = fileIO->Flush(fileHandle);
    uint32_t resultCode = static_cast<uint32_t>(res.GetResultCode());
    if (serial != 0)
    {
        FileFlushResponse response(resultCode);
        Send(connId, serial, response);
    }
    AddFlushRequest(connId, m_realtimeMetrics);

    if (m_realtimeMetrics)
    {
        Q_EMIT BytesSentChanged();
        Q_EMIT NumFlushRequestsChanged();
        Q_EMIT BytesReceivedChanged();
    }
}

void FileServer::ProcessCreatePathRequest(unsigned int connId, unsigned int, unsigned int serial, QByteArray payload)
{
    EnsureCacheFolderExists(connId);
    m_numCreatePathRequests++;

    PathCreateRequest request;
    if (!Recv(connId, payload, request))
    {
        PathCreateResponse response(static_cast<AZ::u32>(ResultCode::Error));
        Send(connId, serial, response);
        return;
    }

    const char* filePath = request.m_path.c_str();
    auto fileIO = m_fileIOs[connId];
    RecordFileOp(fileIO.get(), "CREATEPATH", filePath, nullptr);

    AZ::IO::Result res = fileIO->CreatePath(filePath);
    uint32_t resultCode = static_cast<uint32_t>(res.GetResultCode());

    PathCreateResponse response(resultCode);
    Send(connId, serial, response);
    AddCreatePathRequest(connId, m_realtimeMetrics);

    if (m_realtimeMetrics)
    {
        Q_EMIT BytesSentChanged();
        Q_EMIT NumCreatePathRequestsChanged();
        Q_EMIT BytesReceivedChanged();
    }
}

void FileServer::ProcessDestroyPathRequest(unsigned int connId, unsigned int, unsigned int serial, QByteArray payload)
{
    EnsureCacheFolderExists(connId);
    m_numDestroyPathRequests++;

    PathDestroyRequest request;
    if (!Recv(connId, payload, request))
    {
        PathDestroyResponse response(static_cast<AZ::u32>(ResultCode::Error));
        Send(connId, serial, response);
        return;
    }

    const char* filePath = request.m_path.c_str();
    auto fileIO = m_fileIOs[connId];
    RecordFileOp(fileIO.get(), "DESTROYPATH", filePath, nullptr);

    AZ::IO::Result res = fileIO->DestroyPath(filePath);
    uint32_t resultCode = static_cast<uint32_t>(res.GetResultCode());

    PathDestroyResponse response(resultCode);
    Send(connId, serial, response);
    AddDestroyPathRequest(connId, m_realtimeMetrics);

    if (m_realtimeMetrics)
    {
        Q_EMIT BytesSentChanged();
        Q_EMIT NumDestroyPathRequestsChanged();
        Q_EMIT BytesReceivedChanged();
    }
}

void FileServer::ProcessRemoveRequest(unsigned int connId, unsigned int, unsigned int serial, QByteArray payload)
{
    EnsureCacheFolderExists(connId);
    m_numRemoveRequests++;

    FileRemoveRequest request;
    if (!Recv(connId, payload, request))
    {
        FileRemoveResponse response(static_cast<AZ::u32>(ResultCode::Error));
        Send(connId, serial, response);
        return;
    }

    const char* filePath = request.m_filePath.c_str();
    auto fileIO = m_fileIOs[connId];
    RecordFileOp(fileIO.get(), "REMOVE", filePath, nullptr);

    AZ::IO::Result res = fileIO->Remove(filePath);
    uint32_t resultCode = static_cast<uint32_t>(res.GetResultCode());

    FileRemoveResponse response(resultCode);
    Send(connId, serial, response);
    AddRemoveRequest(connId, m_realtimeMetrics);

    if (m_realtimeMetrics)
    {
        Q_EMIT BytesSentChanged();
        Q_EMIT NumRemoveRequestsChanged();
        Q_EMIT BytesReceivedChanged();
    }
}

void FileServer::ProcessCopyRequest(unsigned int connId, unsigned int, unsigned int serial, QByteArray payload)
{
    EnsureCacheFolderExists(connId);
    m_numCopyRequests++;

    FileCopyRequest request;
    if (!Recv(connId, payload, request))
    {
        FileCopyResponse response(static_cast<AZ::u32>(ResultCode::Error));
        Send(connId, serial, response);
        return;
    }

    const char* sourcePath = request.m_srcPath.c_str();
    const char* destinationPath = request.m_destPath.c_str();
    auto fileIO = m_fileIOs[connId];
    RecordFileOp(fileIO.get(), "COPY", sourcePath, destinationPath, nullptr);

    AZ::IO::Result res = fileIO->Copy(sourcePath, destinationPath);
    uint32_t resultCode = static_cast<uint32_t>(res.GetResultCode());

    FileCopyResponse response(resultCode);
    Send(connId, serial, response);
    AddCopyRequest(connId, m_realtimeMetrics);

    if (m_realtimeMetrics)
    {
        Q_EMIT BytesSentChanged();
        Q_EMIT NumCopyRequestsChanged();
        Q_EMIT BytesReceivedChanged();
    }
}

void FileServer::ProcessRenameRequest(unsigned int connId, unsigned int, unsigned int serial, QByteArray payload)
{
    EnsureCacheFolderExists(connId);
    m_numRenameRequests++;

    FileRenameRequest request;
    if (!Recv(connId, payload, request))
    {
        FileRenameResponse response(static_cast<AZ::u32>(ResultCode::Error));
        Send(connId, serial, response);
        return;
    }

    const char* sourcePath = request.m_srcPath.c_str();
    const char* destinationPath = request.m_destPath.c_str();
    auto fileIO = m_fileIOs[connId];
    RecordFileOp(fileIO.get(), "RENAME", sourcePath, destinationPath, nullptr);

    AZ::IO::Result res = fileIO->Rename(sourcePath, destinationPath);
    uint32_t resultCode = static_cast<uint32_t>(res.GetResultCode());

    FileRenameResponse response(resultCode);
    Send(connId, serial, response);
    AddRenameRequest(connId, m_realtimeMetrics);

    if (m_realtimeMetrics)
    {
        Q_EMIT BytesSentChanged();
        Q_EMIT NumRenameRequestsChanged();
        Q_EMIT BytesReceivedChanged();
    }
}

void FileServer::ProcessFindFileNamesRequest(unsigned int connId, unsigned int, unsigned int serial, QByteArray payload)
{
    EnsureCacheFolderExists(connId);
    m_numFindFileNamesRequests++;

    FindFilesRequest request;
    if (!Recv(connId, payload, request))
    {
        FindFilesResponse response(static_cast<AZ::u32>(ResultCode::Error));
        Send(connId, serial, response);
        return;
    }

    const char* filePath = request.m_path.c_str();
    const char* filter = request.m_filter.c_str();

    FindFilesResponse::FileList fileNames;
    auto fileIO = m_fileIOs[connId];
    AZStd::string moreInfo = AZStd::string::format("filter: %s", filter);
    RecordFileOp(fileIO.get(), "FINDFILES", filePath, moreInfo.c_str());

    AZ::IO::Result res = fileIO->FindFiles(filePath, filter,
            [&fileNames](const char* fileName)
            {
                fileNames.push_back(fileName);
                return true;
            });
    uint32_t resultCode = static_cast<uint32_t>(res.GetResultCode());

    FindFilesResponse response(resultCode, fileNames);
    Send(connId, serial, response);
    AddFindFileNamesRequest(connId, m_realtimeMetrics);

    if (m_realtimeMetrics)
    {
        Q_EMIT BytesSentChanged();
        Q_EMIT NumFindFileNamesRequestsChanged();
        Q_EMIT BytesReceivedChanged();
    }
}

void FileServer::ProcessFileTreeRequest(unsigned int connId, unsigned int, unsigned int serial, QByteArray payload)
{
    EnsureCacheFolderExists(connId);

    FileTreeRequest request;
    if (!Recv(connId, payload, request))
    {
        FileTreeResponse response(static_cast<AZ::u32>(ResultCode::Error));
        Send(connId, serial, response);
        return;
    }

    auto fileIO = m_fileIOs[connId];

    FileTreeResponse::FileList files;
    FileTreeResponse::FolderList folders;

    AZStd::vector<AZ::OSString> untestedFolders;
    if (fileIO->IsDirectory("@products@"))
    {
        folders.push_back("@products@");
        untestedFolders.push_back("@products@");
    }
    if (fileIO->IsDirectory("@usercache@"))
    {
        folders.push_back("@usercache@");
        untestedFolders.push_back("@usercache@");
    }
    if (fileIO->IsDirectory("@user@"))
    {
        folders.push_back("@user@");
        untestedFolders.push_back("@user@");
    }
    if (fileIO->IsDirectory("@log@"))
    {
        folders.push_back("@log@");
        untestedFolders.push_back("@log@");
    }

    AZ::IO::Result res = ResultCode::Success;

    while (untestedFolders.size() && res == ResultCode::Success)
    {
        AZ::OSString folderName = untestedFolders.back();
        untestedFolders.pop_back();

        res = fileIO->FindFiles(folderName.c_str(), "*",
            [&](const char* fileName)
            {
                if (fileIO->IsDirectory(fileName))
                {
                    folders.push_back(fileName);
                    untestedFolders.push_back(fileName);
                }
                else
                {
                    files.push_back(fileName);
                }
                return true;
            }
        );
    }

    if (res == ResultCode::Error)
    {
        files.clear();
        folders.clear();
    }

    uint32_t resultCode = static_cast<uint32_t>(res.GetResultCode());

    FileTreeResponse response(resultCode, files, folders);

    Send(connId, serial, response);
}

void FileServer::RecordFileOp(AZ::IO::FileIOBase* fileIO, const char* op, const AZ::IO::HandleType& fileHandle, const char* moreInfo)
{
    (void)fileIO;
    (void)op;
    (void)fileHandle;
    (void)moreInfo;
#ifdef VERBOSE_FILE_OPS
    char filename[MAX_PATH];
    if (fileIO->GetFilename(fileHandle, filename, sizeof(filename)))
    {
        RecordFileOp(fileIO, op, filename, moreInfo);
    }
#endif
}

void FileServer::RecordFileOp(AZ::IO::FileIOBase* fileIO, const char* op, const char* filePath, const char* moreInfo)
{
    (void)fileIO;
    (void)op;
    (void)filePath;
    (void)moreInfo;
#ifdef VERBOSE_FILE_OPS
    AZ_TracePrintf(AssetProcessor::DebugChannel, "FileServer Operation : %s, filePath : %s, moreInfo: %s.\n", op, filePath, moreInfo ? moreInfo : "");
#endif
}

void FileServer::RecordFileOp(AZ::IO::FileIOBase* fileIO, const char* op, const char* sourceFile, const char* destFile, const char* moreInfo)
{
    (void)fileIO;
    (void)op;
    (void)sourceFile;
    (void)destFile;
    (void)moreInfo;
#ifdef VERBOSE_FILE_OPS
    AZ_TracePrintf(AssetProcessor::DebugChannel, "FileServer Operation : %s, sourceFile : %s, destFile : %s, moreInfo: %s.\n", op, sourceFile, destFile, moreInfo ? moreInfo : "");
#endif
}

