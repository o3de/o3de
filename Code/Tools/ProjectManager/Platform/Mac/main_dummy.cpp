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
    
    AZ::IO::Path processPath{ AZ::IO::PathView(AZ::Utils::GetExecutableDirectory()) };
    AZ::IO::Path enginePath = processPath/".."/"Engine";
    
    /*
       Install python packages that are required before launching ProjectManager.
    */

    AZ::IO::Path projectManagerPath = enginePath/"bin"/"Mac"/"profile"/"o3de.app"/"Contents"/"MacOS"/"o3de";
    AzFramework::ProcessLauncher::ProcessLaunchInfo processLaunchInfo;
    processLaunchInfo.m_processExecutableString = AZStd::move(projectManagerPath.Native());
    processLaunchInfo.m_showWindow = true;

    AzFramework::ProcessLauncher::LaunchUnwatchedProcess(processLaunchInfo);

    application.Destroy();

    return 0;
}

