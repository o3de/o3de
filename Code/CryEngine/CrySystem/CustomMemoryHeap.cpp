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
#include "CustomMemoryHeap.h"


#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define CUSTOMMEMORYHEAP_CPP_SECTION_1 1
#define CUSTOMMEMORYHEAP_CPP_SECTION_2 2
#define CUSTOMMEMORYHEAP_CPP_SECTION_3 3
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION CUSTOMMEMORYHEAP_CPP_SECTION_1
#include AZ_RESTRICTED_FILE(CustomMemoryHeap_cpp)
#endif

//////////////////////////////////////////////////////////////////////////
CCustomMemoryHeapBlock::CCustomMemoryHeapBlock(CCustomMemoryHeap* pHeap)
    : m_pHeap(pHeap)
    , m_pData(0)
    , m_nSize(0)
    , m_nGPUHandle(0)
{
}

//////////////////////////////////////////////////////////////////////////
CCustomMemoryHeapBlock::~CCustomMemoryHeapBlock()
{
    m_pHeap->DeallocateBlock(this);
}

//////////////////////////////////////////////////////////////////////////
void* CCustomMemoryHeapBlock::GetData()
{
    return m_pData;
}

//////////////////////////////////////////////////////////////////////////
void CCustomMemoryHeapBlock::CopyMemoryRegion(void* pOutputBuffer, size_t nOffset, size_t nSize)
{
    assert(nOffset + nSize <= m_nSize);
    if (nOffset + nSize <= m_nSize)
    {
        memcpy(pOutputBuffer, (uint8*)m_pData + nOffset, nSize);
    }
    else
    {
        CryFatalError("Bad CopyMemoryRegion range");
    }
}

//////////////////////////////////////////////////////////////////////////
ICustomMemoryBlock* CCustomMemoryHeap::AllocateBlock(size_t const nAllocateSize, char const* const sUsage, size_t const nAlignment /* = 16 */)
{
    CCustomMemoryHeapBlock* pBlock = new CCustomMemoryHeapBlock(this);
    pBlock->m_sUsage = sUsage;
    pBlock->m_nSize = nAllocateSize;

    switch (m_eAllocPolicy)
    {
    case IMemoryManager::eapDefaultAllocator:
    {
        size_t allocated = 0;
        pBlock->m_pData = CryMalloc(nAllocateSize, allocated, nAlignment);
        break;
    }
    case IMemoryManager::eapPageMapped:
        pBlock->m_pData = CryMemory::AllocPages(nAllocateSize);
        break;
    case IMemoryManager::eapCustomAlignment:
#if defined(DEBUG)
        if (nAlignment == 0)
        {
            CryFatalError("CCustomMemoryHeap: trying to allocate memory via eapCustomAlignment with an alignment of zero!");
        }
#endif
        pBlock->m_pData = CryModuleMemalign(nAllocateSize, nAlignment);
        break;
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION CUSTOMMEMORYHEAP_CPP_SECTION_2
#include AZ_RESTRICTED_FILE(CustomMemoryHeap_cpp)
#endif
    default:
        CryFatalError("CCustomMemoryHeap: unknown allocation policy during AllocateBlock!");
        break;
    }

    CryInterlockedAdd(&m_nAllocatedSize, nAllocateSize);

    return pBlock;
}

void CCustomMemoryHeap::DeallocateBlock(CCustomMemoryHeapBlock* pBlock)
{
    switch (m_eAllocPolicy)
    {
    case IMemoryManager::eapDefaultAllocator:
        CryFree(pBlock->m_pData, 0);
        break;
    case IMemoryManager::eapPageMapped:
        CryMemory::FreePages(pBlock->m_pData, pBlock->GetSize());
        break;
    case IMemoryManager::eapCustomAlignment:
        CryModuleMemalignFree(pBlock->m_pData);
        break;
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION CUSTOMMEMORYHEAP_CPP_SECTION_3
#include AZ_RESTRICTED_FILE(CustomMemoryHeap_cpp)
#endif
    default:
        CryFatalError("CCustomMemoryHeap: unknown allocation policy during DeallocateBlock!");
        break;
    }

    int nAllocateSize = (int)pBlock->m_nSize;
    CryInterlockedAdd(&m_nAllocatedSize, -nAllocateSize);
}

//////////////////////////////////////////////////////////////////////////
void CCustomMemoryHeap::GetMemoryUsage(ICrySizer* pSizer)
{
    pSizer->AddObject(this, m_nAllocatedSize);
}

//////////////////////////////////////////////////////////////////////////
size_t CCustomMemoryHeap::GetAllocated()
{
    return m_nAllocatedSize;
}

//////////////////////////////////////////////////////////////////////////
CCustomMemoryHeap::CCustomMemoryHeap(IMemoryManager::EAllocPolicy const eAllocPolicy)
{
    m_nAllocatedSize = 0;
    m_eAllocPolicy = eAllocPolicy;
    m_nTraceHeapHandle = 0;
}

//////////////////////////////////////////////////////////////////////////
CCustomMemoryHeap::~CCustomMemoryHeap()
{
}
