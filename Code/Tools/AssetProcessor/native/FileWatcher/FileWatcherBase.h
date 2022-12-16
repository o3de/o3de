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

namespace AssetBuilderSDK
{
    class FilePatternMatcher;
}

//////////////////////////////////////////////////////////////////////////
//! FileWatcherBase
/*! Base class that handles creation and deletion of FolderRootWatches.
* This API has the following guarantees:
* - you will ALWAYS get fileAdded for every file and every folder added in a watched folder (assuming recursive), and this holds true
*   no matter the timing, as long as the folder to watch was added before calling StartWatching.
*     - This holds true even for files or folders created immediately inside a folder that was also just created.
* - You will receive all events in the order that they occur on the file system.
*         - One exception to this - Rapidly interleaving creation of files with creation of folders can cause the file create
*         notifications to occur
*           later than the folder create notifications.   The folder containing a new file will always precede the new file in it.
* - you will NOT necessarily get fileRemoved for files when the folder containing them is moved or renamed.
*       (Operating systems tend to just unlink or rename the folder and not generate file events for things inside the folder).
* - you will NOT necessarily get fileModified if rapidly creating new folders, creating new files in that folder, and then modifying
* the files.
*   all in a very short time.  You will still get a fileAdded for each folder and file and you will still always get the folders before
*   the files in them.
* - you may get fileModified MULTIPLE times including for each file created or each modification done.  This is because each operating
* system
*   treats file modify differently, and in some cases, creating a file modifies its metadata (security attributes, size, ...) and that
*   counts as a modify.  likewise, touching the data of a file counts as multiple things being modified (size, content, attributes,
*   modification time) and the OS may issue each of those as separate modify events, with no way to tell them apart.
 * */
class FileWatcherBase : public QObject
{
    Q_OBJECT

public:
    virtual void AddFolderWatch(QString directory, bool recursive = true) = 0;
    virtual void ClearFolderWatches() = 0;

    virtual void StartWatching() = 0;
    virtual void StopWatching() = 0;

    //! Note that on some platforms, it is cheaper to exclude folders by not watching for
    //! events in the first place, but in other platforms, watching is a file system level
    //! operation that recurses into every sub-folder without consuming extra resources.
    //! It is up to the platform implementation to use the IsExcluded as they see fit.
    //! The API will never issue a file signal on something that matches an exclusion.
    virtual void AddExclusion(const AssetBuilderSDK::FilePatternMatcher& excludeMatch) = 0;
    virtual bool IsExcluded(QString filePath) const = 0;

    //! Installs all the default exclusion rules.  Exclusions are somewhat expensive and happens
    //! inside a OS callback that is time sensitive, so this installs just a few specific known
    //! exclusions for cases that usually result in a deep file heirarchy of things that ought to be
    //! excluded - the cache folder, the user/log folder, etc.
    //! Paths for the params that are empty will cause no rules to be installed for that path or children.
    virtual void InstallDefaultExclusionRules(QString cacheRootPath, QString projectRootPath) = 0;

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
