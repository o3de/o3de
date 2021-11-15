/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/thread.h>
#include <QMap>
#include <QVector>
#include <QString>
#include <QObject>

#endif

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
    ~FileWatcher() override;

    //////////////////////////////////////////////////////////////////////////
    void AddFolderWatch(QString directory, bool recursive = true);
    void ClearFolderWatches();
    //////////////////////////////////////////////////////////////////////////

    void StartWatching();
    void StopWatching();

Q_SIGNALS:
    // These signals are emitted when a file under a watched path changes
    void fileAdded(QString filePath);
    void fileRemoved(QString filePath);
    void fileModified(QString filePath);

    // These signals are emitted by the platform implementations when files
    // change. Some platforms' file watch APIs do not support non-recursive
    // watches, so the signals are filtered before being forwarded to the
    // non-"raw" fileAdded/Removed/Modified signals above.
    void rawFileAdded(QString filePath, QPrivateSignal);
    void rawFileRemoved(QString filePath, QPrivateSignal);
    void rawFileModified(QString filePath, QPrivateSignal);

private:
    bool PlatformStart();
    void PlatformStop();
    void WatchFolderLoop();

    class PlatformImplementation;
    friend class PlatformImplementation;
    struct WatchRoot
    {
        QString m_directory;
        bool m_recursive;
    };
    static bool Filter(QString path, const WatchRoot& watchRoot);

    AZStd::unique_ptr<PlatformImplementation> m_platformImpl;
    AZStd::vector<WatchRoot> m_folderWatchRoots;
    AZStd::thread m_thread;
    bool m_startedWatching = false;
    AZStd::atomic_bool m_shutdownThreadSignal = false;
};
