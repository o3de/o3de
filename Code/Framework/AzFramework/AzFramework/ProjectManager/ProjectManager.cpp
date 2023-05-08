/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/ProjectManager/ProjectManager.h>

#include <AzCore/IO/SystemFile.h>
#include <AzCore/Platform.h>
#include <AzCore/Settings/CommandLine.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Utils/Utils.h>

#include <AzFramework/AzFramework_Traits_Platform.h>
#include <AzFramework/Process/ProcessWatcher.h>

namespace AzFramework::ProjectManager
{
    AZ::IO::FixedMaxPath FindProjectPath(const int argc, char* argv[])
    {
        AZ::IO::FixedMaxPath projectRootPath;
        {
            // AZ::CommandLine and SettingsRegistryImpl is in block scope to make sure
            // that the allocated memory is cleaned up before destroying the SystemAllocator
            // at the end of the function
            AZ::CommandLine commandLine;
            commandLine.Parse(argc, argv);
            AZ::SettingsRegistryMergeUtils::ParseCommandLine(commandLine);

            // Store the Command line to the Setting Registry
            AZ::SettingsRegistryImpl settingsRegistry;
            AZ::SettingsRegistryMergeUtils::StoreCommandLineToRegistry(settingsRegistry, commandLine);
            AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_O3deUserRegistry(settingsRegistry, AZ_TRAIT_OS_PLATFORM_CODENAME, {});
            // Retrieve Command Line from Settings Registry, it may have been updated by the call to FindEngineRoot()
            // in MergeSettingstoRegistry_ConfigFile
            AZ::SettingsRegistryMergeUtils::GetCommandLineFromRegistry(settingsRegistry, commandLine);
            AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_CommandLine(settingsRegistry, commandLine, {});
            // Look for the engine first in case the project path is relative
            AZ::SettingsRegistryMergeUtils::FindEngineRoot(settingsRegistry);
            projectRootPath = AZ::SettingsRegistryMergeUtils::FindProjectRoot(settingsRegistry);
        }
        return projectRootPath;
    }

    // Check for a project name, if not found, attempt to launch project manager and shut down
    ProjectPathCheckResult CheckProjectPathProvided(const int argc, char* argv[])
    {
        const auto projectRootPath = FindProjectPath(argc, argv);
        // If we were able to locate a path to a project, we're done
        if (!projectRootPath.empty())
        {
            AZ::IO::FixedMaxPath projectJsonPath = projectRootPath / "project.json";
            if (AZ::IO::SystemFile::Exists(projectJsonPath.c_str()))
            {
                return ProjectPathCheckResult::ProjectPathFound;
            }

            constexpr size_t MaxMessageSize = 2048;
            AZStd::array<char, MaxMessageSize> msg;
            azsnprintf(msg.data(), msg.size(), "No project was found at '%s'.\n"
                "The Project Manager will be opened so you can choose a project.",
                projectJsonPath.c_str());

            AZ_TracePrintf("ProjectManager", msg.data());
            AZ::Utils::NativeErrorMessageBox("Project not found", msg.data());
        }
        else
        {
            AZ::Utils::NativeErrorMessageBox("Project not found",
                "No project path was provided or detected.\nPlease provide a --project-path argument.\n"
                "The Project Manager is being launched now so you can choose a project.");
        }

        if (LaunchProjectManager())
        {
            AZ_TracePrintf("ProjectManager", "Project Manager launched successfully, requesting exit.");
            return ProjectPathCheckResult::ProjectManagerLaunched;
        }
        AZ_Error("ProjectManager", false, "Project Manager failed to launch and no project selected!");
        return ProjectPathCheckResult::ProjectManagerLaunchFailed;
    }

    bool LaunchProjectManager([[maybe_unused]] const AZStd::vector<AZStd::string>& commandLineArgs)
    {
        bool launchSuccess = false;
#if (AZ_TRAIT_AZFRAMEWORK_USE_PROJECT_MANAGER)
        {
            AZStd::string filename = "o3de";
            AZ::IO::FixedMaxPath executablePath = AZ::Utils::GetExecutableDirectory();
            executablePath /= filename + AZ_TRAIT_OS_EXECUTABLE_EXTENSION;

            if (!AZ::IO::SystemFile::Exists(executablePath.c_str()))
            {
                constexpr size_t MaxMessageSize = 2048;
                AZStd::array<char, MaxMessageSize> msg;
                azsnprintf(msg.data(), msg.size(), 
                    "The Project Manager was not found at '%s'.\nPlease verify O3DE is installed correctly and/or built if compiled from source. ",
                    executablePath.c_str());

                AZ_Error("ProjectManager", false, msg.data());
                AZ::Utils::NativeErrorMessageBox("Project Manager not found", msg.data());
                return false;
            }

            AzFramework::ProcessLauncher::ProcessLaunchInfo processLaunchInfo;

            AZStd::vector<AZStd::string> launchCmd = { executablePath.String() };
            launchCmd.insert(launchCmd.end(), commandLineArgs.begin(), commandLineArgs.end());

            processLaunchInfo.m_commandlineParameters = AZStd::move(launchCmd);

            launchSuccess = AzFramework::ProcessLauncher::LaunchUnwatchedProcess(processLaunchInfo);
        }
#endif // #if defined(AZ_FRAMEWORK_USE_PROJECT_MANAGER)
        return launchSuccess;
    }

} // AzFramework::ProjectManager

