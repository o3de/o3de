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

#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Utils/Utils.h>

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
            QStringList pathEnvList = pathEnv.split(":");
            if (!pathEnvList.contains("/usr/local/bin"))
            {
                pathEnv += ":/usr/local/bin";
                if (!qputenv("PATH", pathEnv.toStdString().c_str()))
                {
                    return AZ::Failure(QObject::tr("Failed to set PATH environment variable"));
                }
            }

            return AZ::Success();
        }

        AZ::Outcome<QString, QString> FindSupportedCMake()
        {
            // Validate that cmake is installed and is in the command line
            auto whichCMakeResult = ProjectUtils::ExecuteCommandResult("which", QStringList{ ProjectCMakeCommand });
            if (!whichCMakeResult.IsSuccess())
            {
                return AZ::Failure(
                    QObject::tr("CMake not found. <br><br>"
                                "Make sure that the minimum version of CMake is installed and available from the command prompt. "
                                "Refer to the <a href='https://o3de.org/docs/welcome-guide/setup/requirements/#cmake'>O3DE "
                                "requirements</a> page for more information."));
            }

            QString cmakeInstalledPath = whichCMakeResult.GetValue().split("\n")[0];

            // Query the version of the installed cmake
            auto queryCmakeVersionQuery = ExecuteCommandResult(cmakeInstalledPath, QStringList{ "--version" });
            if (queryCmakeVersionQuery.IsSuccess())
            {
                AZ_TracePrintf(
                    "Project Manager", "\"%s\" detected.", queryCmakeVersionQuery.GetValue().split("\n")[0].toUtf8().constData());
            }

            return AZ::Success(QString{ cmakeInstalledPath });
        }

        AZ::Outcome<QString, QString> FindSupportedCompilerForPlatform([[maybe_unused]] const ProjectInfo& projectInfo)
        {
            AZ::Outcome processEnvResult = SetupCommandLineProcessEnvironment();
            if (!processEnvResult.IsSuccess())
            {
                return AZ::Failure(processEnvResult.GetError());
            }

            // Query the version of the installed cmake
            if (auto queryCmakeVersionQuery = FindSupportedCMake(); !queryCmakeVersionQuery.IsSuccess())
            {
                return queryCmakeVersionQuery;
            }

            // Query for the version of xcodebuild (if installed)
            auto queryXcodeBuildVersion = ExecuteCommandResult("xcodebuild", QStringList{ "-version" });
            if (!queryXcodeBuildVersion.IsSuccess())
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
                AZ::SettingsRegistryImpl localRegistry;
                // Merge the build_path.setreg into the local SettingsRegistry instance
                if (AZ::IO::FixedMaxPath projectBuildPath;
                    localRegistry.MergeSettingsFile(buildPathSetregPath.Native(),
                        AZ::SettingsRegistryInterface::Format::JsonMergePatch)
                    && localRegistry.Get(projectBuildPath.Native(), AZ::SettingsRegistryMergeUtils::ProjectBuildPath))
                {
                    //  local Settings Registry will be used to merge the build_path.setreg for the supplied projectPath
                    AZ::IO::FixedMaxPath buildConfigurationPath = (fixedProjectPath / projectBuildPath).LexicallyNormal();

                    // First try "<project-build-path>/bin/$<CONFIG>/Editor.app/Contents/MacOS"
                    // Followed by "<project-build-path>/bin/$<PLATFORM>/$<CONFIG>/Editor.app/Contents/MacOS"
                    // Directory existence is checked in this case
                    buildConfigurationPath /= "bin";
                    if (editorPath = (buildConfigurationPath
                        / AZ_BUILD_CONFIGURATION_TYPE / "Editor.app/Contents/MacOS/Editor");
                        AZ::IO::SystemFile::Exists(editorPath.c_str()))
                    {
                        return editorPath;
                    }
                    else if (editorPath = (buildConfigurationPath / AZ_TRAIT_OS_PLATFORM_CODENAME
                        / AZ_BUILD_CONFIGURATION_TYPE / "Editor.app/Contents/MacOS/Editor");
                        AZ::IO::SystemFile::Exists(editorPath.c_str()))
                    {
                        return editorPath;
                    }
                }
            }

            // Fall back to locating the Editor.app bundle which should exists
            // outside of the current O3DE.app bundle
            editorPath = (AZ::IO::FixedMaxPath(AZ::Utils::GetExecutableDirectory()) /
                "../../../Editor.app/Contents/MacOS/Editor").LexicallyNormal();

            if (!AZ::IO::SystemFile::Exists(editorPath.c_str()))
            {
                // Attempt to search the O3DE.app global settings registry for an InstalledBinaryFolder
                // key which indicates the relative path to an SDK binary directory on MacOS
                if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
                {
                    if (AZ::IO::FixedMaxPath installedBinariesPath;
                        settingsRegistry->Get(installedBinariesPath.Native(),
                        AZ::SettingsRegistryMergeUtils::FilePathKey_InstalledBinaryFolder))
                    {
                        if (AZ::IO::FixedMaxPath engineRootFolder;
                            settingsRegistry->Get(engineRootFolder.Native(),
                            AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder))
                        {
                            editorPath = engineRootFolder / installedBinariesPath / "Editor.app/Contents/MacOS/Editor";
                        }
                    }
                }

                if (!AZ::IO::SystemFile::Exists(editorPath.c_str()))
                {
                    AZ_Error("ProjectManager", false, "Unable to find the Editor app bundle!");
                    return {};
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
