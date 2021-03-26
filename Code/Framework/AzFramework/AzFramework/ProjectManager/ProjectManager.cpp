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

#include <AzCore/Platform.h>
#include <AzCore/Settings/CommandLine.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Utils/Utils.h>

#include <AzFramework/AzFramework_Traits_Platform.h>
#include <AzFramework/Process/ProcessWatcher.h>
#include <AzFramework/Engine/Engine.h>


namespace AzFramework
{
    namespace ProjectManager
    {
        // Check for a project name, if not found, attempt to launch project manager and shut down
        ProjectPathCheckResult CheckProjectPathProvided(const int argc, char* argv[])
        {
            // If we were able to locate a path to a project, we're done
            if (HasProjectPath(argc, argv))
            {
                return ProjectPathCheckResult::ProjectPathFound;
            }

            if (LaunchProjectManager())
            {
                AZ_TracePrintf("ProjectManager", "Project Manager launched successfully, requesting exit.");
                return ProjectPathCheckResult::ProjectManagerLaunched;
            }
            AZ_Error("ProjectManager", false, "Project Manager failed to launch and no project selected!");
            return ProjectPathCheckResult::ProjectManagerLaunchFailed;
        }

        bool HasProjectPath(const int argc, char* argv[])
        {
            bool hasProjectPath = false;
            bool ownsAllocator = false;
            if (!AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady())
            {
                AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
                ownsAllocator = true;
            }
            {
                AZ::CommandLine commandLine;
                commandLine.Parse(argc, argv);
                AZ::SettingsRegistryImpl settingsRegistry;
                AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_Bootstrap(settingsRegistry);
                AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_CommandLine(settingsRegistry, commandLine, false);
                auto sysGameFolderKey = AZ::SettingsRegistryInterface::FixedValueString(
                    AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey) + "/project_path";
                AZ::IO::FixedMaxPath registryPath;
                hasProjectPath = settingsRegistry.Get(registryPath.Native(), sysGameFolderKey);
            }
            if (ownsAllocator)
            {
                AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
            }
            return hasProjectPath;
        }

        bool LaunchProjectManager()
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

                AZ::IO::FixedMaxPath enginePath = Engine::FindEngineRoot();
                if (enginePath.empty())
                {
                    AZ_Error("ProjectManager", false, "Couldn't find engine root");
                    return false;
                }
                auto projectManagerPath = enginePath / "scripts" / "project_manager";

                if (!AZ::IO::SystemFile::Exists((projectManagerPath / projectsScript).c_str()))
                {
                    AZ_Error("ProjectManager", false, "%s not found at %s!", projectsScript, projectManagerPath.c_str());
                    return false;
                }
                char executablePath[AZ_MAX_PATH_LEN];
                AZ::Utils::GetExecutablePathReturnType result = AZ::Utils::GetExecutablePath(executablePath, AZ_MAX_PATH_LEN);
                auto exeFolder = AZ::IO::PathView(executablePath).ParentPath().Filename().Native();
                AZStd::fixed_string<8> debugOption;
                if (exeFolder == "debug")
                {
                    // We need to use the debug version of the python interpreter to load up our debug version of our libraries which work with the debug version of QT living in this folder
                    debugOption = "debug ";
                }
                AZ::IO::FixedMaxPath pythonPath = enginePath / "python";
                pythonPath /= AZ_TRAIT_AZFRAMEWORK_PYTHON_SHELL;
                auto cmdPath = AZ::IO::FixedMaxPathString::format("%s %s%s --executable_path=%s --parent_pid=%" PRId64, pythonPath.Native().c_str(),
                    debugOption.c_str(), (projectManagerPath / projectsScript).c_str(), executablePath, AZ::Platform::GetCurrentProcessId());

                AzFramework::ProcessLauncher::ProcessLaunchInfo processLaunchInfo;

                processLaunchInfo.m_commandlineParameters = cmdPath;
                processLaunchInfo.m_showWindow = false;
                launchSuccess = AzFramework::ProcessLauncher::LaunchUnwatchedProcess(processLaunchInfo);
            }
            if(ownsSystemAllocator)
            {
                AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
            }
#endif // #if defined(AZ_FRAMEWORK_USE_PROJECT_MANAGER)
            return launchSuccess;
        }
    } // ProjectManager
} // AzFramework

