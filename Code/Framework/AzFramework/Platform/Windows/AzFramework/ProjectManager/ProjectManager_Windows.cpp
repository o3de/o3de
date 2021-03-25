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
#include <AzCore/PlatformIncl.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/Engine/Engine.h>

namespace AzFramework
{
    namespace ProjectManager
    {
        bool LaunchProjectManager() 
        {
            const char projectsScript[] = "projects.py";

            AZ_Warning("ProjectManager", false, "No project provided - launching project selector.");

            AZ::IO::FixedMaxPath enginePath = Engine::FindEngineRoot();
            if (enginePath.empty())
            {
                AZ_Warning("ProjectManager", false, "Couldn't find engine root");
                return false;
            }
            auto projectManagerPath = enginePath / "scripts" / "project_manager";

            if (!AZ::IO::SystemFile::Exists((projectManagerPath / projectsScript).c_str()))
            {
                AZ_Warning("ProjectManager", false, "%s not found at %s!", projectsScript, projectManagerPath.c_str());
            }
            char executablePath[AZ_MAX_PATH_LEN];
            AZ::Utils::GetExecutablePathReturnType result = AZ::Utils::GetExecutablePath(executablePath, AZ_MAX_PATH_LEN);
            auto exeFolder = AZ::IO::PathView(executablePath).ParentPath().Filename().Native();
            AZStd::fixed_string<10> debugOption{ " " };
            if (exeFolder == "debug")
            {
                // We need to use the debug version of the python interpreter to load up our debug version of our libraries which work with the debug version of QT living in this folder
                debugOption = " debug ";
            }
            AZ::IO::FixedMaxPath pythonPath = enginePath / "python" / "python.cmd";
            auto cmdPath = AZ::IO::FixedMaxPathString::format("%s%s%s --executable_path=%s", pythonPath.Native().c_str(), debugOption.c_str(), (projectManagerPath / projectsScript).c_str(), executablePath);


            STARTUPINFO si;
            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            si.dwFlags = STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_HIDE;
            PROCESS_INFORMATION pi;

            auto workingPath = AZ::IO::FixedMaxPath{ executablePath }.ParentPath().Native();
            bool launchSuccess = ::CreateProcessA(nullptr, cmdPath.data(), nullptr, nullptr, FALSE, 0, nullptr, projectManagerPath.c_str(), &si, &pi) != 0;
            if (launchSuccess)
            {
                AZ_TracePrintf("ProjectManagerSystemComponent", "Launched Project Manager successfully, shutting down.\n");
            }
            else
            {
                auto someError = GetLastError();
                AZ_Warning("ProjectManagerSystemComponent", false, "Failed to launch project manager with error %d", someError);
            }
            return launchSuccess;
        }
    } // ProjectManager
} // AzFramework

