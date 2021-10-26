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
    AZStd::tuple<AZ::IO::FixedMaxPath, AZ::IO::FixedMaxPath> FindProjectAndEngineRootPaths(const int argc, char* argv[])
    {
        bool ownsAllocator = false;
        if (!AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady())
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
            ownsAllocator = true;
        }

        AZ::IO::FixedMaxPath projectRootPath;
        AZ::IO::FixedMaxPath engineRootPath;
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
            AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_CommandLine(settingsRegistry, commandLine, false);
            engineRootPath = AZ::SettingsRegistryMergeUtils::FindEngineRoot(settingsRegistry);
            projectRootPath = AZ::SettingsRegistryMergeUtils::FindProjectRoot(settingsRegistry);
        }
        if (ownsAllocator)
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
        }
        return AZStd::make_tuple(projectRootPath, engineRootPath);
    }

    // Check for a project name, if not found, attempt to launch project manager and shut down
    ProjectPathCheckResult CheckProjectPathProvided(const int argc, char* argv[])
    {
        auto [projectRootPath, engineRootPath] = FindProjectAndEngineRootPaths(argc, argv);
        // If we were able to locate a path to a project, we're done
        if (!projectRootPath.empty())
        {
            AZ::IO::FixedMaxPath projectJsonPath = engineRootPath / projectRootPath / "project.json";
            if (AZ::IO::SystemFile::Exists(projectJsonPath.c_str()))
            {
                return ProjectPathCheckResult::ProjectPathFound;
            }
            AZ_TracePrintf(
                "ProjectManager", "Did not find a project file at location '%s', launching the Project Manager...",
                projectJsonPath.c_str());
        }

        if (LaunchProjectManager())
        {
            AZ_TracePrintf("ProjectManager", "Project Manager launched successfully, requesting exit.");
            return ProjectPathCheckResult::ProjectManagerLaunched;
        }
        AZ_Error("ProjectManager", false, "Project Manager failed to launch and no project selected!");
        return ProjectPathCheckResult::ProjectManagerLaunchFailed;
    }

    bool LaunchProjectManager([[maybe_unused]]const AZStd::string& commandLineArgs)
    {
        bool launchSuccess = false;
#if (AZ_TRAIT_AZFRAMEWORK_USE_PROJECT_MANAGER)
        bool ownsSystemAllocator = false;
        if (!AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady())
        {
            ownsSystemAllocator = true;
            AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
        }
        {
            AZStd::string filename = "o3de";
            AZ::IO::FixedMaxPath executablePath = AZ::Utils::GetExecutableDirectory();
            executablePath /= filename + AZ_TRAIT_OS_EXECUTABLE_EXTENSION;

            if (!AZ::IO::SystemFile::Exists(executablePath.c_str()))
            {
                AZ_Error("ProjectManager", false, "%s not found", executablePath.c_str());
                return false;
            }

            AzFramework::ProcessLauncher::ProcessLaunchInfo processLaunchInfo;
            processLaunchInfo.m_commandlineParameters = executablePath.String() + commandLineArgs;
            launchSuccess = AzFramework::ProcessLauncher::LaunchUnwatchedProcess(processLaunchInfo);
        }
        if (ownsSystemAllocator)
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
        }
#endif // #if defined(AZ_FRAMEWORK_USE_PROJECT_MANAGER)
        return launchSuccess;
    }

} // AzFramework::ProjectManager

