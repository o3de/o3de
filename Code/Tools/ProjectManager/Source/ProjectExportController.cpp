/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ProjectExportController.h>
#include <ProjectExportWorker.h>
#include <ProjectButtonWidget.h>
#include <SettingsInterface.h>

#include <QMessageBox>
#include <QDebug>
#include <QDesktopServices>
#include <QUrl>

namespace O3DE::ProjectManager
{
    ProjectExportController::ProjectExportController(const ProjectInfo& projectInfo, ProjectButton* projectButton, QWidget* parent)
        : QObject()
        , m_projectInfo(projectInfo)
        , m_projectButton(projectButton)
        , m_parent(parent)
    {
        m_worker = new ProjectExportWorker(m_projectInfo);
        m_worker->moveToThread(&m_workerThread);

        // Remove key here in case Project Manager crashed while building because that causes HandleResults to not be called
        SettingsInterface::Get()->SetProjectBuiltSuccessfully(m_projectInfo, false);

        connect(&m_workerThread, &QThread::finished, m_worker, &ProjectExportWorker::deleteLater);
        connect(&m_workerThread, &QThread::started, m_worker, &ProjectExportWorker::ExportProject);
        connect(m_worker, &ProjectExportWorker::Done, this, &ProjectExportController::HandleResults);
        connect(m_worker, &ProjectExportWorker::UpdateProgress, this, &ProjectExportController::UpdateUIProgress);
    }

    ProjectExportController::~ProjectExportController()
    {
        m_workerThread.requestInterruption();
        m_workerThread.quit();
        m_workerThread.wait();
    }

    void ProjectExportController::Start()
    {
        m_workerThread.start();
        UpdateUIProgress("");
    }

    void ProjectExportController::SetProjectButton(ProjectButton* projectButton)
    {
        m_projectButton = projectButton;

        if (projectButton)
        {
            projectButton->SetProjectButtonAction(tr("Cancel"), [this] { HandleCancel(); });
            AZ::Outcome<QString, QString> logFilePathQuery = m_worker->GetLogFilePath();
            if (logFilePathQuery.IsSuccess())
            {
                projectButton->SetBuildLogsLink(QUrl::fromLocalFile(logFilePathQuery.GetValue()));
            }
            projectButton->SetState(ProjectButtonState::Exporting);

            if (!m_lastLine.isEmpty())
            {
                UpdateUIProgress(m_lastLine);
            }
        }
    }

    const ProjectInfo& ProjectExportController::GetProjectInfo() const
    {
        return m_projectInfo;
    }

    void ProjectExportController::UpdateUIProgress(const QString& lastLine)
    {
        m_lastLine = lastLine.left(s_maxDisplayedBuiltOutputChars);

        if (m_projectButton)
        {
            m_projectButton->SetContextualText(m_lastLine);
        }
    }

    void ProjectExportController::HandleResults(const QString& result)
    {
        bool success = true;
        if (!result.isEmpty())
        {
            AZ::Outcome<QString, QString> logFilePathQuery = m_worker->GetLogFilePath();
            if (result.contains(tr("log")))
            {
                QMessageBox::StandardButton openLog = QMessageBox::critical(
                    m_parent,
                    tr(LauncherExportFailedMessage),
                    result + tr("\n\nWould you like to view log?"),
                    QMessageBox::No | QMessageBox::Yes);

                if (openLog == QMessageBox::Yes)
                {
                    // Open application assigned to this file type
                    if (!logFilePathQuery.IsSuccess())
                    {
                        qDebug() << "Failed to retrieve desired log file path" << "\n";
                    }
                    else if (!QDesktopServices::openUrl(QUrl::fromLocalFile(logFilePathQuery.GetValue())))
                    {
                        qDebug() << "QDesktopServices::openUrl failed to open " << m_projectInfo.m_logUrl.toString() << "\n";
                    }
                }
                
                if (logFilePathQuery.IsSuccess())
                {
                    m_projectInfo.m_logUrl = QUrl::fromLocalFile(logFilePathQuery.GetValue());
                }
            }
            else
            {
                if (logFilePathQuery.IsSuccess())
                {
                    QMessageBox::critical(m_parent,
                                      tr("%1\nYou can check the logs in the following directory:\n%2")
                                        .arg(LauncherExportFailedMessage)
                                        .arg(logFilePathQuery.GetValue()),
                                      result);
                }
                else
                {
                    QMessageBox::critical(m_parent,
                                      tr("%1\nNo logs are available at this time.")
                                        .arg(LauncherExportFailedMessage),
                                      result);
                }
            }

            success = false;
        }

        emit Done(success);
    }

    void ProjectExportController::HandleCancel()
    {
        m_workerThread.quit();
        emit Done(false);
    }
} // namespace O3DE::ProjectManager
