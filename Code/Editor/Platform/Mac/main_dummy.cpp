/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/Process/ProcessWatcher.h>

#include <cstdlib>

int main(int argc, char* argv[])
{
    // Create a ComponentApplication to initialize the AZ::SystemAllocator and initialize the SettingsRegistry
    AZ::ComponentApplication::Descriptor desc;
    AZ::ComponentApplication application;
    application.Create(desc);
    
    AZStd::vector<AZStd::string> envVars;
    
    const char* homePath = std::getenv("HOME");
    envVars.push_back(AZStd::string::format("HOME=%s", homePath));
    
    if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
    {
        const char* dyldLibPathOrig = std::getenv("DYLD_LIBRARY_PATH");
        AZStd::string dyldSearchPath = AZStd::string::format("DYLD_LIBRARY_PATH=%s", dyldLibPathOrig);
        if (AZ::IO::FixedMaxPath projectModulePath;
            settingsRegistry->Get(projectModulePath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectConfigurationBinPath))
        {
            dyldSearchPath.append(":");
            dyldSearchPath.append(projectModulePath.c_str());
        }
        
        if (AZ::IO::FixedMaxPath installedBinariesFolder;
            settingsRegistry->Get(installedBinariesFolder.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_InstalledBinaryFolder))
        {
            if (AZ::IO::FixedMaxPath engineRootFolder;
                settingsRegistry->Get(engineRootFolder.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder))
            {
                installedBinariesFolder = engineRootFolder / installedBinariesFolder;
                dyldSearchPath.append(":");
                dyldSearchPath.append(installedBinariesFolder.c_str());
            }
        }
        envVars.push_back(dyldSearchPath);
    }
    
    AZStd::string commandArgs;
    for (int i = 1; i < argc; i++)
    {
        commandArgs.append(argv[i]);
        commandArgs.append(" ");
    }
    
    AzFramework::ProcessLauncher::ProcessLaunchInfo processLaunchInfo;
    AZ::IO::Path processPath{ AZ::IO::PathView(AZ::Utils::GetExecutableDirectory()) };
    processPath /= "Editor";
    processLaunchInfo.m_processExecutableString = AZStd::move(processPath.Native());
    processLaunchInfo.m_commandlineParameters = commandArgs;
    processLaunchInfo.m_environmentVariables = &envVars;
    processLaunchInfo.m_showWindow = true;

    AzFramework::ProcessWatcher* processWatcher = AzFramework::ProcessWatcher::LaunchProcess(processLaunchInfo, AzFramework::ProcessCommunicationType::COMMUNICATOR_TYPE_NONE);
    
    // When launching the app from Finder when it's downloaded from the web,
    // the child process terminates if the parent exits immediately.
    AZStd::this_thread::sleep_for(AZStd::chrono::seconds(1));
    application.Destroy();

    return 0;
}

