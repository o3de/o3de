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

class FileWatcher::PlatformImplementation
{
public:
    bool Initialize();
    void Finalize();
    void AddWatchFolder(QString folder, bool recursive);
    void RemoveWatchFolder(int watchHandle);

    //! Try to watch the given path.
    //! @param errnoPtr If provided, gets set to the errno right after the inotify_add_watch() call.
    //!                 Note: only contains valid information if we actually failed.
    //! @return Was the watch successful?
    bool TryToWatch(const QString &path, int* errnoPtr = nullptr);

    int                         m_inotifyHandle = -1;
    QMutex                      m_handleToFolderMapLock;
    QHash<int, QString>         m_handleToFolderMap;
};
