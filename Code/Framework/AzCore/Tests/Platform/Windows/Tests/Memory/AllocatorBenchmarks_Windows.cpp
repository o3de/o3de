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
