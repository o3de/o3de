/*
 * Copyright (c) Contributors to the Open 3D Engine Project
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
        , m_progressEstimate(0)
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
        // Check if we are trying to cancel task
        if (QThread::currentThread()->isInterruptionRequested())
        {
            emit Done(tr("Build Cancelled."));
            return;
        }

        QFile logFile(LogFilePath());
        if (!logFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        {
            emit Done(tr("Failed to open log file."));
            return;
        }

        QTextStream logStream(&logFile);
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

        if (QThread::currentThread()->isInterruptionRequested())
        {
            logFile.close();
            emit Done(tr("Build Cancelled."));
            return;
        }

        // Show some kind of progress with very approximate estimates
        UpdateProgress(m_progressEstimate = 1);

        QProcessEnvironment currentEnvironment(QProcessEnvironment::systemEnvironment());
        // Append cmake path to PATH incase it is missing
        QDir cmakePath(engineInfo.m_path);
        cmakePath.cd("cmake/runtime/bin");
        QString pathValue = currentEnvironment.value("PATH");
        pathValue += ";" + cmakePath.path();
        currentEnvironment.insert("PATH", pathValue);

        m_configProjectProcess = new QProcess(this);
        m_configProjectProcess->setProcessChannelMode(QProcess::MergedChannels);
        m_configProjectProcess->setWorkingDirectory(m_projectInfo.m_path);
        m_configProjectProcess->setProcessEnvironment(currentEnvironment);

        m_configProjectProcess->start(
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

        if (!m_configProjectProcess->waitForStarted())
        {
            emit Done(tr("Configuring project failed to start."));
            return;
        }
        bool containsGeneratingDone = false;
        while (m_configProjectProcess->waitForReadyRead(MaxBuildTimeMSecs))
        {
            QString configOutput = m_configProjectProcess->readAllStandardOutput();

            if (configOutput.contains("Generating done"))
            {
                containsGeneratingDone = true;
            }

            logStream << configOutput;
            logStream.flush();

            UpdateProgress(qMin(++m_progressEstimate, 19));

            if (QThread::currentThread()->isInterruptionRequested())
            {
                logFile.close();
                m_configProjectProcess->close();
                emit Done(tr("Build Cancelled."));
                return;
            }
        }

        if (m_configProjectProcess->exitCode() != 0 || !containsGeneratingDone)
        {
            emit Done(tr("Configuring project failed. See log for details."));
            return;
        }

        UpdateProgress(m_progressEstimate = 20);

        m_buildProjectProcess = new QProcess(this);
        m_buildProjectProcess->setProcessChannelMode(QProcess::MergedChannels);
        m_buildProjectProcess->setWorkingDirectory(m_projectInfo.m_path);
        m_buildProjectProcess->setProcessEnvironment(currentEnvironment);

        m_buildProjectProcess->start(
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

        if (!m_buildProjectProcess->waitForStarted())
        {
            emit Done(tr("Building project failed to start."));
            return;
        }

        // There are a lot of steps when building so estimate around 800 more steps (80 * 10) remaining
        m_progressEstimate = 200;
        while (m_buildProjectProcess->waitForReadyRead(MaxBuildTimeMSecs))
        {
            logStream << m_buildProjectProcess->readAllStandardOutput();
            logStream.flush();

            UpdateProgress(qMin(++m_progressEstimate / 10, 100));

            if (QThread::currentThread()->isInterruptionRequested())
            {
                logFile.close();
                char ctrlC = 0x03;
                m_buildProjectProcess->write(&ctrlC);
                m_buildProjectProcess->waitForBytesWritten();
                m_buildProjectProcess->kill();
                QThread::sleep(20);
                logStream << m_buildProjectProcess->readAllStandardOutput();
                logStream.flush();
                emit Done(tr("Build Cancelled."));
                return;
            }
        }

        if (m_configProjectProcess->exitCode() != 0)
        {
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
        if (!logFilePath.cd(ProjectBuildPathPostfix))
        {
            logFilePath.mkpath(ProjectBuildPathPostfix);
            logFilePath.cd(ProjectBuildPathPostfix);

        }
        if (!logFilePath.cd(ProjectBuildPathCmakeFiles))
        {
            logFilePath.mkpath(ProjectBuildPathCmakeFiles);
            logFilePath.cd(ProjectBuildPathCmakeFiles);
        }
        return logFilePath.filePath(ProjectBuildErrorLogName);
    }

    ProjectBuilderController::ProjectBuilderController(const ProjectInfo& projectInfo, ProjectButton* projectButton, QWidget* parent)
        : QObject()
        , m_projectInfo(projectInfo)
        , m_projectButton(projectButton)
        , m_lastProgress(0)
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

            emit Done(false);
        }

        emit Done(true);
    }

    void ProjectBuilderController::HandleCancel()
    {
        m_workerThread.quit();
        emit Done(false);
    }
} // namespace O3DE::ProjectManager
