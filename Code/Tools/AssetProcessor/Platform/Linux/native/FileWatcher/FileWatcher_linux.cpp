/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <native/FileWatcher/FileWatcher.h>

#include <QDirIterator>
#include <QHash>
#include <QMutex>

#include <unistd.h>
#include <sys/inotify.h>


static constexpr int s_handleToFolderMapLockTimeout = 1000;      // 1 sec timeout for obtaining the handle to folder map lock
static constexpr size_t s_iNotifyMaxEntries = 1024 * 16;         // Control the maximum number of entries (from inotify) that can be read at one time
static constexpr size_t s_iNotifyEventSize = sizeof(struct inotify_event);
static constexpr size_t s_iNotifyReadBufferSize = s_iNotifyMaxEntries * s_iNotifyEventSize;

struct FolderRootWatch::PlatformImplementation
{
    PlatformImplementation() = default;

    int                         m_iNotifyHandle = -1;
    QMutex                      m_handleToFolderMapLock;
    QHash<int, QString>         m_handleToFolderMap;

    bool Initialize()
    {
        if (m_iNotifyHandle < 0)
        {
            m_iNotifyHandle = inotify_init();
        }
        return (m_iNotifyHandle >= 0);
    }

    void Finalize()
    {
        if (m_iNotifyHandle >= 0)
        {
            if (!m_handleToFolderMapLock.tryLock(s_handleToFolderMapLockTimeout))
            {
                AZ_Error("FileWatcher", false, "Unable to obtain inotify handle lock on thread");
                return;
            }

            QHashIterator<int, QString> iter(m_handleToFolderMap);
            while (iter.hasNext())
            {
                iter.next();
                int watchHandle = iter.key();
                inotify_rm_watch(m_iNotifyHandle, watchHandle);
            }
            m_handleToFolderMap.clear();
            m_handleToFolderMapLock.unlock();

            ::close(m_iNotifyHandle);
            m_iNotifyHandle = -1;
        }
    }

    void AddWatchFolder(QString folder)
    {
        if (m_iNotifyHandle >= 0)
        {
            // Clean up the path before accepting it as a watch folder
            QString cleanPath = QDir::cleanPath(folder);

            // Add the folder to watch and track it
            int watchHandle = inotify_add_watch(m_iNotifyHandle, 
                                                cleanPath.toUtf8().constData(),
                                                IN_CREATE | IN_CLOSE_WRITE | IN_DELETE | IN_DELETE_SELF | IN_MODIFY | IN_MOVE);
            
            if (!m_handleToFolderMapLock.tryLock(s_handleToFolderMapLockTimeout))
            {
                AZ_Error("FileWatcher", false, "Unable to obtain inotify handle lock on thread");
                return;
            }
            m_handleToFolderMap[watchHandle] = cleanPath;
            m_handleToFolderMapLock.unlock();

            // Add all the subfolders to watch and track them
            QDirIterator dirIter(folder, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);

            while (dirIter.hasNext())
            {
                QString dirName = dirIter.next();
                if (dirName.endsWith("/.") || dirName.endsWith("/.."))
                {
                    continue;
                }
                
                int watchHandle = inotify_add_watch(m_iNotifyHandle, 
                                                    dirName.toUtf8().constData(),
                                                    IN_CREATE | IN_CLOSE_WRITE | IN_DELETE | IN_DELETE_SELF | IN_MODIFY | IN_MOVE);

                if (!m_handleToFolderMapLock.tryLock(s_handleToFolderMapLockTimeout))
                {
                    AZ_Error("FileWatcher", false, "Unable to obtain inotify handle lock on thread");
                    return;
                }
                m_handleToFolderMap[watchHandle] = dirName;
                m_handleToFolderMapLock.unlock();
            }
        }
    }

    void RemoveWatchFolder(int watchHandle)
    {
        if (m_iNotifyHandle >= 0)
        {
            if (!m_handleToFolderMapLock.tryLock(s_handleToFolderMapLockTimeout))
            {
                AZ_Error("FileWatcher", false, "Unable to obtain inotify handle lock on thread");
                return;
            }

            QHash<int, QString>::iterator handleToRemove = m_handleToFolderMap.find(watchHandle);
            if (handleToRemove != m_handleToFolderMap.end())
            {
                inotify_rm_watch(m_iNotifyHandle, watchHandle);
                m_handleToFolderMap.erase(handleToRemove);
            }

            m_handleToFolderMapLock.unlock();
        }
    }
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
    // inotify will be used by linux to monitor file changes within directories under the root folder
    if (!m_platformImpl->Initialize())
    {
        return false;
    }
    m_platformImpl->AddWatchFolder(m_root);

    m_shutdownThreadSignal = false;
    m_thread = std::thread([this]() { WatchFolderLoop(); });
    return true;
}

void FolderRootWatch::Stop()
{
    m_shutdownThreadSignal = true;

    m_platformImpl->Finalize();

    if (m_thread.joinable())
    {
        m_thread.join(); // wait for the thread to finish
        m_thread = std::thread(); //destroy
    }
}


void FolderRootWatch::WatchFolderLoop()
{
    char eventBuffer[s_iNotifyReadBufferSize];
    while (!m_shutdownThreadSignal)
    {
        ssize_t bytesRead = ::read(m_platformImpl->m_iNotifyHandle, eventBuffer, s_iNotifyReadBufferSize);
        if (bytesRead < 0)
        {
            // Break out of the loop when the notify handle was closed (outside of this thread)
            break;
        }
        else if (bytesRead > 0)
        {
            for (size_t index=0; index<bytesRead;)
            {
                struct inotify_event *event = ( struct inotify_event * ) &eventBuffer[ index ];

                if (event->mask & (IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVE ))
                {
                    QString pathStr = QString("%1%2%3").arg(m_platformImpl->m_handleToFolderMap[event->wd], QDir::separator(), event->name);

                    if (event->mask & (IN_CREATE | IN_MOVED_TO)) 
                    {
                        if ( event->mask & IN_ISDIR ) 
                        {
                            // New Directory, add it to the watch
                            m_platformImpl->AddWatchFolder(pathStr);
                        }
                        else 
                        {
                            ProcessNewFileEvent(pathStr);
                        }
                    }
                    else if (event->mask & (IN_DELETE | IN_MOVED_FROM)) 
                    {
                        if (event->mask & IN_ISDIR) 
                        {
                            // Directory Deleted, remove it from the watch
                            m_platformImpl->RemoveWatchFolder(event->wd);
                        }
                        else 
                        {
                            ProcessDeleteFileEvent(pathStr);
                        }
                    }
                    else if ((event->mask & IN_MODIFY) && ((event->mask & IN_ISDIR) != IN_ISDIR))
                    {
                        ProcessModifyFileEvent(pathStr);
                    }
                }
                index += s_iNotifyEventSize + event->len;
            }
        }
    }
}

