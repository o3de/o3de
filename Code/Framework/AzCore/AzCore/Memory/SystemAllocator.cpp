/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Memory/AllocatorManager.h>

#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/Memory/AllocationRecords.h>
#include <AzCore/Memory/MemoryDrillerBus.h>

#include <AzCore/std/functional.h>

#include <AzCore/Debug/Profiler.h>

#define AZCORE_SYSTEM_ALLOCATOR_HPHA 1
#define AZCORE_SYSTEM_ALLOCATOR_MALLOC 2
#define AZCORE_SYSTEM_ALLOCATOR_HEAP 3

#if !defined(AZCORE_SYSTEM_ALLOCATOR)
    // define the default
    #define AZCORE_SYSTEM_ALLOCATOR AZCORE_SYSTEM_ALLOCATOR_HPHA
#endif

#if AZCORE_SYSTEM_ALLOCATOR == AZCORE_SYSTEM_ALLOCATOR_HPHA
    #include <AzCore/Memory/HphaSchema.h>
#elif AZCORE_SYSTEM_ALLOCATOR == AZCORE_SYSTEM_ALLOCATOR_MALLOC
    #include <AzCore/Memory/MallocSchema.h>
#elif AZCORE_SYSTEM_ALLOCATOR == AZCORE_SYSTEM_ALLOCATOR_HEAP
    #include <AzCore/Memory/HeapSchema.h>
#else
    #error "Invalid allocator selected for SystemAllocator"
#endif


using namespace AZ;

//////////////////////////////////////////////////////////////////////////
// Globals - we use global storage for the first memory schema, since we can't use dynamic memory!
static bool g_isSystemSchemaUsed = false;
#if AZCORE_SYSTEM_ALLOCATOR == AZCORE_SYSTEM_ALLOCATOR_HPHA
    static AZStd::aligned_storage<sizeof(HphaSchema), AZStd::alignment_of<HphaSchema>::value>::type g_systemSchema;
#elif AZCORE_SYSTEM_ALLOCATOR == AZCORE_SYSTEM_ALLOCATOR_MALLOC
    static AZStd::aligned_storage<sizeof(MallocSchema), AZStd::alignment_of<MallocSchema>::value>::type g_systemSchema;
#elif AZCORE_SYSTEM_ALLOCATOR == AZCORE_SYSTEM_ALLOCATOR_HEAP
    static AZStd::aligned_storage<sizeof(HeapSchema), AZStd::alignment_of<HeapSchema>::value>::type g_systemSchema;
#endif

//////////////////////////////////////////////////////////////////////////

//=========================================================================
// SystemAllocator
// [9/2/2009]
//=========================================================================
SystemAllocator::SystemAllocator()
    : AllocatorBase(this, "SystemAllocator", "Fundamental generic memory allocator")
    , m_isCustom(false)
    , m_allocator(nullptr)
    , m_ownsOSAllocator(false)
{
}

//=========================================================================
// ~SystemAllocator
//=========================================================================
SystemAllocator::~SystemAllocator()
{
    if (IsReady())
    {
        Destroy();
    }
}

