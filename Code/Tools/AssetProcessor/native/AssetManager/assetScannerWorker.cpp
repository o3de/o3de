/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "native/AssetManager/assetScannerWorker.h"
#include "native/AssetManager/assetScanner.h"
#include "native/utilities/PlatformConfiguration.h"
#include <QDir>
#include <QtConcurrent/QtConcurrentFilter>

using namespace AssetProcessor;

AssetScannerWorker::AssetScannerWorker(PlatformConfiguration* config, QObject* parent)
    : QObject(parent)
    , m_platformConfiguration(config)
{
}

void AssetScannerWorker::StartScan()
{
    // this must be called from the thread operating it and not the main thread.
    Q_ASSERT(QThread::currentThread() == this->thread());

    m_fileList.clear();
    m_folderList.clear();
    m_excludedList.clear();
    
    m_doScan = true;

    AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Scanning file system for changes...\n");

    Q_EMIT ScanningStateChanged(AssetProcessor::AssetScanningStatus::Started);
    Q_EMIT ScanningStateChanged(AssetProcessor::AssetScanningStatus::InProgress);

    for (int idx = 0; idx < m_platformConfiguration->GetScanFolderCount(); idx++)
    {
        const ScanFolderInfo& scanFolderInfo = m_platformConfiguration->GetScanFolderAt(idx);
        ScanForSourceFiles(scanFolderInfo, scanFolderInfo);
    }

    // we want not to emit any signals until we're finished scanning
    // so that we don't interleave directory tree walking (IO access to the file table)
    // with file access (IO access to file data) caused by sending signals to other classes.

    if (!m_doScan)
    {
        m_fileList.clear();
        m_folderList.clear();
        m_excludedList.clear();
        
        Q_EMIT ScanningStateChanged(AssetProcessor::AssetScanningStatus::Stopped);
        return;
    }
    else
    {
        EmitFiles();
    }

    AZ_TracePrintf(AssetProcessor::ConsoleChannel, "File system scan done.\n");

    Q_EMIT ScanningStateChanged(AssetProcessor::AssetScanningStatus::Completed);
}

// note:  Call this directly from the main thread!
// do not queue this call.
// Join the thread if you intend to wait until its stopped
void AssetScannerWorker::StopScan()
{
    m_doScan = false;
}

void AssetScannerWorker::ScanForSourceFiles(const ScanFolderInfo& scanFolderInfo, const ScanFolderInfo& rootScanFolder)
{
    if (!m_doScan)
    {
        return;
    }

    QFileInfoList entries;

    QDir cacheDir;
    AssetUtilities::ComputeProjectCacheRoot(cacheDir);
    QString normalizedCachePath = AssetUtilities::NormalizeDirectoryPath(cacheDir.absolutePath());
    AZ::IO::Path cachePath(normalizedCachePath.toUtf8().constData());

    QString intermediateAssetsFolder = QString::fromUtf8(AssetUtilities::GetIntermediateAssetsFolder(cachePath).c_str());
    QString normalizedIntermediateAssetsFolder = AssetUtilities::NormalizeDirectoryPath(intermediateAssetsFolder);

    // Implemented non-recursively so that the above functions only have to be called once per scan,
    // and so that the performance is easy to analyze in a profiler:
    QList<ScanFolderInfo> pathsToScanQueue;
    pathsToScanQueue.push_back(scanFolderInfo);

    while (!pathsToScanQueue.empty())
    {
        ScanFolderInfo pathToScan = pathsToScanQueue.back();
        pathsToScanQueue.pop_back();
        QDir dir(pathToScan.ScanPath());
        dir.setSorting(QDir::Unsorted);
        // Only scan sub folders if recurseSubFolders flag is set
        if (!scanFolderInfo.RecurseSubFolders())
        {
            entries = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files);
        }
        else
        {
            entries = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Files);
        }

        for (const QFileInfo& entry : entries)
        {
            if (!m_doScan) // scan was cancelled!
            {
                return;
            }

            QString absPath = entry.absoluteFilePath();
            const bool isDirectory = entry.isDir();
            QDateTime modTime = entry.lastModified();
            AZ::u64 fileSize = isDirectory ? 0 : entry.size();
            AssetFileInfo assetFileInfo(absPath, modTime, fileSize, &rootScanFolder, isDirectory);
            QString relPath = absPath.mid(rootScanFolder.ScanPath().length() + 1);

            if (isDirectory)
            {
                // in debug, assert that the paths coming from qt directory info iteration is already normalized
                // allowing us to skip normalization and know that comparisons like "IsInCacheFolder" will actually succed.
                Q_ASSERT(absPath == AssetUtilities::NormalizeDirectoryPath(absPath));
                // Filtering out excluded directories immediately (not in a thread pool) since that prevents us from recursing.

                // we already know the root scan folder, and can thus chop that part off and call the cheaper IsFileExcludedRelPath:

                if (m_platformConfiguration->IsFileExcludedRelPath(relPath))
                {
                    m_excludedList.insert(AZStd::move(assetFileInfo));
                    continue;
                }

                // Entry is a directory
                // The AP needs to know about all directories so it knows when a delete occurs if the path refers to a folder or a file
                m_folderList.insert(AZStd::move(assetFileInfo));

                // recurse into this folder.
                // Since we only care about source files, we can skip cache folders that are not the Intermediate Assets Folder.

                if (absPath.startsWith(normalizedCachePath))
                {
                    // its in the cache.  Is it the cache itself?
                    if (absPath.length() != normalizedCachePath.length())
                    {
                        // no.  Is it in the intermediateassets?
                        if (!absPath.startsWith(normalizedIntermediateAssetsFolder))
                        {
                            // Its not something in the intermediate assets folder, nor is it the cache itself,
                            // so it is just a file somewhere in the cache.
                            continue; // do not recurse.
                        }
                    }
                }
                // then we can recurse.  Otherwise, its a non-intermediate-assets-folder
                ScanFolderInfo tempScanFolderInfo(absPath, "", "", false, true);
                pathsToScanQueue.push_back(tempScanFolderInfo);
            }
            else
            {
                // Entry is a file
                Q_ASSERT(absPath == AssetUtilities::NormalizeFilePath(absPath));

                if (!AssetUtilities::IsInCacheFolder(absPath.toUtf8().constData(), cachePath)) // Ignore files in the cache
                {
                    if (!m_platformConfiguration->IsFileExcludedRelPath(relPath))
                    {
                        m_fileList.insert(AZStd::move(assetFileInfo));
                    }
                    else
                    {
                        m_excludedList.insert(AZStd::move(assetFileInfo));
                    }
                }
            }
        }
    }
}

void AssetScannerWorker::EmitFiles()
{
    Q_EMIT FilesFound(m_fileList);
    m_fileList.clear();
    Q_EMIT FoldersFound(m_folderList);
    m_folderList.clear();
    Q_EMIT ExcludedFound(m_excludedList);
    m_excludedList.clear();
}
