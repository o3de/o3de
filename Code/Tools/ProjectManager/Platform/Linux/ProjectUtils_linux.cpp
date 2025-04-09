/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ProjectUtils.h>
#include <PythonBindingsInterface.h>

#include <QProcessEnvironment>
#include <QDir>

#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Utils/Utils.h>

namespace O3DE::ProjectManager
{
    namespace ProjectUtils
    {
        // The list of clang C/C++ compiler command lines to validate on the host Linux system
        // Only Ubuntu has clang++-<version> symlinks, other linux distros do not,
        // so a empty string entry is added to the end
        const QStringList SupportedClangVersions = {"-13", "-12", "-11", "-10", "-9", "-8", "-7", "-6.0", ""};

        AZ::Outcome<void, QString> SetupCommandLineProcessEnvironment()
        {
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
            // Query the version of cmake that is installed
            if (auto queryCmakeVersionQuery = FindSupportedCMake(); !queryCmakeVersionQuery.IsSuccess())
            {
                return queryCmakeVersionQuery;
            }

            // Look for the first compatible version of clang. The list below will contain the known clang compilers that have been tested
            // for O3DE.
            for (const QString& supportClangVersion : SupportedClangVersions)
            {
                auto whichClangResult =
                    ProjectUtils::ExecuteCommandResult("which", QStringList{ QString("clang%1").arg(supportClangVersion) });
                auto whichClangPPResult =
                    ProjectUtils::ExecuteCommandResult("which", QStringList{ QString("clang++%1").arg(supportClangVersion) });
                if (whichClangResult.IsSuccess() && whichClangPPResult.IsSuccess())
                {
                    return AZ::Success(QString("clang%1").arg(supportClangVersion));
                }
            }

            // Finally fallback to trying to detect gcc executables without a compiler version
            auto whichGccNoVersionResult = ProjectUtils::ExecuteCommandResult("which", QStringList{ QString("gcc") });
            auto whichGPlusPlusNoVersionResult = ProjectUtils::ExecuteCommandResult("which", QStringList{ QString("g++") });
            if (whichGccNoVersionResult.IsSuccess() && whichGPlusPlusNoVersionResult.IsSuccess())
            {
                return AZ::Success(QString("gcc"));
            }

            return AZ::Failure(QObject::tr("Neither clang nor gcc not found. <br><br>"
                                           "Make sure that the clang or gcc is installed and available from the command prompt. "
                                           "Refer to the <a href='https://o3de.org/docs/welcome-guide/setup/requirements/#cmake'>O3DE "
                                           "requirements</a> page for more information."));
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
                    AZStd::fixed_vector<AZ::IO::FixedMaxPath, 4> paths = {
                        buildConfigurationPath / AZ_BUILD_CONFIGURATION_TYPE / "Editor",
                        buildConfigurationPath / AZ_TRAIT_OS_PLATFORM_CODENAME / AZ_BUILD_CONFIGURATION_TYPE / "Editor"
                    };

                    // always try profile config because that is the default
                    if (strcmp(AZ_BUILD_CONFIGURATION_TYPE, "profile") != 0)
                    {
                        paths.emplace_back(buildConfigurationPath / "profile" / "Editor");
                        paths.emplace_back(buildConfigurationPath / AZ_TRAIT_OS_PLATFORM_CODENAME / "profile" / "Editor");
                    }

                    for (auto& path : paths)
                    {
                        if(AZ::IO::SystemFile::Exists(path.ReplaceExtension(AZ_TRAIT_OS_EXECUTABLE_EXTENSION).c_str()))
                        {
                            return path;
                        }
                    }
                }
            }

            // No Editor executable was found in the project build folder so if this project uses a
            // different engine we must find the Editor executable for that engine
            if(auto engineResult = PythonBindingsInterface::Get()->GetProjectEngine(projectPath.Native().data()); engineResult)
            {
                auto engineInfo = engineResult.GetValue<EngineInfo>();
                if (!engineInfo.m_thisEngine)
                {
                    AZ::IO::FixedMaxPath fixedEnginePath{ engineInfo.m_path.toUtf8().constData() };
                    // try the default sdk path
                    // in the future we may be able to use additional .setreg entries to locate an alternate binary path
                    if (editorPath = (fixedEnginePath / "bin" / AZ_TRAIT_OS_PLATFORM_CODENAME / "profile" / "Default" / "Editor").
                        ReplaceExtension(AZ_TRAIT_OS_EXECUTABLE_EXTENSION);
                        AZ::IO::SystemFile::Exists(editorPath.c_str()))
                    {
                        return editorPath;
                    }
                    return {};
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

        AZ::Outcome<QString, QString> CreateDesktopShortcut([[maybe_unused]] const QString& filename, [[maybe_unused]] const QString& targetPath, [[maybe_unused]] const QStringList& arguments)
        {
            return AZ::Failure(QObject::tr("Creating desktop shortcuts functionality not implemented for this platform yet."));
        }
    } // namespace ProjectUtils
} // namespace O3DE::ProjectManager
