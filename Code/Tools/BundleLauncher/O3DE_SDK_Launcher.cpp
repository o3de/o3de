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
#include <mach-o/dyld.h>

int main(int argc, char* argv[])
{
    // We need to pass in the engine path since we won't be able to find it by searching upwards.
    // We can't use any containers that use our custom allocator till after the call to ComponentApplication::Create()
    AZ::IO::FixedMaxPath processPath = AZ::Utils::GetExecutableDirectory();
    AZ::IO::FixedMaxPath enginePath = (processPath / "../Engine").LexicallyNormal();
    auto enginePathParam = AZ::SettingsRegistryInterface::FixedValueString::format(R"(--engine-path="%s")", enginePath.c_str());
    // Uses the fixed_vector deduction guide to determine the type is AZStd::fixed_vector<char*, 2>
    AZStd::fixed_vector commandLineParams{ processPath.Native().data(), enginePathParam.data() };


    // Create a ComponentApplication to initialize the AZ::SystemAllocator and initialize the SettingsRegistry
    AZ::ComponentApplication application(static_cast<int>(commandLineParams.size()), commandLineParams.data());
    application.Create(AZ::ComponentApplication::Descriptor());
 
    AZ::IO::FixedMaxPath installedBinariesFolder;
    if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
    {
        if (settingsRegistry->Get(installedBinariesFolder.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_InstalledBinaryFolder))
        {
            installedBinariesFolder = enginePath / installedBinariesFolder;
        }
    }

    AZ::IO::FixedMaxPath shellPath = "/bin/sh";
    AZStd::string parameters = AZStd::string::format("-c \"export LY_CMAKE_PATH=/usr/local/bin && \"%s/python/get_python.sh\"\"", enginePath.c_str());
    AzFramework::ProcessLauncher::ProcessLaunchInfo shellProcessLaunch;
    shellProcessLaunch.m_processExecutableString = AZStd::move(shellPath.Native());
    shellProcessLaunch.m_commandlineParameters = parameters;
    shellProcessLaunch.m_showWindow = true;
    shellProcessLaunch.m_workingDirectory = enginePath.String();
    AZStd::unique_ptr<AzFramework::ProcessWatcher> shellProcess(AzFramework::ProcessWatcher::LaunchProcess(shellProcessLaunch, AzFramework::ProcessCommunicationType::COMMUNICATOR_TYPE_NONE));
    shellProcess->WaitForProcessToExit(120);
    shellProcess.reset();
    
    AZ::IO::FixedMaxPath projectManagerPath = installedBinariesFolder/"o3de.app"/"Contents"/"MacOS"/"o3de";
    AzFramework::ProcessLauncher::ProcessLaunchInfo processLaunchInfo;
    processLaunchInfo.m_processExecutableString = AZStd::move(projectManagerPath.Native());
    processLaunchInfo.m_showWindow = true;
    AzFramework::ProcessLauncher::LaunchUnwatchedProcess(processLaunchInfo);

    application.Destroy();

    return 0;
}

