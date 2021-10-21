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
        , m_lastProgress(0)
        , m_parent(parent)
    {
        m_worker = new DownloadWorker();
        m_worker->moveToThread(&m_workerThread);

        connect(&m_workerThread, &QThread::started, m_worker, &DownloadWorker::StartDownload);
        connect(m_worker, &DownloadWorker::Done, this, &DownloadController::HandleResults);
        connect(m_worker, &DownloadWorker::UpdateProgress, this, &DownloadController::UpdateUIProgress);
        connect(this, &DownloadController::StartGemDownload, m_worker, &DownloadWorker::StartDownload);
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
        m_gemNames.push_back(gemName);
        if (m_gemNames.size() == 1)
        {
            m_worker->SetGemToDownload(m_gemNames[0], false);
            m_workerThread.start();
        }
    }

    void DownloadController::CancelGemDownload(const QString& gemName)
    {
        auto findResult = AZStd::find(m_gemNames.begin(), m_gemNames.end(), gemName);

        if (findResult != m_gemNames.end())
        {
            if (findResult == m_gemNames.begin())
            {
                // HandleResults will remove the gem upon cancelling
                PythonBindingsInterface::Get()->CancelDownload();
            }
            else
            {
                m_gemNames.erase(findResult);
            }
        }
    }

    void DownloadController::UpdateUIProgress(int progress)
    {
        m_lastProgress = progress;
        emit GemDownloadProgress(progress);
    }

    void DownloadController::HandleResults(const QString& result)
    {
        bool succeeded = true;
        
        if (!result.isEmpty())
        {
            QMessageBox::critical(nullptr, tr("Gem download"), result);
            succeeded = false;
        }

        m_gemNames.erase(m_gemNames.begin());
        emit Done(succeeded);

        if (!m_gemNames.empty())
        {
            emit StartGemDownload(m_gemNames[0]);
        }
        else
        {
            m_workerThread.quit();
            m_workerThread.wait();
        }
    }
} // namespace O3DE::ProjectManager
