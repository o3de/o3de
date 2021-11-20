/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ProjectUtils.h>

#include <QProcess>
#include <QStandardPaths>
#include <QDir>

#include <AzCore/Utils/Utils.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>

namespace O3DE::ProjectManager
{
    namespace ProjectUtils
    {
        AZ::Outcome<void, QString> SetupCommandLineProcessEnvironment()
        {
            // For CMake on Mac, if its installed through home-brew, then it will be installed
            // under /usr/local/bin, which may not be in the system PATH environment. 
            // Add that path for the command line process so that it will be able to locate
            // a home-brew installed version of CMake
            QString pathEnv = qEnvironmentVariable("PATH");
            if (!pathEnv.contains("/usr/local/bin"))
            {
                pathEnv += ":/usr/local/bin";
                qputenv("PATH", pathEnv.toStdString().c_str());
            }

            return AZ::Success();
        }

        AZ::Outcome<QString, QString> FindSupportedCompilerForPlatform()
        {
            AZ::Outcome processEnvResult = SetupCommandLineProcessEnvironment();
            if (!processEnvResult.IsSuccess())
            {
                return AZ::Failure(processEnvResult.GetError());
            }

            // Validate that we have cmake installed first
            auto queryCmakeInstalled = ExecuteCommandResult("which", QStringList{ProjectCMakeCommand});
            if (!queryCmakeInstalled.IsSuccess())
            {
                return AZ::Failure(QObject::tr("Unable to detect CMake on this host."));
            }
            QString cmakeInstalledPath = queryCmakeInstalled.GetValue().split("\n")[0];

            // Query the version of the installed cmake
            auto queryCmakeVersionQuery = ExecuteCommandResult(cmakeInstalledPath, QStringList{"-version"});
            if (!queryCmakeVersionQuery.IsSuccess())
            {
                return AZ::Failure(QObject::tr("Unable to determine the version of CMake on this host."));
            }
            AZ_TracePrintf("Project Manager", "Cmake version %s detected.", queryCmakeVersionQuery.GetValue().split("\n")[0].toUtf8().constData());

            // Query for the version of xcodebuild (if installed)
            auto queryXcodeBuildVersion = ExecuteCommandResult("xcodebuild", QStringList{"-version"});
            if (!queryCmakeInstalled.IsSuccess())
            {
                return AZ::Failure(QObject::tr("Unable to detect XCodeBuilder on this host."));
            }
            QString xcodeBuilderVersionNumber = queryXcodeBuildVersion.GetValue().split("\n")[0];
            AZ_TracePrintf("Project Manager", "XcodeBuilder version %s detected.", xcodeBuilderVersionNumber.toUtf8().constData());


            return AZ::Success(xcodeBuilderVersionNumber);
        }       

        AZ::Outcome<void, QString> OpenCMakeGUI(const QString& projectPath)
        {
            const QString cmakeHelp = QObject::tr("Please verify you've installed CMake.app from " 
                            "<a href=\"https://cmake.org\">cmake.org</a> or, if using HomeBrew, "
                            "have installed it with <pre>brew install --cask cmake</pre>");
            QString cmakeAppPath = QStandardPaths::locate(QStandardPaths::ApplicationsLocation, "CMake.app", QStandardPaths::LocateDirectory);
            if (cmakeAppPath.isEmpty())
            {
                return AZ::Failure(QObject::tr("CMake.app not found.") + cmakeHelp);
            }

            QString projectBuildPath = QDir(projectPath).filePath(ProjectBuildPathPostfix);
            AZ::Outcome result = GetProjectBuildPath(projectPath);
            if (result.IsSuccess())
            {
                projectBuildPath = result.GetValue();
            }

            QProcess process;

            // if the project build path is relative, it should be relative to the project path 
            process.setWorkingDirectory(projectPath);
            process.setProgram("open");
            process.setArguments({"-a", "CMake", "--args", "-S", projectPath, "-B", projectBuildPath});
            if(!process.startDetached())
            {
                return AZ::Failure(QObject::tr("CMake.app failed to open.") + cmakeHelp);
            }

            return AZ::Success();
        }
        
        AZ::Outcome<QString, QString> RunGetPythonScript(const QString& engineRoot)
        {
            return ExecuteCommandResultModalDialog(
                QString("%1/python/get_python.sh").arg(engineRoot),
                {},
                QObject::tr("Running get_python script..."));
        }

        AZ::IO::FixedMaxPath GetEditorDirectory()
        {
            AZ::IO::FixedMaxPath executableDirectory = AZ::Utils::GetExecutableDirectory();
            AZ::IO::FixedMaxPath editorPath{ executableDirectory };
            editorPath /= "../../../Editor.app/Contents/MacOS";
            editorPath = editorPath.LexicallyNormal();
            if (!AZ::IO::SystemFile::IsDirectory(editorPath.c_str()))
            {
                if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
                {
                    if (AZ::IO::FixedMaxPath installedBinariesPath;
                        settingsRegistry->Get(installedBinariesPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_InstalledBinaryFolder))
                    {
                        if (AZ::IO::FixedMaxPath engineRootFolder;
                            settingsRegistry->Get(engineRootFolder.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder))
                        {
                            editorPath = engineRootFolder / installedBinariesPath / "Editor.app/Contents/MacOS";
                        }
                    }
                }

                if (!AZ::IO::SystemFile::IsDirectory(editorPath.c_str()))
                {
                    AZ_Error("ProjectManager", false, "Unable to find the Editor app bundle!");
                }
            }

            return editorPath;
        }

        AZ::Outcome<QString, QString> CreateDesktopShortcut([[maybe_unused]] const QString& filename, [[maybe_unused]] const QString& targetPath, [[maybe_unused]] const QStringList& arguments)
        {
            return AZ::Failure(QObject::tr("Creating desktop shortcuts functionality not implemented for this platform yet."));
        }
    } // namespace ProjectUtils
} // namespace O3DE::ProjectManager
