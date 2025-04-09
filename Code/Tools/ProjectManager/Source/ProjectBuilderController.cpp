/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ProjectBuilderController.h>
#include <ProjectBuilderWorker.h>
#include <ProjectButtonWidget.h>
#include <SettingsInterface.h>

#include <QMessageBox>
#include <QDebug>
#include <QDesktopServices>
#include <QUrl>

namespace O3DE::ProjectManager
{
    ProjectBuilderController::ProjectBuilderController(const ProjectInfo& projectInfo, ProjectButton* projectButton, QWidget* parent)
        : QObject()
        , m_projectInfo(projectInfo)
        , m_projectButton(projectButton)
        , m_parent(parent)
    {
        m_worker = new ProjectBuilderWorker(m_projectInfo);
        m_worker->moveToThread(&m_workerThread);

        // Remove key here in case Project Manager crashed while building because that causes HandleResults to not be called
        SettingsInterface::Get()->SetProjectBuiltSuccessfully(m_projectInfo, false);

        connect(&m_workerThread, &QThread::finished, m_worker, &ProjectBuilderWorker::deleteLater);
        connect(&m_workerThread, &QThread::started, m_worker, &ProjectBuilderWorker::BuildProject);
        connect(m_worker, &ProjectBuilderWorker::Done, this, &ProjectBuilderController::HandleResults);
        connect(m_worker, &ProjectBuilderWorker::UpdateProgress, this, &ProjectBuilderController::UpdateUIProgress);
    }

    ProjectBuilderController::~ProjectBuilderController()
    {
        m_workerThread.requestInterruption();
        m_workerThread.quit();
        m_workerThread.wait();
    }

    void ProjectBuilderController::Start()
    {
        m_workerThread.start();
        UpdateUIProgress(0);
    }

    void ProjectBuilderController::SetProjectButton(ProjectButton* projectButton)
    {
        m_projectButton = projectButton;

        if (projectButton)
        {
            projectButton->SetProjectButtonAction(tr("Cancel"), [this] { HandleCancel(); });
            projectButton->SetBuildLogsLink(QUrl::fromLocalFile(m_worker->GetLogFilePath()));
            projectButton->SetState(ProjectButtonState::Building);

            if (!m_lastLine.isEmpty())
            {
                UpdateUIProgress(m_lastLine);
            }
        }
    }

    const ProjectInfo& ProjectBuilderController::GetProjectInfo() const
    {
        return m_projectInfo;
    }

    void ProjectBuilderController::UpdateUIProgress(const QString& lastLine)
    {
        m_lastLine = lastLine.left(s_maxDisplayedBuiltOutputChars);

        if (m_projectButton)
        {
            m_projectButton->SetContextualText(m_lastLine);
        }
    }

    void ProjectBuilderController::HandleResults(const QString& result)
    {
        if (!result.isEmpty())
        {
            if (result.contains(tr("log")))
            {
                QMessageBox::StandardButton openLog = QMessageBox::critical(
                    m_parent,
                    tr("Project Failed to Build!"),
                    result + tr("\n\nWould you like to view log?"),
                    QMessageBox::No | QMessageBox::Yes);

                if (openLog == QMessageBox::Yes)
                {
                    // Open application assigned to this file type
                    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(m_worker->GetLogFilePath())))
                    {
                        qDebug() << "QDesktopServices::openUrl failed to open " << m_projectInfo.m_logUrl.toString() << "\n";
                    }
                }

                m_projectInfo.m_buildFailed = true;
                m_projectInfo.m_logUrl = QUrl::fromLocalFile(m_worker->GetLogFilePath());
                emit NotifyBuildProject(m_projectInfo);
            }
            else
            {
                QMessageBox::critical(m_parent, tr("Project Failed to Build!"), result);

                m_projectInfo.m_buildFailed = true;
                m_projectInfo.m_logUrl = QUrl::fromLocalFile(m_worker->GetLogFilePath());
                emit NotifyBuildProject(m_projectInfo);
            }

            SettingsInterface::Get()->SetProjectBuiltSuccessfully(m_projectInfo, false);

            emit Done(false);
            return;
        }
        else
        {
            m_projectInfo.m_buildFailed = false;

            SettingsInterface::Get()->SetProjectBuiltSuccessfully(m_projectInfo, true);
        }

        emit Done(true);
    }

    void ProjectBuilderController::HandleCancel()
    {
        m_workerThread.quit();
        emit Done(false);
    }
} // namespace O3DE::ProjectManager
