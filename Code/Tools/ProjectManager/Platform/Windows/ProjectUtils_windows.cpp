/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ProjectUtils.h>

#include <PythonBindingsInterface.h>

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>

namespace O3DE::ProjectManager
{
    namespace ProjectUtils
    {
        AZ::Outcome<QProcessEnvironment, QString> GetCommandLineProcessEnvironment()
        {
            // Use the engine path to insert a path for cmake
            auto engineInfoResult = PythonBindingsInterface::Get()->GetEngineInfo();
            if (!engineInfoResult.IsSuccess())
            {
                return AZ::Failure(QObject::tr("Failed to get engine info"));    
            }
            auto engineInfo = engineInfoResult.GetValue();

            QProcessEnvironment currentEnvironment(QProcessEnvironment::systemEnvironment());

            // Append cmake path to PATH incase it is missing
            QDir cmakePath(engineInfo.m_path);
            cmakePath.cd("cmake/runtime/bin");
            QString pathValue = currentEnvironment.value("PATH");
            pathValue += ";" + cmakePath.path();
            currentEnvironment.insert("PATH", pathValue);
            return AZ::Success(currentEnvironment);
        }

        AZ::Outcome<QString, QString> FindSupportedCompilerForPlatform()
        {
            // Validate that cmake is installed 
            auto cmakeProcessEnvResult = GetCommandLineProcessEnvironment();
            if (!cmakeProcessEnvResult.IsSuccess())
            {
                return AZ::Failure(cmakeProcessEnvResult.GetError());
            }
            auto cmakeVersionQueryResult = ExecuteCommandResult("cmake", QStringList{"--version"}, cmakeProcessEnvResult.GetValue());
            if (!cmakeVersionQueryResult.IsSuccess())
            {
                return AZ::Failure(QObject::tr("CMake not found. \n\n"
                    "Make sure that the minimum version of CMake is installed and available from the command prompt. "
                    "Refer to the <a href='https://o3de.org/docs/welcome-guide/setup/requirements/#cmake'>O3DE requirements</a> for more information."));
            }

            // Validate that the minimal version of visual studio is installed
            QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
            QString programFilesPath = environment.value("ProgramFiles(x86)");
            QString vsWherePath = QDir(programFilesPath).filePath("Microsoft Visual Studio/Installer/vswhere.exe");

            QFileInfo vsWhereFile(vsWherePath);
            if (vsWhereFile.exists() && vsWhereFile.isFile())
            {
                QStringList vsWhereBaseArguments = QStringList{"-version",
                                                               "16.9.2",
                                                               "-latest",
                                                               "-requires",
                                                               "Microsoft.VisualStudio.Component.VC.Tools.x86.x64"};

                QProcess vsWhereIsCompleteProcess;
                vsWhereIsCompleteProcess.setProcessChannelMode(QProcess::MergedChannels);

                vsWhereIsCompleteProcess.start(vsWherePath, vsWhereBaseArguments + QStringList{ "-property", "isComplete" });

                if (vsWhereIsCompleteProcess.waitForStarted() && vsWhereIsCompleteProcess.waitForFinished())
                {
                    QString vsWhereIsCompleteOutput(vsWhereIsCompleteProcess.readAllStandardOutput());
                    if (vsWhereIsCompleteOutput.startsWith("1"))
                    {
                        QProcess vsWhereCompilerVersionProcess;
                        vsWhereCompilerVersionProcess.setProcessChannelMode(QProcess::MergedChannels);
                        vsWhereCompilerVersionProcess.start(vsWherePath, vsWhereBaseArguments + QStringList{"-property", "catalog_productDisplayVersion"});

                        if (vsWhereCompilerVersionProcess.waitForStarted() && vsWhereCompilerVersionProcess.waitForFinished())
                        {
                            QString vsWhereCompilerVersionOutput(vsWhereCompilerVersionProcess.readAllStandardOutput());
                            return AZ::Success(vsWhereCompilerVersionOutput);
                        }
                    }
                }
            }

            return AZ::Failure(QObject::tr("Visual Studio 2019 version 16.9.2 or higher not found.<br><br>"
                "Visual Studio 2019 is required to build this project."
                " Install any edition of <a href='https://visualstudio.microsoft.com/downloads/'>Visual Studio 2019</a>"
                " or update to a newer version before proceeding to the next step."
                " While installing configure Visual Studio with these <a href='https://o3de.org/docs/welcome-guide/setup/requirements/#visual-studio-configuration'>workloads</a>."));
        }
        
        AZ::Outcome<void, QString> OpenCMakeGUI(const QString& projectPath)
        {
            AZ::Outcome processEnvResult = GetCommandLineProcessEnvironment();
            if (!processEnvResult.IsSuccess())
            {
                return AZ::Failure(processEnvResult.GetError());
            }

            QString projectBuildPath = QDir(projectPath).filePath(ProjectBuildPathPostfix);
            AZ::Outcome projectBuildPathResult = GetProjectBuildPath(projectPath);
            if (projectBuildPathResult.IsSuccess())
            {
                projectBuildPath = projectBuildPathResult.GetValue();
            }

            QProcess process;
            process.setProcessEnvironment(processEnvResult.GetValue());

            // if the project build path is relative, it should be relative to the project path 
            process.setWorkingDirectory(projectPath);

            process.setProgram("cmake-gui");
            process.setArguments({ "-S", projectPath, "-B", projectBuildPath });
            if(!process.startDetached())
            {
                return AZ::Failure(QObject::tr("Failed to start CMake GUI"));
            }

            return AZ::Success();
        }

        AZ::Outcome<QString, QString> RunGetPythonScript(const QString& engineRoot)
        {
            const QString batPath = QString("%1/python/get_python.bat").arg(engineRoot);
            return ExecuteCommandResultModalDialog(
                "cmd.exe",
                QStringList{"/c", batPath},
                QProcessEnvironment::systemEnvironment(),
                QObject::tr("Running get_python script..."));
        }
    } // namespace ProjectUtils
} // namespace O3DE::ProjectManager