//=========================================================================
// ~Create
// [9/2/2009]
//=========================================================================
bool
SystemAllocator::Create(const Descriptor& desc)
{
    AZ_Assert(IsReady() == false, "System allocator was already created!");
    if (IsReady())
    {
        return false;
    }

    m_desc = desc;

    if (!AllocatorInstance<OSAllocator>::IsReady())
    {
        m_ownsOSAllocator = true;
        AllocatorInstance<OSAllocator>::Create();
    }
    bool isReady = false;
    if (desc.m_custom)
    {
        m_isCustom = true;
        m_allocator = desc.m_custom;
        isReady = true;
    }
    else
    {
        m_isCustom = false;
#if AZCORE_SYSTEM_ALLOCATOR == AZCORE_SYSTEM_ALLOCATOR_HPHA
        HphaSchema::Descriptor heapDesc;
        heapDesc.m_pageSize = desc.m_heap.m_pageSize;
        heapDesc.m_poolPageSize = desc.m_heap.m_poolPageSize;
        AZ_Assert(desc.m_heap.m_numFixedMemoryBlocks <= 1, "We support max1 memory block at the moment!");
        if (desc.m_heap.m_numFixedMemoryBlocks > 0)
        {
            heapDesc.m_fixedMemoryBlock = desc.m_heap.m_fixedMemoryBlocks[0];
            heapDesc.m_fixedMemoryBlockByteSize = desc.m_heap.m_fixedMemoryBlocksByteSize[0];
        }
        heapDesc.m_subAllocator = desc.m_heap.m_subAllocator;
        heapDesc.m_isPoolAllocations = desc.m_heap.m_isPoolAllocations;
        // Fix SystemAllocator from growing in small chunks
        heapDesc.m_systemChunkSize = desc.m_heap.m_systemChunkSize;
#elif AZCORE_SYSTEM_ALLOCATOR == AZCORE_SYSTEM_ALLOCATOR_MALLOC
        MallocSchema::Descriptor heapDesc;
#elif AZCORE_SYSTEM_ALLOCATOR == AZCORE_SYSTEM_ALLOCATOR_HEAP
        HeapSchema::Descriptor heapDesc;
        memcpy(heapDesc.m_memoryBlocks, desc.m_heap.m_memoryBlocks, sizeof(heapDesc.m_memoryBlocks));
        memcpy(heapDesc.m_memoryBlocksByteSize, desc.m_heap.m_memoryBlocksByteSize, sizeof(heapDesc.m_memoryBlocksByteSize));
        heapDesc.m_numMemoryBlocks = desc.m_heap.m_numMemoryBlocks;
#endif
        if (&AllocatorInstance<SystemAllocator>::Get() == this) // if we are the system allocator
        {
            AZ_Assert(!g_isSystemSchemaUsed, "AZ::SystemAllocator MUST be created first! It's the source of all allocations!");

#if AZCORE_SYSTEM_ALLOCATOR == AZCORE_SYSTEM_ALLOCATOR_HPHA
            m_allocator = new(&g_systemSchema)HphaSchema(heapDesc);
#elif AZCORE_SYSTEM_ALLOCATOR == AZCORE_SYSTEM_ALLOCATOR_MALLOC
            m_allocator = new(&g_systemSchema)MallocSchema(heapDesc);
#elif AZCORE_SYSTEM_ALLOCATOR == AZCORE_SYSTEM_ALLOCATOR_HEAP
            m_allocator = new(&g_systemSchema)HeapSchema(heapDesc);
#endif
            g_isSystemSchemaUsed = true;
            isReady = true;
        }
        else
        {
            // this class should be inheriting from SystemAllocator
            AZ_Assert(AllocatorInstance<SystemAllocator>::IsReady(), "System allocator must be created before any other allocator! They allocate from it.");

#if AZCORE_SYSTEM_ALLOCATOR == AZCORE_SYSTEM_ALLOCATOR_HPHA
            m_allocator = azcreate(HphaSchema, (heapDesc), SystemAllocator);
#elif AZCORE_SYSTEM_ALLOCATOR == AZCORE_SYSTEM_ALLOCATOR_MALLOC
            m_allocator = azcreate(MallocSchema, (heapDesc), SystemAllocator);
#elif AZCORE_SYSTEM_ALLOCATOR == AZCORE_SYSTEM_ALLOCATOR_HEAP
            m_allocator = azcreate(HeapSchema, (heapDesc), SystemAllocator);
#endif
            if (m_allocator == nullptr)
            {
                isReady = false;
            }
            else
            {
                isReady = true;
            }
        }
    }

    return isReady;
}

//=========================================================================
// Allocate
// [9/2/2009]
//=========================================================================
void
SystemAllocator::Destroy()
{
    if (g_isSystemSchemaUsed)
    {
        int dummy;
        (void)dummy;
    }

    if (!m_isCustom)
    {
        if ((void*)m_allocator == (void*)&g_systemSchema)
        {
#if AZCORE_SYSTEM_ALLOCATOR == AZCORE_SYSTEM_ALLOCATOR_HPHA
            static_cast<HphaSchema*>(m_allocator)->~HphaSchema();
#elif AZCORE_SYSTEM_ALLOCATOR == AZCORE_SYSTEM_ALLOCATOR_MALLOC
            static_cast<MallocSchema*>(m_allocator)->~MallocSchema();
#elif AZCORE_SYSTEM_ALLOCATOR == AZCORE_SYSTEM_ALLOCATOR_HEAP
            static_cast<HeapSchema*>(m_allocator)->~HeapSchema();
#endif
            g_isSystemSchemaUsed = false;
        }
        else
        {
            azdestroy(m_allocator);
        }
    }

    if (m_ownsOSAllocator)
    {
        AllocatorInstance<OSAllocator>::Destroy();
        m_ownsOSAllocator = false;
    }
}

