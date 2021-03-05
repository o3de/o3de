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

#include "CrySystem_precompiled.h"
#include "MTSafeAllocator.h"
#include <IConsole.h>

extern CMTSafeHeap* g_pPakHeap;

// Uncomment this define to enable time tracing of the MTSAFE heap
#define MTSAFE_PROFILE 1
//#undef MTSAFE_PROFILE

namespace
{
    class CSimpleTimer
    {
        LARGE_INTEGER& m_result;
        LARGE_INTEGER m_start;
    public:

        CSimpleTimer(LARGE_INTEGER& li)
            : m_result(li)
        { QueryPerformanceCounter(&m_start); }

        ~CSimpleTimer()
        {
            LARGE_INTEGER end;
            QueryPerformanceCounter(&end);
            m_result.QuadPart = end.QuadPart - m_start.QuadPart;
        }
    };
};

//////////////////////////////////////////////////////////////////////////
CMTSafeHeap::CMTSafeHeap()
    : m_LiveTempAllocations()
    , m_TotalAllocations()
    , m_TempAllocationsFailed()
    , m_TempAllocationsTime()
{
    size_t allocated = 0;
    m_pGeneralHeapStorage = (char*)CryMalloc(MTSAFE_GENERAL_HEAP_SIZE, allocated, MTSAFE_DEFAULT_ALIGNMENT);
    m_pGeneralHeapStorageEnd = m_pGeneralHeapStorage + MTSAFE_GENERAL_HEAP_SIZE;
    m_pGeneralHeap = CryGetIMemoryManager()->CreateGeneralMemoryHeap(m_pGeneralHeapStorage, MTSAFE_GENERAL_HEAP_SIZE, "MTSafeHeap");
}

//////////////////////////////////////////////////////////////////////////
CMTSafeHeap::~CMTSafeHeap()
{
    SAFE_RELEASE(m_pGeneralHeap);
    CryFree(m_pGeneralHeapStorage, MTSAFE_DEFAULT_ALIGNMENT);
}

//////////////////////////////////////////////////////////////////////////
size_t CMTSafeHeap::PersistentAllocSize(size_t nSize)
{
    return nSize;
}

//////////////////////////////////////////////////////////////////////////
void* CMTSafeHeap::PersistentAlloc(size_t nSize)
{
    size_t allocated = 0;
    return CryMalloc(nSize, allocated, MTSAFE_DEFAULT_ALIGNMENT);
}

//////////////////////////////////////////////////////////////////////////
void CMTSafeHeap::FreePersistent(void* p)
{
    CryFree(p, MTSAFE_DEFAULT_ALIGNMENT);
}

//////////////////////////////////////////////////////////////////////////
void* CMTSafeHeap::TempAlloc(size_t nSize, const char* szDbgSource, bool& bFallBackToMalloc, uint32 align)
{
# if MTSAFE_PROFILE
    CSimpleTimer timer(m_TempAllocationsTime);
# endif

    void* ptr = NULL;
    if (align)
    {
        ptr = m_pGeneralHeap->Memalign(align, nSize, szDbgSource);
    }
    else
    {
        ptr = m_pGeneralHeap->Malloc(nSize, szDbgSource);
    }

    //explicit alignment not supported beyond this point, safer to return NULL
    if (ptr || !bFallBackToMalloc)
    {
        bFallBackToMalloc = false;
        return ptr;
    }

    bFallBackToMalloc = true;

# if MTSAFE_PROFILE
    CryInterlockedAdd((volatile int*)&m_TempAllocationsFailed, (int)nSize);
# endif
    
    return CryModuleMemalign(nSize, align > 0 ? align : MTSAFE_DEFAULT_ALIGNMENT);
}

//////////////////////////////////////////////////////////////////////////
void CMTSafeHeap::FreeTemporary(void* p)
{
# if MTSAFE_PROFILE
    CSimpleTimer timer(m_TempAllocationsTime);
# endif

    if (m_pGeneralHeap->IsInAddressRange(p))
    {
        m_pGeneralHeap->Free(p);
        return;
    }

    // Fallback to free
    CryModuleMemalignFree(p);
}

//////////////////////////////////////////////////////////////////////////
void* CMTSafeHeap::StaticAlloc([[maybe_unused]] void* pOpaque, unsigned nItems, unsigned nSize)
{
    return g_pPakHeap->TempAlloc(nItems * nSize, "StaticAlloc");
}

//////////////////////////////////////////////////////////////////////////
void CMTSafeHeap::StaticFree ([[maybe_unused]] void* pOpaque, void* pAddress)
{
    g_pPakHeap->FreeTemporary(pAddress);
}

//////////////////////////////////////////////////////////////////////////
void CMTSafeHeap::GetMemoryUsage(ICrySizer* pSizer)
{
    SIZER_COMPONENT_NAME(pSizer, "FileSystem Pool");
}

void CMTSafeHeap::PrintStats()
{
# if MTSAFE_PROFILE
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    const double rFreq = 1. / static_cast<double>(freq.QuadPart);

    CryLogAlways("mtsafe temporary pool failed for %" PRISIZE_T " bytes, time spent in allocations %3.08f seconds",
        m_TempAllocationsFailed, static_cast<double>(m_TempAllocationsTime.QuadPart) * rFreq);
# endif
}

