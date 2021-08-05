/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <native/FileWatcher/FileWatcher.h>

#include <AzCore/PlatformIncl.h>

struct FolderRootWatch::PlatformImplementation
{
    PlatformImplementation() : m_directoryHandle(nullptr), m_ioHandle(nullptr) { }
    HANDLE m_directoryHandle;
    HANDLE m_ioHandle;
};

//////////////////////////////////////////////////////////////////////////////
/// FolderWatchRoot
FolderRootWatch::FolderRootWatch(const QString rootFolder)
    : m_root(rootFolder)
    , m_shutdownThreadSignal(false)
    , m_fileWatcher(nullptr)
    , m_platformImpl(new PlatformImplementation())
{
}

FolderRootWatch::~FolderRootWatch()
{
    // Destructor is required in here since this file contains the definition of struct PlatformImplementation
    Stop();

    delete m_platformImpl;
}

bool FolderRootWatch::Start()
{
    m_platformImpl->m_directoryHandle = ::CreateFileW(m_root.toStdWString().data(), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);

    if (m_platformImpl->m_directoryHandle != INVALID_HANDLE_VALUE)
    {
        m_platformImpl->m_ioHandle = ::CreateIoCompletionPort(m_platformImpl->m_directoryHandle, nullptr, 1, 0);
        if (m_platformImpl->m_ioHandle != INVALID_HANDLE_VALUE)
        {
            m_shutdownThreadSignal = false;
            m_thread = std::thread(std::bind(&FolderRootWatch::WatchFolderLoop, this));
            return true;
        }
    }
    return false;
}

void FolderRootWatch::Stop()
{
    m_shutdownThreadSignal = true;
    CloseHandle(m_platformImpl->m_ioHandle);
    m_platformImpl->m_ioHandle = nullptr;

    if (m_thread.joinable())
    {
        m_thread.join(); // wait for the thread to finish
        m_thread = std::thread(); //destroy
    }
    CloseHandle(m_platformImpl->m_directoryHandle);
    m_platformImpl->m_directoryHandle = nullptr;
}

void FolderRootWatch::WatchFolderLoop()
{
    FILE_NOTIFY_INFORMATION aFileNotifyInformationList[50000];
    QString path;
    OVERLAPPED aOverlapped;
    LPOVERLAPPED pOverlapped;
    DWORD dwByteCount;
    ULONG_PTR ulKey;

    while (!m_shutdownThreadSignal)
    {
        ::memset(aFileNotifyInformationList, 0, sizeof(aFileNotifyInformationList));
        ::memset(&aOverlapped, 0, sizeof(aOverlapped));

        if (::ReadDirectoryChangesW(m_platformImpl->m_directoryHandle, aFileNotifyInformationList, sizeof(aFileNotifyInformationList), true, FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_FILE_NAME, nullptr, &aOverlapped, nullptr))
        {
            //wait for up to a second for I/O to signal
            dwByteCount = 0;
            if (::GetQueuedCompletionStatus(m_platformImpl->m_ioHandle, &dwByteCount, &ulKey, &pOverlapped, INFINITE))
            {
                //if we are signaled to shutdown bypass
                if (!m_shutdownThreadSignal && ulKey)
                {
                    if (dwByteCount)
                    {
                        int offset = 0;
                        FILE_NOTIFY_INFORMATION* pFileNotifyInformation = aFileNotifyInformationList;
                        do
                        {
                            pFileNotifyInformation = (FILE_NOTIFY_INFORMATION*)((char*)pFileNotifyInformation + offset);

                            path.clear();
                            path.append(m_root);
                            path.append(QString::fromWCharArray(pFileNotifyInformation->FileName, pFileNotifyInformation->FileNameLength / 2));

                            QString file = QDir::toNativeSeparators(QDir::cleanPath(path));

                            switch (pFileNotifyInformation->Action)
                            {
                            case FILE_ACTION_ADDED:
                            case FILE_ACTION_RENAMED_NEW_NAME:
                                ProcessNewFileEvent(file);
                                break;
                            case FILE_ACTION_REMOVED:
                            case FILE_ACTION_RENAMED_OLD_NAME:
                                ProcessDeleteFileEvent(file);
                                break;
                            case FILE_ACTION_MODIFIED:
                                ProcessModifyFileEvent(file);
                                break;
                            }

                            offset = pFileNotifyInformation->NextEntryOffset;
                        } while (offset);
                    }
                }
            }
        }
    }
}