AllocatorDebugConfig SystemAllocator::GetDebugConfig()
{
    return AllocatorDebugConfig()
        .StackRecordLevels(m_desc.m_stackRecordLevels)
        .UsesMemoryGuards(!m_isCustom)
        .MarksUnallocatedMemory(!m_isCustom)
        .ExcludeFromDebugging(!m_desc.m_allocationRecords);
}

IAllocatorAllocate* SystemAllocator::GetSchema()
{
    return m_allocator;
}

//=========================================================================
// Allocate
// [9/2/2009]
//=========================================================================
SystemAllocator::pointer_type
SystemAllocator::Allocate(size_type byteSize, size_type alignment, int flags, const char* name, const char* fileName, int lineNum, unsigned int suppressStackRecord)
{
    if (byteSize == 0)
    {
        return nullptr;
    }
    AZ_Assert(byteSize > 0, "You can not allocate 0 bytes!");
    AZ_Assert((alignment & (alignment - 1)) == 0, "Alignment must be power of 2!");

    byteSize = MemorySizeAdjustedUp(byteSize);
    SystemAllocator::pointer_type address = m_allocator->Allocate(byteSize, alignment, flags, name, fileName, lineNum, suppressStackRecord + 1);

    if (address == nullptr)
    {
        // Free all memory we can and try again!
        AllocatorManager::Instance().GarbageCollect();

        address = m_allocator->Allocate(byteSize, alignment, flags, name, fileName, lineNum, suppressStackRecord + 1);
    }

    if (address == nullptr)
    {
        byteSize = MemorySizeAdjustedDown(byteSize);  // restore original size

        if (!OnOutOfMemory(byteSize, alignment, flags, name, fileName, lineNum))
        {
            if (GetRecords())
            {
                EBUS_EVENT(Debug::MemoryDrillerBus, DumpAllAllocations);
            }
        }
    }

    AZ_Assert(address != nullptr, "SystemAllocator: Failed to allocate %d bytes aligned on %d (flags: 0x%08x) %s : %s (%d)!", byteSize, alignment, flags, name ? name : "(no name)", fileName ? fileName : "(no file name)", lineNum);

    AZ_PROFILE_MEMORY_ALLOC_EX(MemoryReserved, fileName, lineNum, address, byteSize, name);
    AZ_MEMORY_PROFILE(ProfileAllocation(address, byteSize, alignment, name, fileName, lineNum, suppressStackRecord + 1));

    return address;
}

//=========================================================================
// DeAllocate
// [9/2/2009]
//=========================================================================
void
SystemAllocator::DeAllocate(pointer_type ptr, size_type byteSize, size_type alignment)
{
    byteSize = MemorySizeAdjustedUp(byteSize);
    AZ_PROFILE_MEMORY_FREE(MemoryReserved, ptr);
    AZ_MEMORY_PROFILE(ProfileDeallocation(ptr, byteSize, alignment, nullptr));
    m_allocator->DeAllocate(ptr, byteSize, alignment);
}

//=========================================================================
// ReAllocate
// [9/13/2011]
//=========================================================================
SystemAllocator::pointer_type
SystemAllocator::ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment)
{
    newSize = MemorySizeAdjustedUp(newSize);

    AZ_MEMORY_PROFILE(ProfileReallocationBegin(ptr, newSize));
    AZ_PROFILE_MEMORY_FREE(MemoryReserved, ptr);
    pointer_type newAddress = m_allocator->ReAllocate(ptr, newSize, newAlignment);
    AZ_PROFILE_MEMORY_ALLOC(MemoryReserved, newAddress, newSize, "SystemAllocator realloc");
    AZ_MEMORY_PROFILE(ProfileReallocationEnd(ptr, newAddress, newSize, newAlignment));

    return newAddress;
}

//=========================================================================
// Resize
// [8/12/2011]
//=========================================================================
SystemAllocator::size_type
SystemAllocator::Resize(pointer_type ptr, size_type newSize)
{
    newSize = MemorySizeAdjustedUp(newSize);
    size_type resizedSize = m_allocator->Resize(ptr, newSize);

    AZ_MEMORY_PROFILE(ProfileResize(ptr, resizedSize));

    return MemorySizeAdjustedDown(resizedSize);
}

//=========================================================================
//
// [8/12/2011]
//=========================================================================
SystemAllocator::size_type
SystemAllocator::AllocationSize(pointer_type ptr)
{
    size_type allocSize = MemorySizeAdjustedDown(m_allocator->AllocationSize(ptr));

    return allocSize;
}
