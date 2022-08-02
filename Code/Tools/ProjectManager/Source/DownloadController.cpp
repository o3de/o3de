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

#include <AzCore/std/algorithm.h>

#include <QMessageBox>

namespace O3DE::ProjectManager
{
    DownloadController::DownloadController(QWidget* parent)
        : QObject()
        , m_parent(parent)
    {
        m_worker = new DownloadWorker();
        m_worker->moveToThread(&m_workerThread);

        connect(&m_workerThread, &QThread::started, m_worker, &DownloadWorker::StartDownload);
        connect(m_worker, &DownloadWorker::Done, this, &DownloadController::HandleResults);
        connect(m_worker, &DownloadWorker::UpdateProgress, this, &DownloadController::UpdateUIProgress);
        connect(this, &DownloadController::StartGemDownload, m_worker, &DownloadWorker::SetGemToDownload);
        connect(this, &DownloadController::StartProjectDownload, m_worker, &DownloadWorker::SetProjectToDownload);
        connect(this, &DownloadController::StartTemplateDownload, m_worker, &DownloadWorker::SetTemplateToDownload);
    }

    DownloadController::~DownloadController()
    {
        connect(&m_workerThread, &QThread::finished, m_worker, &DownloadController::deleteLater);
        m_workerThread.requestInterruption();
        m_workerThread.quit();
        m_workerThread.wait();
    }

    void DownloadController::AddGemDownload(const QString& gemName)
    {
        m_objects.push_back({ gemName, DownloadObjectType::Gem });
        emit GemDownloadAdded(gemName);

        if (m_objects.size() == 1)
        {
            m_worker->SetGemToDownload(m_objects.front().m_objectName, false);
            m_workerThread.start();
        }
    }

    void DownloadController::CancelGemDownload(const QString& gemName)
    {
        auto findResult = AZStd::find_if(
            m_objects.begin(),
            m_objects.end(),
            [gemName](const DownloadableObject& object)
            {
                return (object.m_objectType == DownloadObjectType::Gem && object.m_objectName == gemName);
            });

        if (findResult != m_objects.end())
        {
            if (findResult == m_objects.begin())
            {
                // HandleResults will remove the gem upon cancelling
                PythonBindingsInterface::Get()->CancelDownload();
            }
            else
            {
                m_objects.erase(findResult);
                emit GemDownloadRemoved(gemName);
            }
        }
    }

    void DownloadController::AddProjectDownload(const QString& projectName)
    {
        m_objects.push_back({ projectName, DownloadObjectType::Project });
        emit GemDownloadAdded(projectName);

        if (m_objects.size() == 1)
        {
            m_worker->SetProjectToDownload(m_objects.front().m_objectName, false);
            m_workerThread.start();
        }
    }

    void DownloadController::CancelProjectDownload(const QString& projectName)
    {
        auto findResult = AZStd::find_if(
            m_objects.begin(),
            m_objects.end(),
            [projectName](const DownloadableObject& object)
            {
                return (object.m_objectType == DownloadObjectType::Project && object.m_objectName == projectName);
            });

        if (findResult != m_objects.end())
        {
            if (findResult == m_objects.begin())
            {
                // HandleResults will remove the gem upon cancelling
                PythonBindingsInterface::Get()->CancelDownload();
            }
            else
            {
                m_objects.erase(findResult);
                emit ProjectDownloadRemoved(projectName);
            }
        }
    }

    void DownloadController::UpdateUIProgress(int bytesDownloaded, int totalBytes)
    {
        if (m_objects.front().m_objectType == DownloadObjectType::Gem)
        {
            emit GemDownloadProgress(m_objects.front().m_objectName, bytesDownloaded, totalBytes);
        }
        else if (m_objects.front().m_objectType == DownloadObjectType::Project)
        {
            emit ProjectDownloadProgress(m_objects.front().m_objectName, bytesDownloaded, totalBytes);
        }
    }

    void DownloadController::HandleResults(const QString& result, const QString& detailedError)
    {
        bool succeeded = true;
        
        if (!result.isEmpty())
        {
            if (!detailedError.isEmpty())
            {
                QMessageBox gemDownloadError;
                gemDownloadError.setIcon(QMessageBox::Critical);
                gemDownloadError.setWindowTitle(tr("Object download"));
                gemDownloadError.setText(result);
                gemDownloadError.setDetailedText(detailedError);
                gemDownloadError.exec();
            }
            else
            {
                QMessageBox::critical(nullptr, tr("Object download"), result);
            }
            succeeded = false;
        }

        DownloadableObject downloadableObject = m_objects.front();
        m_objects.erase(m_objects.begin());
        emit Done(downloadableObject.m_objectName, succeeded);
        emit GemDownloadRemoved(downloadableObject.m_objectName);

        if (!m_objects.empty())
        {
            if (m_objects.front().m_objectType == DownloadObjectType::Gem)
            {
                emit StartGemDownload(m_objects.front().m_objectName, true);
            }
            else if (m_objects.front().m_objectType == DownloadObjectType::Project)
            {
                emit StartProjectDownload(m_objects.front().m_objectName, true);
            }
        }
        else
        {
            m_workerThread.quit();
            m_workerThread.wait();
        }
    }
} // namespace O3DE::ProjectManager
