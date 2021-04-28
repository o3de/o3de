/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
            AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_Bootstrap(settingsRegistry);
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

        if (LaunchProjectManager(engineRootPath))
        {
            AZ_TracePrintf("ProjectManager", "Project Manager launched successfully, requesting exit.");
            return ProjectPathCheckResult::ProjectManagerLaunched;
        }
        AZ_Error("ProjectManager", false, "Project Manager failed to launch and no project selected!");
        return ProjectPathCheckResult::ProjectManagerLaunchFailed;
    }

    bool LaunchProjectManager([[maybe_unused]] const AZ::IO::FixedMaxPath& engineRootPath)
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
            const char projectsScript[] = "projects.py";

            AZ_Warning("ProjectManager", false, "No project provided - launching project selector.");

            if (engineRootPath.empty())
            {
                AZ_Error("ProjectManager", false, "Couldn't find engine root");
                return false;
            }
            auto projectManagerPath = engineRootPath / "scripts" / "project_manager";

            if (!AZ::IO::SystemFile::Exists((projectManagerPath / projectsScript).c_str()))
            {
                AZ_Error("ProjectManager", false, "%s not found at %s!", projectsScript, projectManagerPath.c_str());
                return false;
            }
            AZ::IO::FixedMaxPathString executablePath;
            AZ::Utils::GetExecutablePathReturnType result = AZ::Utils::GetExecutablePath(executablePath.data(), executablePath.capacity());
            if (result.m_pathStored != AZ::Utils::ExecutablePathResult::Success)
            {
                AZ_Error("ProjectManager", false, "Could not determine executable path!");
                return false;
            }
            AZ::IO::FixedMaxPath parentPath(executablePath.c_str());
            auto exeFolder = parentPath.ParentPath();
            AZStd::fixed_string<8> debugOption;
            auto lastSep = exeFolder.Native().find_last_of(AZ_CORRECT_FILESYSTEM_SEPARATOR);
            if (lastSep != AZStd::string_view::npos)
            {
                exeFolder = exeFolder.Native().substr(lastSep + 1);
            }
            if (exeFolder == "debug")
            {
                // We need to use the debug version of the python interpreter to load up our debug version of our libraries which work with the debug version of QT living in this folder
                debugOption = "debug ";
            }
            AZ::IO::FixedMaxPath pythonPath = engineRootPath / "python";
            pythonPath /= AZ_TRAIT_AZFRAMEWORK_PYTHON_SHELL;
            auto cmdPath = AZ::IO::FixedMaxPathString::format("%s %s%s --executable_path=%s --parent_pid=%" PRId64, pythonPath.Native().c_str(),
                debugOption.c_str(), (projectManagerPath / projectsScript).c_str(), executablePath.c_str(), AZ::Platform::GetCurrentProcessId());

            AzFramework::ProcessLauncher::ProcessLaunchInfo processLaunchInfo;

            processLaunchInfo.m_commandlineParameters = cmdPath;
            processLaunchInfo.m_showWindow = false;
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

