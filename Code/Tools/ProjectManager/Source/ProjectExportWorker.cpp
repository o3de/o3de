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
#include <QRegExp>
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

    AZ::Outcome<QString, QString> ProjectExportWorker::GetLogFilePath() const
    {
        QDir logFilePath(m_projectInfo.m_path);
        // Make directories if they aren't on disk
        if (!logFilePath.cd(ProjectBuildPathPostfix))
        {
            if (!logFilePath.mkpath(ProjectBuildPathPostfix))
            {
                QString errorMessage = QString("Unable to make log directory '%1' for project build path")
                                        .arg(logFilePath.absoluteFilePath(ProjectBuildPathPostfix).toUtf8().constData());
                return AZ::Failure(errorMessage);
            }
            logFilePath.cd(ProjectBuildPathPostfix);
        }
        if (!logFilePath.cd(ProjectBuildPathCmakeFiles))
        {
            if (!logFilePath.mkpath(ProjectBuildPathCmakeFiles))
            {
                QString errorMessage = QString("Unable to make log directory '%1' for cmake files path")
                                        .arg(logFilePath.absoluteFilePath(ProjectBuildPathCmakeFiles).toUtf8().constData());
                return AZ::Failure(errorMessage);
            }
            logFilePath.cd(ProjectBuildPathCmakeFiles);
        }
        return AZ::Success(logFilePath.filePath(ProjectExportErrorLogName));
    }

    AZ::Outcome<QString, QString> ProjectExportWorker::GetExpectedOutputPath() const
    {
        if (m_expectedOutputDir.isEmpty())
        {
            return AZ::Failure(tr("Project Export output folder not detected in the output logs."));
        }

        if (!QDir(m_expectedOutputDir).isAbsolute())
        {
            return AZ::Failure(tr("Project Export output folder %s is invalid.").arg(m_expectedOutputDir));
        }

        return AZ::Success(m_expectedOutputDir);
    }

    void ProjectExportWorker::QStringToAZTracePrint([[maybe_unused]] const QString& error)
    {
        AZ_Trace("Project Manager", error.toStdString().c_str());
    }

    AZ::Outcome<void, QString> ProjectExportWorker::ExportProjectForPlatform()
    {
        AZ::Outcome<QString, QString> logFilePathQuery = GetLogFilePath();
        if(!logFilePathQuery.IsSuccess())
        {
            QString errorMessage = QString("%1: %2").arg(tr(ErrorMessages::LogPathFailureMsg)).arg(logFilePathQuery.GetError());
            QStringToAZTracePrint(errorMessage);
            return AZ::Failure(errorMessage);
        }
        QString logFilePath = logFilePathQuery.GetValue();
        QFile logFile(logFilePath);
        if (!logFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        {
            QString errorMessage = QString("%1: %2").arg(tr(ErrorMessages::LogOpenFailureMsg)).arg(logFilePath);
            QStringToAZTracePrint(errorMessage);
            return AZ::Failure(errorMessage);
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

        UpdateProgress(tr("Setting Up Environment"));

        auto currentEnvironmentRequest = ProjectUtils::SetupCommandLineProcessEnvironment();
        if (!currentEnvironmentRequest.IsSuccess())
        {
            QStringToAZTracePrint(currentEnvironmentRequest.GetError());
            return currentEnvironmentRequest;
        }

        

        m_exportProjectProcess = new QProcess(this);
        m_exportProjectProcess->setProcessChannelMode(QProcess::MergedChannels);
        m_exportProjectProcess->setWorkingDirectory(m_projectInfo.m_path);

        
        QStringList exportArgs = {"export-project", "--export-script", m_projectInfo.m_currentExportScript,
                                                    "--project-path", m_projectInfo.m_path};
        QString commandProgram = QDir(engineInfo.m_path).filePath(GetO3DECLIString());

        m_exportProjectProcess->start(commandProgram, exportArgs);
        if (!m_exportProjectProcess->waitForStarted())
        {
            QString error = tr("Exporting project failed to start.");
            QStringToAZTracePrint(error);
            return AZ::Failure(error);
        }

        QString exportOutput;

        while (m_exportProjectProcess->waitForReadyRead(MaxExportTimeMSecs))
        {
            exportOutput = m_exportProjectProcess->readAllStandardOutput();

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
                AZ::Outcome<QStringList, QString> killProcessArgumentsResult = ConstructKillProcessCommandArguments(QString::number(m_exportProjectProcess->processId()));
                if (!killProcessArgumentsResult.IsSuccess())
                {
                    return AZ::Failure(killProcessArgumentsResult.GetError());
                }
                auto killProcessArguments = killProcessArgumentsResult.GetValue();

                QProcess killExportProcess;
                killExportProcess.setProcessChannelMode(QProcess::MergedChannels);
                killExportProcess.start(killProcessArguments.front(), killProcessArguments.mid(1));
                bool finishedKilling = killExportProcess.waitForFinished();

                logStream << "Killing Project Export.";
                logStream << killExportProcess.readAllStandardOutput();

                if (!finishedKilling)
                {
                    killExportProcess.kill();
                }

                m_exportProjectProcess->kill();
                logFile.close();
                QStringToAZTracePrint(ErrorMessages::ExportCancelled);
                return AZ::Failure(ErrorMessages::ExportCancelled);
            }
        }

        //check for edge case where a single wait cycle causes premature termination of the loop
        //force the interruption in this case
        if (m_exportProjectProcess->waitForReadyRead(MaxExportTimeMSecs))
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
            bool finishedKilling = killExportProcess.waitForFinished();

            if (!finishedKilling)
            {
                killExportProcess.kill();
            }

            logStream << "Project Export took too long to respond. Terminating...";
            logStream << killExportProcess.readAllStandardOutput();
            m_exportProjectProcess->kill();
            logFile.close();
        }

        if (m_exportProjectProcess->exitStatus() != QProcess::ExitStatus::NormalExit || m_exportProjectProcess->exitCode() != 0)
        {
            QString error = tr("Exporting project failed. See log for details. %1").arg(logFilePath);
            QStringToAZTracePrint(error);
            return AZ::Failure(error);
        }

        // Fetch the output directory from the logs if the process was successful
        // Use regex to collect that directory, and collect the slice of the word surrounded in quotes
        
        QRegExp quotedDirRegex("Project exported to '([^']*)'\\.");
        int pos = quotedDirRegex.indexIn(exportOutput);
        if (pos > -1)
        {
            m_expectedOutputDir = quotedDirRegex.cap(1);
        }
        
        return AZ::Success();
    }

} // namespace O3DE::ProjectManager
