/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <O3DEObjectDownloadController.h>
#include <O3DEObjectDownloadWorker.h>
#include <ProjectButtonWidget.h>

#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>


namespace O3DE::ProjectManager
{
    O3DEObjectDownloadController::O3DEObjectDownloadController(QWidget* parent)
        : QObject()
        , m_lastProgress(0)
        , m_parent(parent)
    {
        m_worker = new O3DEObjectDownloadWorker();
        m_worker->moveToThread(&m_workerThread);

        connect(&m_workerThread, &QThread::started, m_worker, &O3DEObjectDownloadWorker::StartDownload);
        connect(m_worker, &O3DEObjectDownloadWorker::Done, this, &O3DEObjectDownloadController::HandleResults);
        connect(m_worker, &O3DEObjectDownloadWorker::UpdateProgress, this, &O3DEObjectDownloadController::UpdateUIProgress);
        connect(this, &O3DEObjectDownloadController::StartGemDownload, m_worker, &O3DEObjectDownloadWorker::StartDownload);
    }

    O3DEObjectDownloadController::~O3DEObjectDownloadController()
    {
        connect(&m_workerThread, &QThread::finished, m_worker, &O3DEObjectDownloadWorker::deleteLater);
        m_workerThread.requestInterruption();
        m_workerThread.quit();
        m_workerThread.wait();
    }

    void O3DEObjectDownloadController::AddGemDownload(const QString& gemName)
    {
        m_gemNames.push_back(gemName);
        if (m_gemNames.size() == 1)
        {
            m_worker->SetGemToDownload(m_gemNames[0], false);
            m_workerThread.start();
        }
    }

    void O3DEObjectDownloadController::Start()
    {
        
    }

    void O3DEObjectDownloadController::UpdateUIProgress(int progress)
    {
        m_lastProgress = progress;
        emit GemDownloadProgress(progress);
    }

    void O3DEObjectDownloadController::HandleResults(const QString& result)
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

    void O3DEObjectDownloadController::HandleCancel()
    {
        m_workerThread.quit();
        emit Done(false);
    }
} // namespace O3DE::ProjectManager
