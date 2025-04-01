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
#include <QDir>

#include <AssetBuilderSDK/AssetBuilderSDK.h>

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

    auto isSlash = [](const QChar c) constexpr
    {
        return c == AZ::IO::WindowsPathSeparator || c == AZ::IO::PosixPathSeparator;
    };

    // if folderB doesn't end in a slash, make sure folderA has one at the appropriate location to
    // avoid matching a partial path that isn't a folder e.g.
    // folderA = c:/folderWithLongerName
    // folderB = c:/folder
    if (!isSlash(folderB[folderB.length() - 1]) && !isSlash(folderA[folderB.length()]))
    {
        return false;
    }

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

            if (IsExcluded(path))
            {
                return;
            }

            AZStd::invoke(signal, this, path);
        };
    };

    // The rawFileAdded signals are emitted by the watcher thread. Use a queued
    // connection so that the consumers of the notification process the
    // notification on the main thread.
    connect(this, &FileWatcherBase::rawFileAdded, this, makeFilter(&FileWatcherBase::fileAdded), Qt::QueuedConnection);
    connect(this, &FileWatcherBase::rawFileRemoved, this, makeFilter(&FileWatcherBase::fileRemoved), Qt::QueuedConnection);
    connect(this, &FileWatcherBase::rawFileModified, this, makeFilter(&FileWatcherBase::fileModified), Qt::QueuedConnection);
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

bool FileWatcher::HasWatchFolder(QString directory) const
{
    const auto found = AZStd::find_if(begin(m_folderWatchRoots), end(m_folderWatchRoots), [directory](const WatchRoot& root)
    {
            return root.m_directory == directory;
    });
    return found != end(m_folderWatchRoots);
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

    m_shutdownThreadSignal = false;

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
    
    while(!m_startedSignal)
    {
        // wait for the thread to signal that it is completely ready.  This should
        // take a very short amount of time, so yield for it (not sleep).
        AZStd::this_thread::yield();
    }

    m_startedWatching = true;
}

void FileWatcher::StopWatching()
{
    if (!m_startedWatching)
    {
        return;
    }

    m_shutdownThreadSignal = true;

    // The platform is expected to join the thread in PlatformStop.  It cannot be joined here,
    // since the platform may have to signal the thread to stop in a platform specific
    // way before it is safe to join.
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
    }

    return true;
}

bool FileWatcher::IsExcluded(QString filepath) const
{
    for (const AssetBuilderSDK::FilePatternMatcher& matcher : m_excludes)
    {
        if (matcher.MatchesPath(filepath.toUtf8().constData()))
        {
            return true;
        }
    }
    return false;
}

void FileWatcher::AddExclusion(const AssetBuilderSDK::FilePatternMatcher& excludeMatch)
{
    m_excludes.push_back(excludeMatch);
}

void FileWatcher::InstallDefaultExclusionRules(QString cacheRootPath, QString projectRootPath)
{
    constexpr const char* intermediates = AssetProcessor::IntermediateAssetsFolderName;
    using AssetBuilderSDK::FilePatternMatcher;
    using AssetBuilderSDK::AssetBuilderPattern;

    // Note to maintainers:
    // If you add more here, consider updating DefaultExcludes_ExcludeExpectedLocations.  It turns out each platform
    // can approach these exclusions slightly differently, due to slash direction, naming, and how it installs its
    // file monitors.
    // 
    // File exclusions from the config are already checked on all files coming from the file watcher, but are done so
    // in the main thread quite late in the process, so as not to block the file monitor unnecessarily.
    // The file monitor is sensitive to being blocked due to it being a raw listener to some operating system level file event
    // stream, and as little work in its threads should be done as possible, so do not add a large number of exclusions here.
    // The best situation is a really small number of exclusions that match a very broad number of actual files (like the entire
    // user folder full of log files).
    //
    // For most situations its probably better to just filter on the main thread instead of in the watcher - and most implementations
    // of this class do just that.  
    // However, on some operating systems, each monitored folder in a tree of monitored folders costs actual system resources
    // (a handle) and there are limited handles available, so excluding entire folder trees that we know we don't care about
    // is valuable to save resources even if it costs more in the listener.
    // 
    // It is up to each implementation to make use of the list of excludes to best optimize itself for performance.  Even if the
    // implementation does absolutely nothing with this exclude list, this class itself will still filter out excludes before
    // forwarding the file events to actual handlers.
    //
    // To strike a balance here, add just a few hand-picked exclusions that tend to contain a lot of unnecessary folders:
    //    * Everything in the cache EXCEPT for the "Intermediate Assets" and "fence" folders (filtering out fence will deadlock!)
    //    * Project/build/*    (case insensitive)
    //    * Project/user/*     (case insensitive)
    //    * Project/gem/code/* (case insensitive)
    // These (except for the cache) also mimic the built in exclusions for scanning, and are also likely to be deep folder trees.

    if (!cacheRootPath.isEmpty())
    {
        // Use the actual cache root as part of the regex for filtering out the Intermediate Assets and fence folder
        // this prevents accidental filtering of folders that have the word 'Cache' in them.
        QString nativeCacheRoot = QDir::toNativeSeparators(cacheRootPath);

        // Sanitize for regex by prepending any special characters with the escape character:
        QString sanitizedCacheFolderString;
        QString regexEscapeChars(R"(\.^$-+()[]{}|?*)");
        for (int pos = 0; pos < nativeCacheRoot.size(); ++pos)
        {
            if (regexEscapeChars.contains(nativeCacheRoot[pos]))
            {
                sanitizedCacheFolderString.append('\\');
            }
            sanitizedCacheFolderString.append(nativeCacheRoot[pos]);
        }

        const char* filterString = R"(^%s[\\\/](?!%s|fence).*$)"; // [\\\/] will match \\ and /
        // final form is something like ^C:\\o3de\\projects\\Project1\\Cache[\\\/](?!Intermediate Assets|fence).*$
        // on unix-like,                ^/home/myuser/o3de-projects/Project1/Cache[\\\/](?!Intermediate Assets|fence).*$
        AZStd::string exclusion = AZStd::string::format(filterString, sanitizedCacheFolderString.toUtf8().constData(), intermediates);
        AddExclusion(AssetBuilderSDK::FilePatternMatcher(exclusion, AssetBuilderSDK::AssetBuilderPattern::Regex));
    }

    if (!projectRootPath.isEmpty())
    {
        // These are not regexes, so do not need sanitation.  Files can't use special characters like * or ? from globs anyway.
        AZStd::string userPath = QDir::toNativeSeparators(QDir(projectRootPath).absoluteFilePath("user/*")).toUtf8().constData();
        AZStd::string buildPath = QDir::toNativeSeparators(QDir(projectRootPath).absoluteFilePath("build/*")).toUtf8().constData();
        AZStd::string gemCodePath = QDir::toNativeSeparators(QDir(projectRootPath).absoluteFilePath("gem/code/*")).toUtf8().constData();

        AddExclusion(FilePatternMatcher(userPath.c_str(), AssetBuilderPattern::Wildcard));
        AddExclusion(FilePatternMatcher(buildPath.c_str(), AssetBuilderPattern::Wildcard));
        AddExclusion(FilePatternMatcher(gemCodePath.c_str(), AssetBuilderPattern::Wildcard));
    }
}
