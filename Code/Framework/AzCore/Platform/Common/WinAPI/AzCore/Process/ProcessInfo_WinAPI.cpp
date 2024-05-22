/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>
#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzCore/Process/ProcessInfo.h>
#include <Psapi.h>

namespace AZ
{
    inline namespace Process
    {
        bool QueryMemInfo(ProcessMemInfo& meminfo)
        {
            // The DynamicModuleHandle stores the filename to load in an fixed_string which avoids any dynamic memory allocations
            // It will unload the Psapi.dll in static de-initialization
            static auto handlePsapi = DynamicModuleHandle::Create("psapi.dll");

            using GetProcessMemoryInfoProc = BOOL(WINAPI*)(HANDLE, PPROCESS_MEMORY_COUNTERS, DWORD);
            static GetProcessMemoryInfoProc procGetProcessMemoryInfo = nullptr;
            if (procGetProcessMemoryInfo == nullptr)
            {
                // Cache off the function addres of GetProcessMemoryInfo proc on the first call
                if (handlePsapi != nullptr)
                {
                    // Load the PSAPI.dll if it is not loaded
                    if (!handlePsapi->IsLoaded())
                    {
                        handlePsapi->Load();
                    }

                    // If the Psapi.dll is loaded lookup the GetProcessMemoryInfo function
                    if (handlePsapi->IsLoaded())
                    {
                        procGetProcessMemoryInfo = handlePsapi->GetFunction<GetProcessMemoryInfoProc>("GetProcessMemoryInfo");
                    }
                }
            }

            if (!procGetProcessMemoryInfo)
            {
                return false;
            }

            PROCESS_MEMORY_COUNTERS pc;
            HANDLE hProcess = GetCurrentProcess();
            pc.cb = sizeof(pc);
            procGetProcessMemoryInfo(hProcess, &pc, sizeof(pc));

            memset(&meminfo, 0, sizeof(ProcessMemInfo));
            meminfo.m_workingSet = pc.WorkingSetSize;
            meminfo.m_peakWorkingSet = pc.PeakWorkingSetSize;
            meminfo.m_pagefileUsage = pc.PagefileUsage;
            meminfo.m_peakPagefileUsage = pc.PeakPagefileUsage;
            meminfo.m_pageFaultCount = pc.PageFaultCount;

            return true;
        }
    } // inline namespace Process
} // namespace AZ
