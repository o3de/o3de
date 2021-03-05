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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "EditorDefs.h"

#include "ProcessInfo.h"

#if AZ_TRAIT_OS_PLATFORM_APPLE
#include <mach/mach.h>
#endif

#if defined(AZ_PLATFORM_WINDOWS)
#include "Psapi.h"
void UnloadPSApi();

typedef BOOL (WINAPI * GetProcessMemoryInfoProc)(HANDLE, PPROCESS_MEMORY_COUNTERS, DWORD);

static HMODULE                                  g_hPSAPI = 0;
static GetProcessMemoryInfoProc g_pGetProcessMemoryInfo = 0;
#endif // AZ_PLATFORM_WINDOWS

CProcessInfo::CProcessInfo(void)
{
}

CProcessInfo::~CProcessInfo(void)
{
#if defined(AZ_PLATFORM_WINDOWS)
    UnloadPSApi();
#endif
}

#if defined(AZ_PLATFORM_WINDOWS)
void LoadPSApi()
{
    if (!g_hPSAPI)
    {
        g_hPSAPI = LoadLibraryA("psapi.dll");

        if (g_hPSAPI)
        {
            g_pGetProcessMemoryInfo = (GetProcessMemoryInfoProc)GetProcAddress(g_hPSAPI, "GetProcessMemoryInfo");
        }
    }
}

void UnloadPSApi()
{
    if (g_hPSAPI)
    {
        FreeLibrary(g_hPSAPI);
        g_hPSAPI = 0;
        g_pGetProcessMemoryInfo = 0;
    }
}
#endif

void CProcessInfo::QueryMemInfo(ProcessMemInfo& meminfo)
{
    memset(&meminfo, 0, sizeof(ProcessMemInfo));
#if defined(AZ_PLATFORM_WINDOWS)
    LoadPSApi();

    if (!g_pGetProcessMemoryInfo)
    {
        return;
    }

    PROCESS_MEMORY_COUNTERS pc;
    HANDLE hProcess = GetCurrentProcess();
    pc.cb = sizeof(pc);
    g_pGetProcessMemoryInfo(hProcess, &pc, sizeof(pc));

    meminfo.WorkingSet = pc.WorkingSetSize;
    meminfo.PeakWorkingSet = pc.PeakWorkingSetSize;
    meminfo.PagefileUsage = pc.PagefileUsage;
    meminfo.PeakPagefileUsage = pc.PeakPagefileUsage;
    meminfo.PageFaultCount = pc.PageFaultCount;
#elif AZ_TRAIT_OS_PLATFORM_APPLE
    task_basic_info basic_info;
    mach_msg_type_number_t size = sizeof(basic_info);
    kern_return_t kerr = task_info(mach_task_self(), TASK_BASIC_INFO, reinterpret_cast<task_info_t>(&basic_info), &size);
    if (kerr == KERN_SUCCESS)
    {
        meminfo.WorkingSet = basic_info.resident_size;
        meminfo.PagefileUsage = basic_info.virtual_size;
    }
    task_events_info events_info;
    size = sizeof(events_info);
    kerr = task_info(mach_task_self(), TASK_EVENTS_INFO, reinterpret_cast<task_info_t>(&events_info), &size);
    if (kerr == KERN_SUCCESS)
    {
        meminfo.PageFaultCount = events_info.faults;
    }
#endif
}
