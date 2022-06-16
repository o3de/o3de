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

    int                         m_inotifyHandle = -1;
    QMutex                      m_handleToFolderMapLock;
    QHash<int, QString>         m_handleToFolderMap;
};
