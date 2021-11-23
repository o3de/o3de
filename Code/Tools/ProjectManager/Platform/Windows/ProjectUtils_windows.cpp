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
#include <QStandardPaths>

#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Utils/Utils.h>

namespace O3DE::ProjectManager
{
    namespace ProjectUtils
    {
        AZ::Outcome<void, QString> SetupCommandLineProcessEnvironment()
        {
            // Use the engine path to insert a path for cmake
            auto engineInfoResult = PythonBindingsInterface::Get()->GetEngineInfo();
            if (!engineInfoResult.IsSuccess())
            {
                return AZ::Failure(QObject::tr("Failed to get engine info"));
            }
            auto engineInfo = engineInfoResult.GetValue();

            // Append cmake path to the current environment PATH incase it is missing, since if
            // we are starting CMake itself the current application needs to find it using Path
            // This also takes affect for all child processes.
            QDir cmakePath(engineInfo.m_path);
            cmakePath.cd("cmake/runtime/bin");
            QString pathEnv = qEnvironmentVariable("Path");
            QStringList pathEnvList = pathEnv.split(";");
            if (!pathEnvList.contains(cmakePath.path()))
            {
                pathEnv += ";" + cmakePath.path();
                if (!qputenv("Path", pathEnv.toStdString().c_str()))
                {
                    return AZ::Failure(QObject::tr("Failed to set Path environment variable"));
                }
            }

            return AZ::Success();
        }

        AZ::Outcome<QString, QString> FindSupportedCompilerForPlatform()
        {
            // Validate that cmake is installed
            auto cmakeProcessEnvResult = SetupCommandLineProcessEnvironment();
            if (!cmakeProcessEnvResult.IsSuccess())
            {
                return AZ::Failure(cmakeProcessEnvResult.GetError());
            }
            auto cmakeVersionQueryResult = ExecuteCommandResult("cmake", QStringList{"--version"});
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
            AZ::Outcome processEnvResult = SetupCommandLineProcessEnvironment();
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
                QObject::tr("Running get_python script..."));
        }

        AZ::IO::FixedMaxPath GetEditorExecutablePath(const AZ::IO::PathView& projectPath)
        {
            AZ::IO::FixedMaxPath editorPath;
            AZ::IO::FixedMaxPath fixedProjectPath{ projectPath };
            // First attempt to launch the Editor.exe within the project build directory if it exists
            AZ::IO::FixedMaxPath buildPathSetregPath = fixedProjectPath
                / AZ::SettingsRegistryInterface::DevUserRegistryFolder
                / "Platform" / AZ_TRAIT_OS_PLATFORM_CODENAME / "build_path.setreg";
            if (AZ::IO::SystemFile::Exists(buildPathSetregPath.c_str()))
            {
                AZ::SettingsRegistryImpl settingsRegistry;
                // Merge the build_path.setreg into the local SettingsRegistry instance
                if (AZ::IO::FixedMaxPath projectBuildPath;
                    settingsRegistry.MergeSettingsFile(buildPathSetregPath.Native(),
                        AZ::SettingsRegistryInterface::Format::JsonMergePatch)
                    && settingsRegistry.Get(projectBuildPath.Native(), AZ::SettingsRegistryMergeUtils::ProjectBuildPath))
                {
                    // local Settings Registry will be used to merge the build_path.setreg for the supplied projectPath
                    AZ::IO::FixedMaxPath buildConfigurationPath = (fixedProjectPath / projectBuildPath).LexicallyNormal();

                    // First try <project-build-path>/bin/$<CONFIG> and if that path doesn't exist
                    // try <project-build-path>/bin/$<PLATFORM>/$<CONFIG>
                    buildConfigurationPath /= "bin";
                    if (editorPath = (buildConfigurationPath / AZ_BUILD_CONFIGURATION_TYPE / "Editor").
                        ReplaceExtension(AZ_TRAIT_OS_EXECUTABLE_EXTENSION);
                        AZ::IO::SystemFile::Exists(editorPath.c_str()))
                    {
                        return editorPath;
                    }
                    else if (editorPath = (buildConfigurationPath / AZ_TRAIT_OS_PLATFORM_CODENAME
                        / AZ_BUILD_CONFIGURATION_TYPE / "Editor").
                        ReplaceExtension(AZ_TRAIT_OS_EXECUTABLE_EXTENSION);
                        AZ::IO::SystemFile::Exists(editorPath.c_str()))
                    {
                        return editorPath;
                    }
                }
            }

            // Fall back to checking if an Editor exists in O3DE executable directory
            editorPath = AZ::IO::FixedMaxPath(AZ::Utils::GetExecutableDirectory()) / "Editor";
            editorPath.ReplaceExtension(AZ_TRAIT_OS_EXECUTABLE_EXTENSION);
            if (AZ::IO::SystemFile::Exists(editorPath.c_str()))
            {
                return editorPath;
            }
            return {};
        }

        AZ::Outcome<QString, QString> CreateDesktopShortcut(const QString& filename, const QString& targetPath, const QStringList& arguments)
        {
            const QString cmd{"powershell.exe"};
            const QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
            const QString shortcutPath = QString("%1/%2.lnk").arg(desktopPath).arg(filename);
            const QString arg = QString("$s=(New-Object -COM WScript.Shell).CreateShortcut('%1');$s.TargetPath='%2';$s.Arguments='%3';$s.Save();")
                    .arg(shortcutPath)
                    .arg(targetPath)
                    .arg(arguments.join(' '));
            auto createShortcutResult = ExecuteCommandResult(cmd, QStringList{"-Command", arg});
            if (!createShortcutResult.IsSuccess())
            {
                return AZ::Failure(QObject::tr("Failed to create desktop shortcut %1 <br><br>"
                                               "Please verify you have permission to create files at the specified location.<br><br> %2")
                                    .arg(shortcutPath)
                                    .arg(createShortcutResult.GetError()));
            }

            return AZ::Success(QObject::tr("Desktop shortcut created at<br><a href=\"%1\">%2</a>").arg(desktopPath).arg(shortcutPath));
        }
    } // namespace ProjectUtils
} // namespace O3DE::ProjectManager
