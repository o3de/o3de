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

#include "GeneralMemoryHeap.h"

#include <AzCore/Memory/AllocatorBase.h>
#include <AzCore/Memory/HphaSchema.h>

class GeneralMemoryHeapAllocator
    : public AZ::SimpleSchemaAllocator<AZ::HphaSchema>
{
    using Base = AZ::SimpleSchemaAllocator<AZ::HphaSchema>;
public:
    static const size_t DEFAULT_ALIGNMENT = sizeof(void*);

    GeneralMemoryHeapAllocator(const char* desc)
        : Base("GeneralMemoryHeapAllocator", desc)
    {
    }

    void Reserve(size_t size)
    {
        // Allocate a block, then free it, forcing it into the page tree/cache
        void* block = m_schema->Allocate(size, GeneralMemoryHeapAllocator::DEFAULT_ALIGNMENT, 0, "GeneralMemoryHeapAllocator Reserve", __FILE__, __LINE__);
        m_schema->DeAllocate(block);
    }
};

CGeneralMemoryHeap::CGeneralMemoryHeap([[maybe_unused]] UINT_PTR base, [[maybe_unused]] size_t upperLimit, size_t reserveSize, const char* sUsage)
    : m_refCount(0)
    , m_block(nullptr)
    , m_blockSize(0)
{
    AZ::HphaSchema::Descriptor desc;
    desc.m_subAllocator = &AZ::AllocatorInstance<AZ::LegacyAllocator>::Get();
    m_allocator.reset(new AZ::AllocatorWrapper<GeneralMemoryHeapAllocator>);
    m_allocator->Create(desc, sUsage);
    if (reserveSize)
    {
        (*m_allocator)->Reserve(reserveSize);
    }
}

CGeneralMemoryHeap::CGeneralMemoryHeap(void* base, size_t size, const char* sUsage)
    : m_refCount(0)
    , m_block(base)
    , m_blockSize(size)
{
    AZ::HphaSchema::Descriptor desc;
    desc.m_fixedMemoryBlock = base;
    desc.m_fixedMemoryBlockByteSize = size;
    desc.m_fixedMemoryBlockAlignment = GeneralMemoryHeapAllocator::DEFAULT_ALIGNMENT;
    m_allocator.reset(new AZ::AllocatorWrapper<GeneralMemoryHeapAllocator>);
    m_allocator->Create(desc, sUsage);
}

CGeneralMemoryHeap::~CGeneralMemoryHeap()
{
}

bool CGeneralMemoryHeap::Cleanup()
{
    (*m_allocator)->GarbageCollect();
    return true;
}

int CGeneralMemoryHeap::AddRef()
{
    return m_refCount.fetch_add(1);
}

int CGeneralMemoryHeap::Release()
{
    int nRef = m_refCount.fetch_sub(1);

    if (nRef <= 1)
    {
        delete this;
    }

    return nRef;
}

void CGeneralMemoryHeap::RecordAlloc(void* ptr, size_t size)
{
    if (m_block == nullptr)
    {
        m_allocs.emplace(ptr, size);
    }
}

void CGeneralMemoryHeap::RecordFree(void* ptr, size_t size)
{
    if (m_block == nullptr)
    {
        m_allocs.erase(Alloc(ptr, size));
    }
}

bool CGeneralMemoryHeap::IsInAddressRange(void* ptr) const
{
    if (m_block)
    {
        return (static_cast<char*>(ptr) - static_cast<char*>(m_block)) <= m_blockSize;
    }
    auto it = m_allocs.find(Alloc(ptr));
    return it != m_allocs.end();
}

void* CGeneralMemoryHeap::Calloc(size_t numElements, size_t size, const char* sUsage)
{
    void* ptr = (*m_allocator)->Allocate(numElements * size, GeneralMemoryHeapAllocator::DEFAULT_ALIGNMENT, 0, sUsage, __FILE__, __LINE__);
    memset(ptr, 0, numElements * size);
    RecordAlloc(ptr, numElements * size);
    return ptr;
}

void* CGeneralMemoryHeap::Malloc(size_t size, const char* sUsage)
{
    void* ptr = (*m_allocator)->Allocate(size, GeneralMemoryHeapAllocator::DEFAULT_ALIGNMENT, 0, sUsage, __FILE__, __LINE__);
    RecordAlloc(ptr, size);
    return ptr;
}

size_t CGeneralMemoryHeap::Free(void* ptr)
{
    // The client code using these heaps tend to use a guesswork algorithm to freeing
    // which involves handing the pointer to every known heap until it frees, so
    // it's necessary to validate that the ptr belongs to this heap before attempting to free
    if (IsInAddressRange(ptr))
    {
        size_t size = (*m_allocator)->AllocationSize(ptr);
        RecordFree(ptr, size);
        (*m_allocator)->DeAllocate(ptr);
        return size;
    }
    return 0;
}

void* CGeneralMemoryHeap::Realloc(void* ptr, size_t size, const char* /*sUsage*/)
{
    RecordFree(ptr, (*m_allocator)->AllocationSize(ptr));
    void* newPtr = (*m_allocator)->ReAllocate(ptr, size, GeneralMemoryHeapAllocator::DEFAULT_ALIGNMENT);
    RecordAlloc(newPtr, size);
    return newPtr;
}

void* CGeneralMemoryHeap::ReallocAlign(void* ptr, size_t size, size_t alignment, const char* /*sUsage*/)
{
    RecordFree(ptr, (*m_allocator)->AllocationSize(ptr));
    void* newPtr = (*m_allocator)->ReAllocate(ptr, size, alignment);
    RecordAlloc(newPtr, size);
    return newPtr;
}

void* CGeneralMemoryHeap::Memalign(size_t boundary, size_t size, const char* sUsage)
{
    void* ptr = (*m_allocator)->Allocate(size, boundary, 0, sUsage, __FILE__, __LINE__);
    RecordAlloc(ptr, size);
    return ptr;
}

size_t CGeneralMemoryHeap::UsableSize(void* ptr) const
{
    // The client code using these heaps tend to use a guesswork algorithm to determine
    // which heap owns the pointer.  Calls to UsableSize() are a part of this guesswork.
    // The overrun detector doesn't play nicely on AllocationSize() lookups for pointers that
    // don't belong to the heap, so validate that we're in the correct address range before trying
    // to look up the size.
    return IsInAddressRange(ptr) ? (*m_allocator)->AllocationSize(ptr) : 0;
}

AZ::IAllocator* CGeneralMemoryHeap::GetAllocator() const
{
    return m_allocator->Get();
}
