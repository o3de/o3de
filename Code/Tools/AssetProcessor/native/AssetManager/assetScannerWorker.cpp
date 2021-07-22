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

    QDir dir(scanFolderInfo.ScanPath());

    QFileInfoList entries;

    //Only scan sub folders if recurseSubFolders flag is set
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

        // Skip over the Cache folder if the file entry is the project cache root
        QDir projectCacheRoot;
        AssetUtilities::ComputeProjectCacheRoot(projectCacheRoot);
        QString relativeToProjectCacheRoot = projectCacheRoot.relativeFilePath(absPath);
        if (QDir::isRelativePath(relativeToProjectCacheRoot) && !relativeToProjectCacheRoot.startsWith(".."))
        {
            // The Cache folder should not be scanned
            continue;
        }

        // Filtering out excluded files
        if (m_platformConfiguration->IsFileExcluded(absPath))
        {
            m_excludedList.insert(AZStd::move(assetFileInfo));
            continue;
        }

        if (isDirectory)
        {
            //Entry is a directory
            m_folderList.insert(AZStd::move(assetFileInfo));
            ScanFolderInfo tempScanFolderInfo(absPath, "", "", false, true);
            ScanForSourceFiles(tempScanFolderInfo, rootScanFolder);
        }
        else
        {
            //Entry is a file
            m_fileList.insert(AZStd::move(assetFileInfo));
        }
    }
}

void AssetScannerWorker::EmitFiles()
{
    //Loop over all source asset files and send them up the chain:
    Q_EMIT FilesFound(m_fileList);
    m_fileList.clear();
    Q_EMIT FoldersFound(m_folderList);
    m_folderList.clear();
    Q_EMIT ExcludedFound(m_excludedList);
    m_excludedList.clear();

}
