/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/string/fixed_string.h>
#include <native/FileWatcher/FileWatcher.h>
#include <native/FileWatcher/FileWatcher_platform.h>

#include <QDirIterator>
#include <QHash>
#include <QMutex>

#include <unistd.h>
#include <sys/inotify.h>


static constexpr size_t s_inotifyMaxEntries = 1024 * 16;         // Control the maximum number of entries (from inotify) that can be read at one time
static constexpr size_t s_inotifyEventSize = sizeof(struct inotify_event);
static constexpr size_t s_inotifyReadBufferSize = s_inotifyMaxEntries * s_inotifyEventSize;

bool FileWatcher::PlatformImplementation::Initialize()
{
    if (m_inotifyHandle < 0)
    {
        // The CLOEXEC flag prevents the inotify watchers from copying on fork/exec
        m_inotifyHandle = inotify_init1(IN_CLOEXEC);

        [[maybe_unused]] const auto err = errno;
        [[maybe_unused]] AZStd::fixed_string<255> errorString;
        AZ_Warning("FileWatcher", (m_inotifyHandle >= 0), "Unable to initialize inotify, file monitoring will not be available: %s\n", strerror_r(err, errorString.data(), errorString.capacity()));
    }
    return (m_inotifyHandle >= 0);
}

void FileWatcher::PlatformImplementation::Finalize()
{
    if (m_inotifyHandle < 0)
    {
        return;
    }

    {
        QMutexLocker lock{&m_handleToFolderMapLock};
        for (const auto& watchHandle : m_handleToFolderMap.keys())
        {
            inotify_rm_watch(m_inotifyHandle, watchHandle);
        }
        m_handleToFolderMap.clear();
    }

    ::close(m_inotifyHandle);
    m_inotifyHandle = -1;
}

void FileWatcher::PlatformImplementation::AddWatchFolder(QString folder, bool recursive)
{
    if (m_inotifyHandle < 0)
    {
        return;
    }

    // Clean up the path before accepting it as a watch folder
    QString cleanPath = QDir::cleanPath(folder);

    // Add the folder to watch and track it
    int watchHandle = inotify_add_watch(m_inotifyHandle,
                                        cleanPath.toUtf8().constData(),
                                        IN_CREATE | IN_CLOSE_WRITE | IN_DELETE | IN_DELETE_SELF | IN_MODIFY | IN_MOVE);

    if (watchHandle < 0)
    {
        [[maybe_unused]] const auto err = errno;
        [[maybe_unused]] AZStd::fixed_string<255> errorString;
        AZ_Warning("FileWatcher", false, "inotify_add_watch failed for path %s: %s", cleanPath.toUtf8().constData(), strerror_r(err, errorString.data(), errorString.capacity()));
        return;
    }
    {
        QMutexLocker lock{&m_handleToFolderMapLock};
        m_handleToFolderMap[watchHandle] = cleanPath;
    }

    // Add all the contents (files and directories) to watch and track them
    QDirIterator dirIter(folder, QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files, (recursive ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags) | QDirIterator::FollowSymlinks);

    while (dirIter.hasNext())
    {
        QString dirName = dirIter.next();

        watchHandle = inotify_add_watch(m_inotifyHandle,
                                            dirName.toUtf8().constData(),
                                            IN_CREATE | IN_CLOSE_WRITE | IN_DELETE | IN_DELETE_SELF | IN_MODIFY | IN_MOVE);
        if (watchHandle < 0)
        {
            [[maybe_unused]] const auto err = errno;
            [[maybe_unused]] AZStd::fixed_string<255> errorString;
            AZ_Warning("FileWatcher", false, "inotify_add_watch failed for path %s: %s", dirName.toUtf8().constData(), strerror_r(err, errorString.data(), errorString.capacity()));
            return;
        }

        QMutexLocker lock{&m_handleToFolderMapLock};
        m_handleToFolderMap[watchHandle] = dirName;
    }
}

void FileWatcher::PlatformImplementation::RemoveWatchFolder(int watchHandle)
{
    if (m_inotifyHandle < 0)
    {
        return;
    }

    QMutexLocker lock{&m_handleToFolderMapLock};
    if (m_handleToFolderMap.remove(watchHandle))
    {
        inotify_rm_watch(m_inotifyHandle, watchHandle);
    }
}

bool FileWatcher::PlatformStart()
{
    // inotify will be used by linux to monitor file changes within directories under the root folder
    if (!m_platformImpl->Initialize())
    {
        return false;
    }
    for (const auto& [directory, recursive] : m_folderWatchRoots)
    {
        if (QDir(directory).exists())
        {
            m_platformImpl->AddWatchFolder(directory, recursive);
        }
    }

    return true;
}

void FileWatcher::PlatformStop()
{
    m_shutdownThreadSignal = true;

    m_platformImpl->Finalize();

    if (m_thread.joinable())
    {
        m_thread.join(); // wait for the thread to finish
    }
}


void FileWatcher::WatchFolderLoop()
{
    char eventBuffer[s_inotifyReadBufferSize];
    while (!m_shutdownThreadSignal)
    {
        ssize_t bytesRead = ::read(m_platformImpl->m_inotifyHandle, eventBuffer, s_inotifyReadBufferSize);
        if (bytesRead < 0)
        {
            // Break out of the loop when the notify handle was closed (outside of this thread)
            break;
        }
        if (!bytesRead)
        {
            continue;
        }
        for (size_t index=0; index<bytesRead;)
        {
            const auto* event = reinterpret_cast<inotify_event*>(&eventBuffer[index]);

            if (event->mask & (IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVE))
            {
                const QString pathStr = QDir(m_platformImpl->m_handleToFolderMap[event->wd]).absoluteFilePath(event->name);

                if (event->mask & (IN_CREATE | IN_MOVED_TO))
                {
                    if (event->mask & IN_ISDIR)
                    {
                        // New Directory, see if it should be added to the watched directories
                        // It is only added if it is a child of a recursively watched directory
                        const auto found = AZStd::find_if(begin(m_folderWatchRoots), end(m_folderWatchRoots), [this, event](const WatchRoot& watchRoot)
                        {
                            return watchRoot.m_directory == m_platformImpl->m_handleToFolderMap[event->wd];
                        });

                        // If the path is not in m_folderWatchRoots, it must
                        // be a new subdirectory of a subdirectory of some
                        // other root that is being watched recursively.
                        // Maintain the recursive nature of that root.
                        const bool shouldAddFolder = (found == end(m_folderWatchRoots)) ? true : found->m_recursive;

                        if (shouldAddFolder)
                        {
                            m_platformImpl->AddWatchFolder(pathStr, true);
                        }
                    }
                    else
                    {
                        rawFileAdded(pathStr, {});
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
                        rawFileRemoved(pathStr, {});
                    }
                }
                else if ((event->mask & IN_MODIFY) && ((event->mask & IN_ISDIR) != IN_ISDIR))
                {
                    rawFileModified(pathStr, {});
                }
            }
            index += s_inotifyEventSize + event->len;
        }
    }
}
