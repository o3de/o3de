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
        emit GemDownloadAdded(gemName);

        if (m_gemNames.size() == 1)
        {
            m_worker->SetGemToDownload(m_gemNames.front(), false);
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
                emit GemDownloadRemoved(gemName);
            }
        }
    }

    void DownloadController::UpdateUIProgress(int bytesDownloaded, int totalBytes)
    {
        emit GemDownloadProgress(m_gemNames.front(), bytesDownloaded, totalBytes);
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
                gemDownloadError.setWindowTitle(tr("Gem download"));
                gemDownloadError.setText(result);
                gemDownloadError.setDetailedText(detailedError);
                gemDownloadError.exec();
            }
            else
            {
                QMessageBox::critical(nullptr, tr("Gem download"), result);
            }
            succeeded = false;
        }

        QString gemName = m_gemNames.front();
        m_gemNames.erase(m_gemNames.begin());
        emit Done(gemName, succeeded);
        emit GemDownloadRemoved(gemName);

        if (!m_gemNames.empty())
        {
            emit StartGemDownload(m_gemNames.front());
        }
        else
        {
            m_workerThread.quit();
            m_workerThread.wait();
        }
    }
} // namespace O3DE::ProjectManager
