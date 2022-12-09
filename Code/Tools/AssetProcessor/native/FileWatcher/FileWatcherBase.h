/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QObject>
#include <QString>
#endif

//////////////////////////////////////////////////////////////////////////
//! FileWatcherBase
/*! Base class that handles creation and deletion of FolderRootWatches.
 * */
class FileWatcherBase : public QObject
{
    Q_OBJECT

public:
    virtual void AddFolderWatch(QString directory, bool recursive = true) = 0;
    virtual void ClearFolderWatches() = 0;

    virtual void StartWatching() = 0;
    virtual void StopWatching() = 0;

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
};
