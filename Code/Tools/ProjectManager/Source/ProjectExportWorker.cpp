/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ProjectExportWorker.h>
#include <ProjectManagerDefs.h>
#include <PythonBindingsInterface.h>
#include <ProjectUtils.h>

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTextStream>
#include <QThread>

namespace O3DE::ProjectManager
{
    ProjectExportWorker::ProjectExportWorker(const ProjectInfo& projectInfo)
        : QObject()
        , m_projectInfo(projectInfo)
    {
    }

    void ProjectExportWorker::ExportProject()
    {
        auto result = ExportProjectForPlatform();

        if (result.IsSuccess())
        {
            emit Done();
        }
        else
        {
            emit Done(result.GetError());
        }
    }

    QString ProjectExportWorker::GetLogFilePath() const
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
        return logFilePath.filePath(ProjectExportErrorLogName);
    }

    void ProjectExportWorker::QStringToAZTracePrint([[maybe_unused]] const QString& error)
    {
        AZ_TracePrintf("Project Manager", error.toStdString().c_str());
    }

    //TODO: update this code to just run the desired worker process with the project path argument
    AZ::Outcome<void, QString> ProjectExportWorker::ExportProjectForPlatform()
    {
        // Check if we are trying to cancel task
        if (QThread::currentThread()->isInterruptionRequested())
        {
            QStringToAZTracePrint(ExportCancelled);
            return AZ::Failure(ExportCancelled);
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
            QStringToAZTracePrint(ExportCancelled);
            return AZ::Failure(ExportCancelled);
        }

        UpdateProgress(tr("Setting Up Environment"));

        auto currentEnvironmentRequest = ProjectUtils::SetupCommandLineProcessEnvironment();
        if (!currentEnvironmentRequest.IsSuccess())
        {
            QStringToAZTracePrint(currentEnvironmentRequest.GetError());
            return AZ::Failure(currentEnvironmentRequest.GetError());
        }

        

        m_exportProjectProcess = new QProcess(this);
        m_exportProjectProcess->setProcessChannelMode(QProcess::MergedChannels);
        m_exportProjectProcess->setWorkingDirectory(m_projectInfo.m_path);

        
        QStringList exportArgs = {"export-project","--project-path", m_projectInfo.m_path};
        QString commandProgram = QDir(engineInfo.m_path).filePath(GetO3DECLIString());

        m_exportProjectProcess->start(commandProgram, exportArgs);
        if (!m_exportProjectProcess->waitForStarted())
        {
            QString error = tr("Exporting project failed to start.");
            QStringToAZTracePrint(error);
            return AZ::Failure(error);
        }

        while (m_exportProjectProcess->waitForReadyRead(MaxExportTimeMSecs))
        {
            QString exportOutput = m_exportProjectProcess->readAllStandardOutput();

            logStream << exportOutput;
            logStream.flush();

            // Show last line of output
            if (QStringList strs = exportOutput.split('\n', Qt::SkipEmptyParts);
                !strs.empty())
            {
                UpdateProgress(strs.last());
            }

            if (QThread::currentThread()->isInterruptionRequested())
            {
                // QProcess is unable to kill its child processes so we need to ask the operating system to do that for us
                auto killProcessArgumentsResult = ConstructKillProcessCommandArguments(QString::number(m_exportProjectProcess->processId()));
                if (!killProcessArgumentsResult.IsSuccess())
                {
                    return AZ::Failure(killProcessArgumentsResult.GetError());
                }
                auto killProcessArguments = killProcessArgumentsResult.GetValue();


                QProcess killExportProcess;


                killExportProcess.setProcessChannelMode(QProcess::MergedChannels);
                killExportProcess.start(killProcessArguments.front(), killProcessArguments.mid(1));
                killExportProcess.waitForFinished();

                logStream << "Killing Project Export.";
                logStream << killExportProcess.readAllStandardOutput();
                m_exportProjectProcess->kill();
                logFile.close();
                QStringToAZTracePrint(ExportCancelled);
                return AZ::Failure(ExportCancelled);
            }
        }

        if (m_exportProjectProcess->exitStatus() != QProcess::ExitStatus::NormalExit || m_exportProjectProcess->exitCode() != 0)
        {
            QString error = tr("Building project failed. See log for details.");
            QStringToAZTracePrint(error);
            return AZ::Failure(error);
        }

        return AZ::Success();
    }

} // namespace O3DE::ProjectManager
