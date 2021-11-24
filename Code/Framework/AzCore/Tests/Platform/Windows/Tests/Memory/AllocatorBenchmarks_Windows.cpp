/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>
#include <AzCore/Debug/Trace.h>

#include <malloc.h>
#include <psapi.h>

namespace Benchmark
{
    namespace Platform
    {
        size_t GetProcessMemoryUsageBytes()
        {
            EmptyWorkingSet(GetCurrentProcess());

            //PROCESS_MEMORY_COUNTERS_EX pmc;
            //GetProcessMemoryInfo(GetCurrentProcess(), (PPROCESS_MEMORY_COUNTERS)&pmc, sizeof(pmc));
            //return pmc.PrivateUsage;
            //return pmc.WorkingSetSize;
                                   
            //size_t memoryUsage = 0;
            //HANDLE defaultProcessHeap = GetProcessHeap();
            //PROCESS_HEAP_ENTRY heapEntry;
            //if (HeapLock(defaultProcessHeap) == FALSE) 
            //{
            //    AZ_Error("Benchmark", false, "Could not lock process' heap, error: %d", GetLastError());
            //    return memoryUsage;
            //}

            //heapEntry.lpData = NULL;
            //while (HeapWalk(defaultProcessHeap, &heapEntry) != FALSE) 
            //{
            //    memoryUsage += heapEntry.cbData;
            //}

            //DWORD lastError = GetLastError();
            //if (lastError != ERROR_NO_MORE_ITEMS) 
            //{
            //    AZ_Error("Benchmark", false, "HeapWalk failed with LastError %d", lastError);
            //}

            //if (HeapUnlock(defaultProcessHeap) == FALSE) 
            //{
            //    AZ_Error("Benchmark", false, "Failed to unlock heap with LastError %d", GetLastError());
            //}

            //return memoryUsage;

            size_t memoryUsage = 0;

            MEMORY_BASIC_INFORMATION mbi = { 0 };
            unsigned char* pEndRegion = NULL;
            while (sizeof(mbi) == VirtualQuery(pEndRegion, &mbi, sizeof(mbi))) {
                pEndRegion += mbi.RegionSize;
                if ((mbi.AllocationProtect & PAGE_READWRITE) && (mbi.State & MEM_COMMIT)) {
                    memoryUsage += mbi.RegionSize;
                }
            }
            return memoryUsage;
        }

        size_t GetMemorySize(void* memory)
        {
            return memory ? _aligned_msize(memory, 1, 0) : 0;
        }
    }
}
