/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef ASSETSCANNERWORKER_H
#define ASSETSCANNERWORKER_H

#if !defined(Q_MOC_RUN)
#include "native/assetprocessor.h"
#include "assetScanFolderInfo.h"
#include <QString>
#include <QSet>
#include <QObject>
#endif

namespace AssetProcessor
{
    class PlatformConfiguration;

    /** This Class is actually responsible for scanning the game folder
     * and finding file of interest files.
     * Its created on the main thread and then moved to the worker thread
     * so it should contain no QObject-based classes at construction time (it can make them later)
     */
    class AssetScannerWorker
        : public QObject
    {
        Q_OBJECT
    public:
        explicit AssetScannerWorker(PlatformConfiguration* config, QObject* parent = 0);

Q_SIGNALS:
        void ScanningStateChanged(AssetProcessor::AssetScanningStatus status);
        void FilesFound(QSet<AssetFileInfo> files); // QSet<QString> is a refcounted copy-on-write object, do not pass by ref.
        void FoldersFound(QSet<AssetFileInfo> folders); // QSet<QString> is a refcounted copy-on-write object, do not pass by ref.
        void ExcludedFound(QSet<AssetFileInfo> excluded); // QSet<QString> is a refcounted copy-on-write object, do not pass by ref.

    public Q_SLOTS:
        void StartScan();
        void StopScan();

    protected:
        // scanFolderInfo - the folder we're currently scanning (this will sometimes be a fake scanfolder created when recursing through directories)
        // rootScanFolder - the actual scan folder we started with, which will either be the same as scanFolderInfo or a parent folder
        void ScanForSourceFiles(const ScanFolderInfo& scanFolderInfo, const ScanFolderInfo& rootScanFolder);
        void EmitFiles();

    private:
        volatile bool m_doScan = true;
        QSet<AssetFileInfo> m_fileList; // note:  neither QSet nor QString are qobject-derived
        QSet<AssetFileInfo> m_folderList;
        QSet<AssetFileInfo> m_excludedList;

        PlatformConfiguration* m_platformConfiguration;
    };
} // end namespace AssetProcessor

#endif // ASSETSCANNERWORKER_H
