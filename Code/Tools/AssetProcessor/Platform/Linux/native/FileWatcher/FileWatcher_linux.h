/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <FileWatcher/FileWatcher.h>
#include <QMutex>
#include <QHash>
#include <QSet>

class FileWatcher::PlatformImplementation
{
public:
    bool Initialize();
    void CloseMainWatchHandle();
    void Finalize();
    void AddWatchFolder(QString folder, bool recursive, FileWatcher& source, bool notifyFiles);
    void RemoveWatchFolder(int watchHandle);

    //! Try to watch the given directory
    //! @param path the absolute path to the directory to watch.
    //! @param errnoPtr If provided, gets set to the errno right after the inotify_add_watch() call.
    //!                 Note: only contains valid information if we actually failed.
    //! @return Was the watch successful?
    bool TryToWatch(const QString &path, int* errnoPtr = nullptr);

    // This handle represents the handle to the entire notify tree.
    // Individual watches will be added to this same handle.
    int                         m_inotifyHandle = -1;

    // This handle is a simple semaphore, which will be signaled when its time to quit.
    int                         m_wakeThreadHandle = -1;
    
    QHash<int, QString>         m_handleToFolderMap;
    QSet<QString>               m_alreadyNotifiedCreate;
};
