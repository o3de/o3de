/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef FILEWATCHER_COMPONENT_H
#define FILEWATCHER_COMPONENT_H
//////////////////////////////////////////////////////////////////////////

#if !defined(Q_MOC_RUN)
#include "FileWatcherAPI.h"

#include <AzCore/std/containers/vector.h>
#include <QMap>
#include <QVector>
#include <QString>

#include <thread>
#endif

class FileWatcher;

//////////////////////////////////////////////////////////////////////////
//! FolderRootWatch
/*! Class used for holding a point in the files system from which file changes are tracked.
 * */
class FolderRootWatch
    : public QObject
{
    Q_OBJECT

    friend class FileWatcher;
public:
    FolderRootWatch(const QString rootFolder);
    virtual ~FolderRootWatch();

    void ProcessNewFileEvent(const QString& file);
    void ProcessDeleteFileEvent(const QString& file);
    void ProcessModifyFileEvent(const QString& file);
    void ProcessRenameFileEvent(const QString& fileOld, const QString& fileNew);

public Q_SLOTS:
    bool Start();
    void Stop();

private:
    void WatchFolderLoop();

private:
    std::thread m_thread;
    QString m_root;
    QMap<int, FolderWatchBase*> m_subFolderWatchesMap;
    volatile bool m_shutdownThreadSignal;
    FileWatcher* m_fileWatcher;

    // Can't use unique_ptr because this is a QObject and Qt's magic sauce is
    // unable to determine the size of the unique_ptr and so fails to compile
    struct PlatformImplementation;
    PlatformImplementation* m_platformImpl;
};

//////////////////////////////////////////////////////////////////////////
//! FileWatcher
/*! Class that handles creation and deletion of FolderRootWatches based on
 *! the given FolderWatches, and forwards file change signals to them.
 * */
class FileWatcher
    : public QObject
{
    Q_OBJECT

public:
    FileWatcher();
    virtual ~FileWatcher();

    //////////////////////////////////////////////////////////////////////////
    virtual int AddFolderWatch(FolderWatchBase* pFolderWatch);
    virtual void RemoveFolderWatch(int handle);
    //////////////////////////////////////////////////////////////////////////
    
    void StartWatching();
    void StopWatching();

Q_SIGNALS:
    void AnyFileChange(FileChangeInfo info);

private:
    int m_nextHandle;
    AZStd::vector<FolderRootWatch*> m_folderWatchRoots;
    bool m_startedWatching = false;
};

#endif//FILEWATCHER_COMPONENT_H
