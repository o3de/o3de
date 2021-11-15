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
#include <ProjectManagerSettings.h>

#include <AzCore/Settings/SettingsRegistry.h>

#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>

namespace O3DE::ProjectManager
{
    ProjectBuilderController::ProjectBuilderController(const ProjectInfo& projectInfo, ProjectButton* projectButton, QWidget* parent)
        : QObject()
        , m_projectInfo(projectInfo)
        , m_projectButton(projectButton)
        , m_lastProgress(0)
        , m_parent(parent)
    {
        m_worker = new ProjectBuilderWorker(m_projectInfo);
        m_worker->moveToThread(&m_workerThread);

        auto settingsRegistry = AZ::SettingsRegistry::Get();
        if (settingsRegistry)
        {
            // Remove key here in case Project Manager crashing while building that causes HandleResults to not be called
            QString settingsKey = GetProjectBuiltSuccessfullyKey(m_projectInfo.m_projectName);
            settingsRegistry->Remove(settingsKey.toStdString().c_str());
            SaveProjectManagerSettings();
        }

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
            projectButton->SetProjectBuilding();
            projectButton->SetProjectButtonAction(tr("Cancel Build"), [this] { HandleCancel(); });

            if (m_lastProgress != 0)
            {
                UpdateUIProgress(m_lastProgress);
            }
        }
    }

    const ProjectInfo& ProjectBuilderController::GetProjectInfo() const
    {
        return m_projectInfo;
    }

    void ProjectBuilderController::UpdateUIProgress(int progress)
    {
        m_lastProgress = progress;
        if (m_projectButton)
        {
            m_projectButton->SetButtonOverlayText(QString("%1 (%2%)<br>%3<br>").arg(tr("Building Project..."), QString::number(progress), tr("Click to <a href=\"logs\">view logs</a>.")));
            m_projectButton->SetProgressBarValue(progress);
            m_projectButton->SetBuildLogsLink(m_worker->GetLogFilePath());
        }
    }

    void ProjectBuilderController::HandleResults(const QString& result)
    {
        QString settingsKey = GetProjectBuiltSuccessfullyKey(m_projectInfo.m_projectName);

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
                    QDesktopServices::openUrl(QUrl("file:///" + m_worker->GetLogFilePath()));
                }

                m_projectInfo.m_buildFailed = true;
                m_projectInfo.m_logUrl = QUrl("file:///" + m_worker->GetLogFilePath());
                emit NotifyBuildProject(m_projectInfo);
            }
            else
            {
                QMessageBox::critical(m_parent, tr("Project Failed to Build!"), result);

                m_projectInfo.m_buildFailed = true;
                m_projectInfo.m_logUrl = QUrl("file:///" + m_worker->GetLogFilePath());
                emit NotifyBuildProject(m_projectInfo);
            }

            auto settingsRegistry = AZ::SettingsRegistry::Get();
            if (settingsRegistry)
            {
                settingsRegistry->Remove(settingsKey.toStdString().c_str());
                SaveProjectManagerSettings();
            }

            emit Done(false);
            return;
        }
        else
        {
            m_projectInfo.m_buildFailed = false;

            auto settingsRegistry = AZ::SettingsRegistry::Get();
            if (settingsRegistry)
            {
                settingsRegistry->Set(settingsKey.toStdString().c_str(), true);
                SaveProjectManagerSettings();
            }
        }

        emit Done(true);
    }

    void ProjectBuilderController::HandleCancel()
    {
        m_workerThread.quit();
        emit Done(false);
    }
} // namespace O3DE::ProjectManager
