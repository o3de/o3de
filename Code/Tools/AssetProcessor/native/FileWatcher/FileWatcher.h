/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "FileWatcherBase.h"

#if !defined(Q_MOC_RUN)
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/thread.h>
#include <QString>
#include <QObject>
#endif

//////////////////////////////////////////////////////////////////////////
//! FileWatcher
/*! Class that handles creation and deletion of FolderRootWatches based on
 *! the given FolderWatches, and forwards file change signals to them.
 * */
class FileWatcher : public FileWatcherBase
{
    Q_OBJECT

public:
    FileWatcher();
    ~FileWatcher() override;

    //////////////////////////////////////////////////////////////////////////
    void AddFolderWatch(QString directory, bool recursive = true) override;
    bool HasWatchFolder(QString directory) const; 
    void ClearFolderWatches() override;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    void StartWatching() override;
    void StopWatching() override;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    void AddExclusion(const AssetBuilderSDK::FilePatternMatcher& excludeMatch) override;
    bool IsExcluded(QString filePath) const override;
    //////////////////////////////////////////////////////////////////////////

    void InstallDefaultExclusionRules(QString cacheRootPath, QString projectRootPath) override;

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
    AZStd::vector<AssetBuilderSDK::FilePatternMatcher> m_excludes;
    AZStd::thread m_thread;
    bool m_startedWatching = false;
    AZStd::atomic_bool m_shutdownThreadSignal = false;

    // platform implementations must signal this to indicate they have fully initialized
    // and will not be dropping events.
    AZStd::atomic_bool m_startedSignal = false;
};
