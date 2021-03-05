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

#ifndef __CustomMemoryHeap_h__
#define __CustomMemoryHeap_h__
#pragma once

#include "IMemory.h"

class CCustomMemoryHeap;

//////////////////////////////////////////////////////////////////////////
class CCustomMemoryHeapBlock
    : public ICustomMemoryBlock
{
public:
    CCustomMemoryHeapBlock(CCustomMemoryHeap* pHeap);
    virtual ~CCustomMemoryHeapBlock();

    //////////////////////////////////////////////////////////////////////////
    // IMemoryBlock
    //////////////////////////////////////////////////////////////////////////
    virtual void* GetData();
    virtual int GetSize() { return m_nSize; }
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // ICustomMemoryBlock
    //////////////////////////////////////////////////////////////////////////
    virtual void CopyMemoryRegion(void* pOutputBuffer, size_t nOffset, size_t nSize);
    //////////////////////////////////////////////////////////////////////////

private:
    friend class CCustomMemoryHeap;
    CCustomMemoryHeap* m_pHeap;
    string m_sUsage;
    void* m_pData;
    uint32 m_nGPUHandle;
    size_t m_nSize;
};

//////////////////////////////////////////////////////////////////////////
class CCustomMemoryHeap
    : public ICustomMemoryHeap
{
public:

    explicit CCustomMemoryHeap(IMemoryManager::EAllocPolicy const eAllocPolicy);
    ~CCustomMemoryHeap();

    //////////////////////////////////////////////////////////////////////////
    // ICustomMemoryHeap
    //////////////////////////////////////////////////////////////////////////
    virtual ICustomMemoryBlock* AllocateBlock(size_t const nAllocateSize, char const* const sUsage, size_t const nAlignment = 16);
    virtual void GetMemoryUsage(ICrySizer* pSizer);
    virtual size_t GetAllocated();
    //////////////////////////////////////////////////////////////////////////

    void DeallocateBlock(CCustomMemoryHeapBlock* pBlock);

private:

    friend class CCustomMemoryHeapBlock;
    int m_nAllocatedSize;
    IMemoryManager::EAllocPolicy m_eAllocPolicy;
    IMemoryManager::HeapHandle m_nTraceHeapHandle;
};

#endif // __CustomMemoryHeap_h__
