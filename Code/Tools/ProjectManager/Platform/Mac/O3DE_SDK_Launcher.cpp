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
    AZ::ComponentApplication application;
    application.Create(AZ::ComponentApplication::Descriptor());
    {    
        AZ::IO::Path processPath{ AZ::IO::PathView(AZ::Utils::GetExecutableDirectory()) };
        AZ::IO::Path enginePath = processPath/".."/"Engine";
        
        /*
           Install python packages that are required before launching ProjectManager.
        */

        AZ::IO::Path pythonPath = enginePath/"python"/"runtime"/"python-3.7.10-rev1-darwin"/"Python.framework"/"Versions"/"3.7"/"bin"/"python3";
        AZStd::string parameters = AZStd::string::format("\"-s\" \"-m\" \"pip\" \"install\" \"-r\" \"%s/python/requirements.txt\" \"--disable-pip-version-check\" \"--no-warn-script-location\"", enginePath.c_str());
        AzFramework::ProcessLauncher::ProcessLaunchInfo pythonProcessLaunch;
        pythonProcessLaunch.m_processExecutableString = AZStd::move(pythonPath.Native());
        pythonProcessLaunch.m_commandlineParameters = parameters;
        pythonProcessLaunch.m_showWindow = true;
        AZStd::unique_ptr<AzFramework::ProcessWatcher> pythonProcess(AzFramework::ProcessWatcher::LaunchProcess(pythonProcessLaunch, AzFramework::ProcessCommunicationType::COMMUNICATOR_TYPE_NONE));
        pythonProcess->WaitForProcessToExit(60);

        pythonProcessLaunch.m_commandlineParameters = AZStd::string::format("\"-s\" \"-m\" \"pip\" \"install\" \"-e\" \"%s/scripts/o3de\" \"--no-deps\" \"--disable-pip-version-check\" \"--no-warn-script-location\"", enginePath.c_str());
        pythonProcess.reset(AzFramework::ProcessWatcher::LaunchProcess(pythonProcessLaunch, AzFramework::ProcessCommunicationType::COMMUNICATOR_TYPE_NONE));
        pythonProcess->WaitForProcessToExit(60);
        pythonProcess.reset();

        AZ::IO::Path projectManagerPath = enginePath/"bin"/"Mac"/"profile"/"o3de.app"/"Contents"/"MacOS"/"o3de";
        AzFramework::ProcessLauncher::ProcessLaunchInfo processLaunchInfo;
        processLaunchInfo.m_processExecutableString = AZStd::move(projectManagerPath.Native());
        processLaunchInfo.m_showWindow = true;

        AzFramework::ProcessLauncher::LaunchUnwatchedProcess(processLaunchInfo);
    }
    application.Destroy();

    return 0;
}

