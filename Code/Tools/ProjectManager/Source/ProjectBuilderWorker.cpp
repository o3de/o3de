/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ProjectBuilderWorker.h>
#include <ProjectManagerDefs.h>
#include <PythonBindingsInterface.h>
#include <ProjectUtils.h>

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTextStream>
#include <QThread>

//#define MOCK_BUILD_PROJECT true

namespace O3DE::ProjectManager
{
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
        Done("");
#else
        auto result = BuildProjectForPlatform();

        if (result.IsSuccess())
        {
            emit Done();
        }
        else
        {
            emit Done(result.GetError());
        }
#endif
    }

    QString ProjectBuilderWorker::GetLogFilePath() const
    {
        QDir logFilePath(m_projectInfo.m_path);
        // Make directories if they aren't on disk
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

    void ProjectBuilderWorker::QStringToAZTracePrint([[maybe_unused]] const QString& error)
    {
        AZ_TracePrintf("Project Manager", error.toStdString().c_str());
    }

    AZ::Outcome<void, QString> ProjectBuilderWorker::BuildProjectForPlatform()
    {
        // Check if we are trying to cancel task
        if (QThread::currentThread()->isInterruptionRequested())
        {
            QStringToAZTracePrint(BuildCancelled);
            return AZ::Failure(BuildCancelled);
        }

        QFile logFile(GetLogFilePath());
        if (!logFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        {
            QString error = tr("Failed to open log file.");
            QStringToAZTracePrint(error);
            return AZ::Failure(error);
        }

        EngineInfo engineInfo;

        AZ::Outcome<EngineInfo> engineInfoResult = PythonBindingsInterface::Get()->GetEngineInfo();
        if (engineInfoResult.IsSuccess())
        {
            engineInfo = engineInfoResult.GetValue();
        }
        else
        {
            QString error = tr("Failed to get engine info.");
            QStringToAZTracePrint(error);
            return AZ::Failure(error);
        }

        QTextStream logStream(&logFile);
        if (QThread::currentThread()->isInterruptionRequested())
        {
            logFile.close();
            QStringToAZTracePrint(BuildCancelled);
            return AZ::Failure(BuildCancelled);
        }

        // Show some kind of progress with very approximate estimates
        UpdateProgress(++m_progressEstimate);

        auto currentEnvironmentRequest = ProjectUtils::SetupCommandLineProcessEnvironment();
        if (!currentEnvironmentRequest.IsSuccess())
        {
            QStringToAZTracePrint(currentEnvironmentRequest.GetError());
            return AZ::Failure(currentEnvironmentRequest.GetError());
        }

        m_configProjectProcess = new QProcess(this);
        m_configProjectProcess->setProcessChannelMode(QProcess::MergedChannels);
        m_configProjectProcess->setWorkingDirectory(m_projectInfo.m_path);

        auto cmakeGenerateArgumentsResult = ConstructCmakeGenerateProjectArguments(engineInfo.m_thirdPartyPath);
        if (!cmakeGenerateArgumentsResult.IsSuccess())
        {
            QStringToAZTracePrint(cmakeGenerateArgumentsResult.GetError());
            return AZ::Failure(cmakeGenerateArgumentsResult.GetError());
        }
        auto cmakeGenerateArguments = cmakeGenerateArgumentsResult.GetValue();
        m_configProjectProcess->start(cmakeGenerateArguments.front(), cmakeGenerateArguments.mid(1));
        if (!m_configProjectProcess->waitForStarted())
        {
            QString error = tr("Configuring project failed to start.");
            QStringToAZTracePrint(error);
            return AZ::Failure(error);
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
                QStringToAZTracePrint(BuildCancelled);
                return AZ::Failure(BuildCancelled);
            }
        }

        if (m_configProjectProcess->exitStatus() != QProcess::ExitStatus::NormalExit || m_configProjectProcess->exitCode() != 0 ||
            !containsGeneratingDone)
        {
            QString error = tr("Configuring project failed. See log for details.");
            QStringToAZTracePrint(error);
            return AZ::Failure(error);
        }

        UpdateProgress(++m_progressEstimate);

        m_buildProjectProcess = new QProcess(this);
        m_buildProjectProcess->setProcessChannelMode(QProcess::MergedChannels);
        m_buildProjectProcess->setWorkingDirectory(m_projectInfo.m_path);

        auto cmakeBuildArgumentsResult = ConstructCmakeBuildCommandArguments();
        if (!cmakeBuildArgumentsResult.IsSuccess())
        {
            QStringToAZTracePrint(cmakeBuildArgumentsResult.GetError());
            return AZ::Failure(cmakeBuildArgumentsResult.GetError());
        }
        auto cmakeBuildArguments = cmakeBuildArgumentsResult.GetValue();

        m_buildProjectProcess->start(cmakeBuildArguments.front(), cmakeBuildArguments.mid(1));
        if (!m_buildProjectProcess->waitForStarted())
        {
            QString error = tr("Building project failed to start.");
            QStringToAZTracePrint(error);
            return AZ::Failure(error);
        }

        // There are a lot of steps when building so estimate around 800 more steps ((100 - 20) * 10) remaining
        m_progressEstimate = 200;
        while (m_buildProjectProcess->waitForReadyRead(MaxBuildTimeMSecs))
        {
            logStream << m_buildProjectProcess->readAllStandardOutput();
            logStream.flush();

            // Show 1% progress for every 10 steps completed
            UpdateProgress(qMin(++m_progressEstimate / 10, 99));

            if (QThread::currentThread()->isInterruptionRequested())
            {
                // QProcess is unable to kill its child processes so we need to ask the operating system to do that for us
                auto killProcessArgumentsResult = ConstructKillProcessCommandArguments(QString::number(m_buildProjectProcess->processId()));
                if (!killProcessArgumentsResult.IsSuccess())
                {
                    return AZ::Failure(killProcessArgumentsResult.GetError());
                }
                auto killProcessArguments = killProcessArgumentsResult.GetValue();


                QProcess killBuildProcess;


                killBuildProcess.setProcessChannelMode(QProcess::MergedChannels);
                killBuildProcess.start(killProcessArguments.front(), killProcessArguments.mid(1));
                killBuildProcess.waitForFinished();

                logStream << "Killing Project Build.";
                logStream << killBuildProcess.readAllStandardOutput();
                m_buildProjectProcess->kill();
                logFile.close();
                QStringToAZTracePrint(BuildCancelled);
                return AZ::Failure(BuildCancelled);
            }
        }

        if (m_buildProjectProcess->exitStatus() != QProcess::ExitStatus::NormalExit || m_buildProjectProcess->exitCode() != 0)
        {
            QString error = tr("Building project failed. See log for details.");
            QStringToAZTracePrint(error);
            return AZ::Failure(error);
        }

        return AZ::Success();
    }

} // namespace O3DE::ProjectManager
