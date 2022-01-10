/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "FileWatcher.h"
#include "AzCore/std/containers/vector.h"
#include <native/assetprocessor.h>
#include <native/FileWatcher/FileWatcher_platform.h>
#include <QFileInfo>

//! IsSubfolder(folderA, folderB)
//! returns whether folderA is a subfolder of folderB
//! assumptions: absolute paths
static bool IsSubfolder(const QString& folderA, const QString& folderB)
{
    // lets avoid allocating or messing with memory - this is a MAJOR hotspot as it is called for any file change even in the cache!
    if (folderA.length() <= folderB.length())
    {
        return false;
    }

    using AZStd::begin;
    using AZStd::end;

    constexpr auto isSlash = [](const QChar c) constexpr
    {
        return c == AZ::IO::WindowsPathSeparator || c == AZ::IO::PosixPathSeparator;
    };

    const auto firstPathSeparator = AZStd::find_if(begin(folderB), end(folderB), [&isSlash](const QChar c)
    {
        return isSlash(c);
    });

    // Follow the convention used by AZ::IO::Path, and use a case-sensitive comparison on Posix paths
    const bool useCaseSensitiveCompare = (firstPathSeparator == end(folderB)) ? true : (*firstPathSeparator == AZ::IO::PosixPathSeparator);

    return AZStd::equal(begin(folderB), end(folderB), begin(folderA), [isSlash, useCaseSensitiveCompare](const QChar charAtB, const QChar charAtA)
    {
        if (isSlash(charAtA))
        {
            return isSlash(charAtB);
        }
        if (useCaseSensitiveCompare)
        {
            return charAtA == charAtB;
        }
        return charAtA.toLower() == charAtB.toLower();
    });
}

//////////////////////////////////////////////////////////////////////////
/// FileWatcher
FileWatcher::FileWatcher()
    : m_platformImpl(AZStd::make_unique<PlatformImplementation>())
{
    auto makeFilter = [this](auto signal)
    {
        return [this, signal](QString path)
        {
            const auto foundWatchRoot = AZStd::find_if(begin(m_folderWatchRoots), end(m_folderWatchRoots), [path](const WatchRoot& watchRoot)
            {
                return Filter(path, watchRoot);
            });
            if (foundWatchRoot == end(m_folderWatchRoots))
            {
                return;
            }
            AZStd::invoke(signal, this, path);
        };
    };

    // The rawFileAdded signals are emitted by the watcher thread. Use a queued
    // connection so that the consumers of the notification process the
    // notification on the main thread.
    connect(this, &FileWatcher::rawFileAdded, this, makeFilter(&FileWatcher::fileAdded), Qt::QueuedConnection);
    connect(this, &FileWatcher::rawFileRemoved, this, makeFilter(&FileWatcher::fileRemoved), Qt::QueuedConnection);
    connect(this, &FileWatcher::rawFileModified, this, makeFilter(&FileWatcher::fileModified), Qt::QueuedConnection);
}

FileWatcher::~FileWatcher()
{
    disconnect();
    StopWatching();
}

void FileWatcher::AddFolderWatch(QString directory, bool recursive)
{
    // Search for an already monitored root that is a parent of `directory`,
    // that is already watching subdirectories recursively
    const auto found = AZStd::find_if(begin(m_folderWatchRoots), end(m_folderWatchRoots), [directory](const WatchRoot& root)
    {
        return root.m_recursive && IsSubfolder(directory, root.m_directory);
    });

    if (found != end(m_folderWatchRoots))
    {
        // This directory is already watched
        return;
    }

    //create a new root and start listening for changes
    m_folderWatchRoots.push_back({directory, recursive});

    //since we created a new root, see if the new root is a super folder
    //of other roots, if it is then then fold those roots into the new super root
    if (recursive)
    {
        AZStd::erase_if(m_folderWatchRoots, [directory](const WatchRoot& root)
        {
            return IsSubfolder(root.m_directory, directory);
        });
    }
}

void FileWatcher::ClearFolderWatches()
{
    m_folderWatchRoots.clear();
}

void FileWatcher::StartWatching()
{
    if (m_startedWatching)
    {
        AZ_Warning("FileWatcher", false, "StartWatching() called when already watching for file changes.");
        return;
    }

    if (PlatformStart())
    {
        m_thread = AZStd::thread({/*.name=*/ "AssetProcessor FileWatcher thread"}, [this]{
            WatchFolderLoop();
        });
        AZ_TracePrintf(AssetProcessor::ConsoleChannel, "File Change Monitoring started.\n");
    }
    else
    {
        AZ_TracePrintf(AssetProcessor::ConsoleChannel, "File Change Monitoring failed to start.\n");
    }

    m_startedWatching = true;
}

void FileWatcher::StopWatching()
{
    if (!m_startedWatching)
    {
        AZ_Warning("FileWatcher", false, "StopWatching() called when is not watching for file changes.");
        return;
    }

    PlatformStop();

    m_startedWatching = false;
}

bool FileWatcher::Filter(QString path, const WatchRoot& watchRoot)
{
    if (!IsSubfolder(path, watchRoot.m_directory))
    {
        return false;
    }
    if (!watchRoot.m_recursive)
    {
        // filter out subtrees too.
        QStringRef subRef = path.rightRef(path.length() - watchRoot.m_directory.length());
        if ((subRef.indexOf('/') != -1) || (subRef.indexOf('\\') != -1))
        {
            return false; // filter this out.
        }

        // we don't care about subdirs.  IsDir is more expensive so we do it after the above filter.
        if (QFileInfo(path).isDir())
        {
            return false;
        }
    }
    return true;
}
