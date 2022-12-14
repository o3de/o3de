/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/tuple.h>
#include <AzCore/std/utils.h>
#include <native/FileWatcher/FileWatcher.h>
#include <native/FileWatcher/FileWatcher_platform.h>
#include <QDir>
#include <QSet>
#include <QString>

bool FileWatcher::PlatformStart()
{
    bool allSucceeded = true;
    for (const auto& [directory, recursive] : m_folderWatchRoots)
    {
        if (QDir(directory).exists())
        {
            allSucceeded &= m_platformImpl->AddWatchFolder(directory, recursive);
        }
    }
    return allSucceeded;
}

bool FileWatcher::PlatformImplementation::AddWatchFolder(QString root, bool recursive)
{
    HandleUniquePtr directoryHandle{::CreateFileW(
        root.toStdWString().data(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        nullptr
    )};

    if (directoryHandle.get() == INVALID_HANDLE_VALUE)
    {
        AZ_Warning("FileWatcher", false, "Failed to start watching %s", root.toUtf8().constData());
        return false;
    }

    // Associate this file handle with our existing io completion port handle
    if (!::CreateIoCompletionPort(directoryHandle.get(), m_ioHandle.get(), /*CompletionKey =*/ static_cast<ULONG_PTR>(PlatformImplementation::EventType::FileRead), 1))
    {
        return false;
    }

    auto id = AZStd::make_unique<OVERLAPPED>();
    auto* idp = id.get();
    const auto& [folderWatch, inserted] = m_folderRootWatches.emplace(AZStd::piecewise_construct, AZStd::forward_as_tuple(idp),
        AZStd::forward_as_tuple(AZStd::move(id), AZStd::move(directoryHandle), root, recursive));

    if (!inserted)
    {
        return false;
    }

    return folderWatch->second.ReadChanges();
}

bool FileWatcher::PlatformImplementation::FolderRootWatch::ReadChanges()
{
    // Register to get directory change notifications for our directory handle
    return ::ReadDirectoryChangesW(
        m_directoryHandle.get(),
        &m_fileNotifyInformationList,
        sizeof(m_fileNotifyInformationList),
        m_recursive,
        FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_FILE_NAME,
        nullptr,
        m_overlapped.get(),
        nullptr
    );
}

void FileWatcher::PlatformStop()
{
    // Send a special signal to the child thread, that is blocked in a GetQueuedCompletionStatus call, with a completion
    // key set to Shutdown. The child thread will stop its processing when it receives this value for the completion key
    PostQueuedCompletionStatus(m_platformImpl->m_ioHandle.get(), 0, /*CompletionKey =*/ static_cast<ULONG_PTR>(PlatformImplementation::EventType::Shutdown), nullptr);
    if (m_thread.joinable())
    {
        m_thread.join(); // wait for the thread to finish
    }
}

void FileWatcher::WatchFolderLoop()
{
    LPOVERLAPPED directoryId = nullptr;
    ULONG_PTR completionKey = 0;

    m_startedSignal = true; // signal that we are no longer going to drop any events.

    while (!m_shutdownThreadSignal)
    {
        DWORD dwByteCount = 0;
        if (::GetQueuedCompletionStatus(m_platformImpl->m_ioHandle.get(), &dwByteCount, &completionKey, &directoryId, INFINITE))
        {
            if (m_shutdownThreadSignal || completionKey == static_cast<ULONG_PTR>(PlatformImplementation::EventType::Shutdown))
            {
                break;
            }
            if (dwByteCount == 0)
            {
                continue;
            }

            const auto foundFolderRoot = m_platformImpl->m_folderRootWatches.find(directoryId);
            if (foundFolderRoot == end(m_platformImpl->m_folderRootWatches))
            {
                continue;
            }

            PlatformImplementation::FolderRootWatch& folderRoot = foundFolderRoot->second;
            // Initialize offset to 1 to ensure that the first iteration is always processed
            DWORD offset = 1;
            for (
                const FILE_NOTIFY_INFORMATION* pFileNotifyInformation = reinterpret_cast<const FILE_NOTIFY_INFORMATION*>(&folderRoot.m_fileNotifyInformationList);
                offset;
                pFileNotifyInformation = reinterpret_cast<const FILE_NOTIFY_INFORMATION*>(reinterpret_cast<const char*>(pFileNotifyInformation) + offset)
            ){
                const QString file = QDir::toNativeSeparators(QDir(folderRoot.m_directoryRoot)
                    .filePath(QString::fromWCharArray(pFileNotifyInformation->FileName, pFileNotifyInformation->FileNameLength / 2)));

                switch (pFileNotifyInformation->Action)
                {
                case FILE_ACTION_ADDED:
                case FILE_ACTION_RENAMED_NEW_NAME:
                    rawFileAdded(file, {});
                    break;
                case FILE_ACTION_REMOVED:
                case FILE_ACTION_RENAMED_OLD_NAME:
                    rawFileRemoved(file, {});
                    break;
                case FILE_ACTION_MODIFIED:
                    // note that changing a files size, attributes, data, modified time, create time, all count as individual modifies
                    // and may send multiple events asynchronously.
                    rawFileModified(file, {});
                    break;
                }

                offset = pFileNotifyInformation->NextEntryOffset;
            }

            folderRoot.ReadChanges();
        }
    }
}
