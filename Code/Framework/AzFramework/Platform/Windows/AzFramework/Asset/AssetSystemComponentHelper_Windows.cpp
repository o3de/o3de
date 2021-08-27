/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Console/IConsole.h>
#include <AzCore/PlatformIncl.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>

#include <Psapi.h>

AZ_CVAR(bool, ap_tether_lifetime, false, nullptr, AZ::ConsoleFunctorFlags::Null,
    "If enabled, a parent process that launches the AP will terminate the AP on exit");

namespace AzFramework::AssetSystem::Platform
{
    void AllowAssetProcessorToForeground()
    {
        // Make sure that all of the asset processors can bring their window to the front
        // Hacky to put it here, but not really any better place.
        DWORD bytesReturned;

        // There's no straightforward way to get the exact number of running processes,
        // So we use 2^13 processes as a generous upper bound that shouldn't be hit.
        DWORD processIds[8 * 1024];

        if (EnumProcesses(processIds, sizeof(processIds), &bytesReturned))
        {
            const DWORD numProcesses = bytesReturned / sizeof(DWORD);
            for (DWORD processIndex = 0; processIndex < numProcesses; ++processIndex)
            {
                DWORD processId = processIds[processIndex];
                HANDLE processHandle = OpenProcess(PROCESS_QUERY_INFORMATION |
                    PROCESS_VM_READ,
                    FALSE, processId);

                // Get the process name.
                if (processHandle)
                {
                    HMODULE moduleHandle;
                    DWORD bytesNeededForAllProcessModules;

                    // Get the first module, because that will be the executable
                    if (EnumProcessModules(processHandle, &moduleHandle, sizeof(moduleHandle), &bytesNeededForAllProcessModules))
                    {
                        wchar_t processName[4096] = L"<unknown>";
                        if (GetModuleBaseName(processHandle, moduleHandle, processName, AZ_ARRAY_SIZE(processName)) > 0)
                        {
                            if (azwcsicmp(processName, L"AssetProcessor") == 0)
                            {
                                AllowSetForegroundWindow(processId);
                            }
                        }
                    }
                }
            }
        }
    }

    bool LaunchAssetProcessor(AZStd::string_view executableDirectory, AZStd::string_view engineRoot,
        AZStd::string_view projectPath)
    {
        AZ::IO::FixedMaxPath assetProcessorPath{ executableDirectory };
        assetProcessorPath /= "AssetProcessor.exe";

        if (!AZ::IO::SystemFile::Exists(assetProcessorPath.c_str()))
        {
            // Check for existence of one under a "bin" directory, i.e. engineRoot is an SDK structure.
            assetProcessorPath =
                AZ::IO::FixedMaxPath{engineRoot} / "bin" / AZ_TRAIT_OS_PLATFORM_NAME / AZ_BUILD_CONFIGURATION_TYPE / "AssetProcessor.exe";

            if (!AZ::IO::SystemFile::Exists(assetProcessorPath.c_str()))
            {
                return false;
            }
        }

        auto fullLaunchCommand = AZ::IO::FixedMaxPathString::format(R"("%s" --start-hidden)", assetProcessorPath.c_str());

        // Add the engine path to the launch command if not empty
        if (!engineRoot.empty())
        {
            fullLaunchCommand += R"( --engine-path=")";
            fullLaunchCommand += engineRoot;
            // Windows CreateProcess has issues with paths that end with a trailing backslash
            // so remove it if it exist
            if (fullLaunchCommand.ends_with(AZ::IO::WindowsPathSeparator))
            {
                fullLaunchCommand.pop_back();
            }
            fullLaunchCommand += '"';
        }

        // Add the active project path to the launch command if not empty
        if (!projectPath.empty())
        {
            fullLaunchCommand += R"( --project-path=")";
            fullLaunchCommand += projectPath;
            fullLaunchCommand += '"';
        }

        // Create or retrieve the job handle associated with the asset processor
        HANDLE apJob = nullptr;

        if (ap_tether_lifetime)
        {
            apJob = ::CreateJobObjectA(nullptr, "AssetProcessorJob");
            if (apJob && GetLastError() != ERROR_ALREADY_EXISTS)
            {
                // We're creating the job for the first time. Configure it to close child processes when this process exits.
                JOBOBJECT_EXTENDED_LIMIT_INFORMATION info = {};
                info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
                ::SetInformationJobObject(apJob, JobObjectExtendedLimitInformation, &info, sizeof(info));
            }
        }

        STARTUPINFO si;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_MINIMIZE;
        PROCESS_INFORMATION pi;

        AZStd::wstring fullLaunchCommandW;
        AZStd::to_wstring(fullLaunchCommandW, fullLaunchCommand.c_str());
        AZStd::wstring execututableDirectoryW;
        AZStd::to_wstring(execututableDirectoryW, executableDirectory.data());
        bool createResult = ::CreateProcessW(nullptr, fullLaunchCommandW.data(), nullptr, nullptr, FALSE, 0, nullptr, execututableDirectoryW.c_str(), &si, &pi) != 0;

        if (ap_tether_lifetime && apJob && createResult)
        {
            // Save process and thread handle to terminate AP when the parent process exits
            ::AssignProcessToJobObject(apJob, pi.hProcess);
        }

        return createResult;
    }
}
