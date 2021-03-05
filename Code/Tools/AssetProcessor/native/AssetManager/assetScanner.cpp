/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include "native/AssetManager/assetScanner.h"

namespace AssetProcessor
{
    AssetScanner::AssetScanner(PlatformConfiguration* config, QObject* parent)
        : QObject(parent)
        , m_assetScannerWorker(config)
        , m_status(AssetScanningStatus::Unknown)
    {
        m_assetScannerWorker.moveToThread( &m_assetWorkerScannerThread );

        QObject::connect(&m_assetScannerWorker, &AssetScannerWorker::FilesFound, this, &AssetScanner::FilesFound);
        QObject::connect(&m_assetScannerWorker, &AssetScannerWorker::FoldersFound, this, &AssetScanner::FoldersFound);
        QObject::connect(&m_assetScannerWorker, &AssetScannerWorker::ExcludedFound, this, &AssetScanner::ExcludedFound);

        QObject::connect(&m_assetScannerWorker, &AssetScannerWorker::ScanningStateChanged, this,
            [this](AssetProcessor::AssetScanningStatus status)
            {
                if (m_status == status)
                {
                    return;
                }
                m_status = status;

                Q_EMIT AssetScanningStatusChanged(status);
            });
    }

    AssetScanner::~AssetScanner()
    {
        StopScan();
        m_assetWorkerScannerThread.quit();
        m_assetWorkerScannerThread.wait();
    }

    void AssetScanner::StartScan()
    {
        if (!m_workerCreated)
        {
            m_workerCreated = true;
            m_assetWorkerScannerThread.setObjectName("AssetScannerWorker");
            m_assetWorkerScannerThread.start();
        }

        QMetaObject::invokeMethod(&m_assetScannerWorker, "StartScan", Qt::QueuedConnection);
    }

    void AssetScanner::StopScan()
    {
        QMetaObject::invokeMethod(&m_assetScannerWorker, "StopScan", Qt::DirectConnection);
    }

    AssetProcessor::AssetScanningStatus AssetScanner::status() const
    {
        return m_status;
    }
}

#include "native/AssetManager/moc_assetScanner.cpp"
