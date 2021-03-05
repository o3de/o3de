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

#ifndef CRYINCLUDE_CRYSYSTEM_GENERALMEMORYHEAP_H
#define CRYINCLUDE_CRYSYSTEM_GENERALMEMORYHEAP_H
#pragma once


#include "IMemory.h"
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/containers/set.h>

class GeneralMemoryHeapAllocator;

class CGeneralMemoryHeap
    : public IGeneralMemoryHeap
{
    struct Alloc
    {
        void*  m_base;
        size_t m_size;

        Alloc(void* base = nullptr, size_t size = 0)
            : m_base(base)
            , m_size(size)
        {}
        
        bool operator==(const Alloc& rhs) const
        {
            // size doesn't matter
            return m_base == rhs.m_base;
        }

        bool operator<(const Alloc& rhs) const
        {
            // this will cause allocs to be sorted by address
            return m_base < rhs.m_base;
        }
    };

public:
    // Create a heap that will map/unmap pages in the range [baseAddress, baseAddress + upperLimit).
    CGeneralMemoryHeap(UINT_PTR baseAddress, size_t upperLimit, size_t reserveSize, const char* sUsage);

    // Create a heap that will assumes all memory in the range [base, base + size) is already mapped.
    CGeneralMemoryHeap(void* base, size_t size, const char* sUsage);

    ~CGeneralMemoryHeap();

public: // IGeneralMemoryHeap Members
    bool Cleanup();

    int AddRef();
    int Release();

    bool IsInAddressRange(void* ptr) const;

    void* Calloc(size_t nmemb, size_t size, const char* sUsage = NULL);
    void* Malloc(size_t sz, const char* sUsage = NULL);
    size_t Free(void* ptr);
    void* Realloc(void* ptr, size_t sz, const char* sUsage = NULL);
    void* ReallocAlign(void* ptr, size_t size, size_t alignment, const char* sUsage = NULL);
    void* Memalign(size_t boundary, size_t size, const char* sUsage = NULL);
    size_t UsableSize(void* ptr) const;

    AZ::IAllocator* GetAllocator() const override;

private:
    CGeneralMemoryHeap(const CGeneralMemoryHeap&) = delete;
    CGeneralMemoryHeap& operator = (const CGeneralMemoryHeap&) = delete;

    void RecordAlloc(void* ptr, size_t size);
    void RecordFree(void* ptr, size_t size);

private:
    AZStd::unique_ptr<AZ::AllocatorWrapper<GeneralMemoryHeapAllocator>> m_allocator;
    AZStd::atomic_int m_refCount;
    void* m_block;
    size_t m_blockSize;
    AZStd::set<Alloc, AZStd::less<Alloc>, AZ::AZStdAlloc<AZ::LegacyAllocator>> m_allocs;
};

#endif // CRYINCLUDE_CRYSYSTEM_GENERALMEMORYHEAP_H
