/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ProjectBuilderWorker.h>
#include <ProjectManagerDefs.h>
#include <PythonBindingsInterface.h>

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTextStream>
#include <QThread>

namespace O3DE::ProjectManager
{
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
                "-DLY_3RDPARTY_PATH=" + engineInfo.m_thirdPartyPath,
                "-DLY_UNITY_BUILD=1"
            });

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

        if (m_configProjectProcess->exitCode() != 0 || !containsGeneratingDone)
        {
            QString error = tr("Configuring project failed. See log for details.");
            QStringToAZTracePrint(error);
            return AZ::Failure(error);
        }

        UpdateProgress(++m_progressEstimate);

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
                QProcess killBuildProcess;
                killBuildProcess.setProcessChannelMode(QProcess::MergedChannels);
                killBuildProcess.start(
                    "cmd.exe", QStringList{ "/C", "taskkill", "/pid", QString::number(m_buildProjectProcess->processId()), "/f", "/t" });
                killBuildProcess.waitForFinished();

                logStream << "Killing Project Build.";
                logStream << killBuildProcess.readAllStandardOutput();
                m_buildProjectProcess->kill();
                logFile.close();
                QStringToAZTracePrint(BuildCancelled);
                return AZ::Failure(BuildCancelled);
            }
        }

        if (m_configProjectProcess->exitCode() != 0)
        {
            QString error = tr("Building project failed. See log for details.");
            QStringToAZTracePrint(error);
            return AZ::Failure(error);
        }

        return AZ::Success();
    }

} // namespace O3DE::ProjectManager
