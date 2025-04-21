/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <DownloadController.h>
#include <DownloadWorker.h>
#include <PythonBindings.h>

#include <AzCore/std/ranges/ranges_algorithm.h>

#include <QMessageBox>

namespace O3DE::ProjectManager
{
    DownloadController::DownloadController(QWidget* parent)
        : QObject(parent)
    {
        m_worker = new DownloadWorker();
        m_worker->moveToThread(&m_workerThread);

        connect(&m_workerThread, &QThread::started, m_worker, &DownloadWorker::StartDownload);
        connect(m_worker, &DownloadWorker::Done, this, &DownloadController::HandleResults);
        connect(m_worker, &DownloadWorker::UpdateProgress, this, &DownloadController::UpdateUIProgress);
        connect(this, &DownloadController::StartObjectDownload, m_worker, &DownloadWorker::SetObjectToDownload);
    }

    DownloadController::~DownloadController()
    {
        connect(&m_workerThread, &QThread::finished, m_worker, &DownloadController::deleteLater);
        m_workerThread.requestInterruption();
        m_workerThread.quit();
        m_workerThread.wait();
    }

    void DownloadController::AddObjectDownload(const QString& objectName, const QString& destinationPath, DownloadObjectType objectType)
    {
        m_objects.push_back({ objectName, destinationPath, objectType });
        emit ObjectDownloadAdded(objectName, objectType);

        if (m_objects.size() == 1)
        {
            m_worker->SetObjectToDownload(m_objects.front().m_objectName, destinationPath, objectType, false);
            m_workerThread.start();
        }
    }

    bool DownloadController::IsDownloadingObject(const QString& objectName, DownloadObjectType objectType)
    {
        auto findResult = AZStd::ranges::find_if(m_objects,
            [objectName, objectType](const DownloadableObject& object)
            {
                return (object.m_objectType == objectType && object.m_objectName == objectName);
            });
        return findResult != m_objects.end();
    }

    void DownloadController::CancelObjectDownload(const QString& objectName, DownloadObjectType objectType)
    {
        auto findResult = AZStd::ranges::find_if(m_objects,
            [objectName, objectType](const DownloadableObject& object)
            {
                return (object.m_objectType == objectType && object.m_objectName == objectName);
            });

        if (findResult != m_objects.end())
        {
            if (findResult == m_objects.begin())
            {
                // HandleResults will remove the object upon cancelling
                PythonBindingsInterface::Get()->CancelDownload();
            }
            else
            {
                m_objects.erase(findResult);
                emit ObjectDownloadRemoved(objectName, objectType);
            }
        }
    }

    void DownloadController::UpdateUIProgress(int bytesDownloaded, int totalBytes)
    {
        if (!m_objects.empty())
        {
            const DownloadableObject& downloadableObject = m_objects.front();
            emit ObjectDownloadProgress(downloadableObject.m_objectName, downloadableObject.m_objectType, bytesDownloaded, totalBytes);
        }
    }

    void DownloadController::HandleResults(const QString& result, const QString& detailedError)
    {
        bool succeeded = true;
        
        if (!result.isEmpty())
        {
            if (!detailedError.isEmpty())
            {
                QMessageBox downloadError;
                downloadError.setIcon(QMessageBox::Critical);
                downloadError.setWindowTitle(tr("Download failed"));
                downloadError.setText(result);
                downloadError.setDetailedText(detailedError);
                downloadError.exec();
            }
            else
            {
                QMessageBox::critical(nullptr, tr("Download failed"), result);
            }
            succeeded = false;
        }

        if (!m_objects.empty())
        {
            DownloadableObject downloadableObject = m_objects.front();
            m_objects.erase(m_objects.begin());
            emit Done(downloadableObject.m_objectName, succeeded);
            emit ObjectDownloadRemoved(downloadableObject.m_objectName, downloadableObject.m_objectType);
        }

        if (!m_objects.empty())
        {
            const DownloadableObject& nextObject = m_objects.front();
            emit StartObjectDownload(nextObject.m_objectName, nextObject.m_destinationPath, nextObject.m_objectType, true);
        }
        else
        {
            m_workerThread.quit();
            m_workerThread.wait();
        }
    }
} // namespace O3DE::ProjectManager
