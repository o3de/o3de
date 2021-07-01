/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ProjectBuilder.h>
#include <ProjectManagerDefs.h>
#include <ProjectButtonWidget.h>
#include <PythonBindingsInterface.h>

#include <QProcess>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QProcessEnvironment>


//#define MOCK_BUILD_PROJECT true 

namespace O3DE::ProjectManager
{
    // QProcess::waitForFinished uses -1 to indicate that the process should not timeout
    constexpr int MaxBuildTimeMSecs = -1;

    ProjectBuilderWorker::ProjectBuilderWorker(const ProjectInfo& projectInfo)
        : QObject()
        , m_projectInfo(projectInfo)
    {
    }

    void ProjectBuilderWorker::BuildProject()
    {
#ifdef MOCK_BUILD_PROJECT
        for (int i = 0; i < 10; ++i)
        {
            QThread::sleep(1);
            UpdateProgress(i * 10);
        }
        Done(m_projectPath);
#else
        EngineInfo engineInfo;

        AZ::Outcome<EngineInfo> engineInfoResult = PythonBindingsInterface::Get()->GetEngineInfo();
        if (engineInfoResult.IsSuccess())
        {
            engineInfo = engineInfoResult.GetValue();
        }
        else
        {
            emit Done(tr("Failed to get engine info."));
            return;
        }

        // Show some kind of progress with very approximate estimates
        UpdateProgress(1);

        QProcessEnvironment currentEnvironment(QProcessEnvironment::systemEnvironment());
        // Append cmake path to PATH incase it is missing
        QDir cmakePath(engineInfo.m_path);
        cmakePath.cd("cmake/runtime/bin");
        QString pathValue = currentEnvironment.value("PATH");
        pathValue += ";" + cmakePath.path();
        currentEnvironment.insert("PATH", pathValue);

        QProcess configProjectProcess;
        configProjectProcess.setProcessChannelMode(QProcess::MergedChannels);
        configProjectProcess.setWorkingDirectory(m_projectInfo.m_path);
        configProjectProcess.setProcessEnvironment(currentEnvironment);

        configProjectProcess.start(
            "cmake",
            QStringList
            {
                "-B",
                QDir(m_projectInfo.m_path).filePath(ProjectBuildPathPostfix),
                "-S",
                m_projectInfo.m_path,
                "-G",
                "Visual Studio 16",
                "-DLY_3RDPARTY_PATH=" + engineInfo.m_thirdPartyPath
            });

        if (!configProjectProcess.waitForStarted())
        {
            emit Done(tr("Configuring project failed to start."));
            return;
        }
        if (!configProjectProcess.waitForFinished(MaxBuildTimeMSecs))
        {
            WriteErrorLog(configProjectProcess.readAllStandardOutput());
            emit Done(tr("Configuring project timed out. See log for details"));
            return;
        }

        QString configProjectOutput(configProjectProcess.readAllStandardOutput());
        if (configProjectProcess.exitCode() != 0 || !configProjectOutput.contains("Generating done"))
        {
            WriteErrorLog(configProjectOutput);
            emit Done(tr("Configuring project failed. See log for details."));
            return;
        }

        UpdateProgress(20);

        QProcess buildProjectProcess;
        buildProjectProcess.setProcessChannelMode(QProcess::MergedChannels);
        buildProjectProcess.setWorkingDirectory(m_projectInfo.m_path);
        buildProjectProcess.setProcessEnvironment(currentEnvironment);

        buildProjectProcess.start(
            "cmake",
            QStringList
            {
                "--build",
                QDir(m_projectInfo.m_path).filePath(ProjectBuildPathPostfix),
                "--target",
                m_projectInfo.m_projectName + ".GameLauncher",
                "Editor",
                "--config",
                "profile"
            });

        if (!buildProjectProcess.waitForStarted())
        {
            emit Done(tr("Building project failed to start."));
            return;
        }
        if (!buildProjectProcess.waitForFinished(MaxBuildTimeMSecs))
        {
            WriteErrorLog(configProjectProcess.readAllStandardOutput());
            emit Done(tr("Building project timed out. See log for details"));
            return;
        }

        QString buildProjectOutput(buildProjectProcess.readAllStandardOutput());
        if (configProjectProcess.exitCode() != 0)
        {
            WriteErrorLog(buildProjectOutput);
            emit Done(tr("Building project failed. See log for details."));
        }
        else
        {
            emit Done("");
        }
#endif
    }

    QString ProjectBuilderWorker::LogFilePath() const
    {
        QDir logFilePath(m_projectInfo.m_path);
        logFilePath.cd(ProjectBuildPathPostfix);
        return logFilePath.filePath(ProjectBuildErrorLogPathPostfix);
    }

    void ProjectBuilderWorker::WriteErrorLog(const QString& log)
    {
        QFile logFile(LogFilePath());
        // Overwrite file with truncate
        if (logFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        {
            QTextStream output(&logFile);
            output << log;
            logFile.close();
        }
    }

    ProjectBuilderController::ProjectBuilderController(const ProjectInfo& projectInfo, ProjectButton* projectButton, QWidget* parent)
        : QObject()
        , m_projectInfo(projectInfo)
        , m_projectButton(projectButton)
        , m_parent(parent)
    {
        m_worker = new ProjectBuilderWorker(m_projectInfo);
        m_worker->moveToThread(&m_workerThread);

        connect(&m_workerThread, &QThread::finished, m_worker, &ProjectBuilderWorker::deleteLater);
        connect(&m_workerThread, &QThread::started, m_worker, &ProjectBuilderWorker::BuildProject);
        connect(m_worker, &ProjectBuilderWorker::Done, this, &ProjectBuilderController::HandleResults);
        connect(m_worker, &ProjectBuilderWorker::UpdateProgress, this, &ProjectBuilderController::UpdateUIProgress);
    }

    ProjectBuilderController::~ProjectBuilderController()
    {
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
    }

    QString ProjectBuilderController::GetProjectPath() const
    {
        return m_projectInfo.m_path;
    }

    void ProjectBuilderController::UpdateUIProgress(int progress)
    {
        if (m_projectButton)
        {
            m_projectButton->SetButtonOverlayText(QString("%1 (%2%)\n\n").arg(tr("Building Project..."), QString::number(progress)));
            m_projectButton->SetProgressBarValue(progress);
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
                    QDesktopServices::openUrl(QUrl("file:///" + m_worker->LogFilePath()));
                }
            }
            else
            {
                QMessageBox::critical(m_parent, tr("Project Failed to Build!"), result);
            }
        }

        emit Done();
    }
} // namespace O3DE::ProjectManager
