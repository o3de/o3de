/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef ASSETSCANNER_H
#define ASSETSCANNER_H

#if !defined(Q_MOC_RUN)
#include "native/assetprocessor.h"
#include "assetScannerWorker.h"
#include "assetScanFolderInfo.h"
#include <QString>
#include <QThread>
#include <QList>
#endif

namespace AssetProcessor
{
    class PlatformConfiguration;

    /** This Class is responsible for scanning for assets at startup
     */

    class AssetScanner
        : public QObject
    {
        Q_OBJECT
    public:

        explicit AssetScanner(PlatformConfiguration* config, QObject* parent = nullptr);
        virtual ~AssetScanner();

        void StartScan();//Should be called to start a scan
        void StopScan();//Should be called to stop a scan

        Q_INVOKABLE AssetScanningStatus status() const;

    Q_SIGNALS:
        void AssetScanningStatusChanged(AssetScanningStatus status);
        void FilesFound(QSet<AssetFileInfo> files);
        void FoldersFound(QSet<AssetFileInfo> folders);
        void ExcludedFound(QSet<AssetFileInfo> excluded);

    private:

        QThread m_assetWorkerScannerThread;
        AssetScannerWorker m_assetScannerWorker;

        bool m_workerCreated = false;
        AZStd::atomic<AssetScanningStatus> m_status;
    };
}// end namespace AssetProcessor

#endif // ASSETSCANNER_H
