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

#pragma once


#if defined(LINUX)
#   include "Linux_Win32Wrapper.h"
#endif
#include <ISystem.h>

////////////////////////////////////////////////////////////////////////////////
#if defined(AZ_RESTRICTED_PLATFORM)
#include AZ_RESTRICTED_FILE(MTSafeAllocator_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(MOBILE)  // IOS/Android
# define MTSAFE_DEFAULT_ALIGNMENT 8
# define MTSAFE_GENERAL_HEAP_SIZE ((1U << 20) + (1U << 19))
#elif defined(WIN32) || defined(WIN64) || defined(LINUX) || defined(MAC)
# define MTSAFE_GENERAL_HEAP_SIZE (12U << 20)
# define MTSAFE_DEFAULT_ALIGNMENT 8
#else
# error Unknown target platform
#endif

class CMTSafeHeap
{
public:
    // Constructor
    CMTSafeHeap();

    // Destructor
    ~CMTSafeHeap();

    // Performs a persisistent (in other words, non-temporary) allocation.
    void* PersistentAlloc(size_t nSize);

    // Retrieves system memory allocation size for any call to PersistentAlloc.
    // Required to not count virtual memory usage inside CrySizer
    size_t PersistentAllocSize(size_t nSize);

    // Frees memory allocation
    void FreePersistent(void* p);

    // Perform a allocation that is considered temporary and will be handled by
    // the pool itself.
    // Note: It is important that these temporary allocations are actually
    // temporary and do not persist for a long persiod of time.
    void* TempAlloc (size_t nSize, const char* szDbgSource, uint32 align = 0)
    {
        bool bFallbackToMalloc = true;
        return TempAlloc(nSize, szDbgSource, bFallbackToMalloc, align);
    }

    void* TempAlloc (size_t nSize, const char* szDbgSource, bool& bFallBackToMalloc, uint32 align = 0);

    bool IsInGeneralHeap(const void* p)
    {
        return m_pGeneralHeapStorage <= p && p < m_pGeneralHeapStorageEnd;
    }

    // Free a temporary allocaton.
    void FreeTemporary(void* p);

    // The number of live allocations allocation within the temporary pool
    size_t NumAllocations() const { return m_LiveTempAllocations; }

    // The memory usage of the mtsafe allocator
    void GetMemoryUsage(ICrySizer* pSizer);

    // zlib-compatible stubs
    static void* StaticAlloc (void* pOpaque, unsigned nItems, unsigned nSize);
    static void StaticFree (void* pOpaque, void* pAddress);

    // Dump some statistics to the cry log
    void PrintStats();


private:
    friend class CSystem;

    IGeneralMemoryHeap* m_pGeneralHeap;
    char* m_pGeneralHeapStorage;
    char* m_pGeneralHeapStorageEnd;

    // The number of temporary allocations currently active within the pool
    size_t m_LiveTempAllocations;

    // The total number of allocations performed in the pool
    size_t m_TotalAllocations;

    // The total bytes that weren't temporarily allocated
    size_t m_TempAllocationsFailed;
    // The total number of temporary allocations that fell back to global system memory
    LARGE_INTEGER m_TempAllocationsTime;
};
