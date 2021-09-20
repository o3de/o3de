/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/BestFitExternalMapAllocator.h>
#include <AzCore/Memory/BestFitExternalMapSchema.h>

#include <AzCore/Memory/AllocationRecords.h>
#include <AzCore/Memory/MemoryDrillerBus.h>

#include <AzCore/std/functional.h>

using namespace AZ;

//=========================================================================
// BestFitExternalMapAllocator
// [1/28/2011]
//=========================================================================
BestFitExternalMapAllocator::BestFitExternalMapAllocator()
    : AllocatorBase(this, "BestFitExternalMapAllocator", "Best fit allocator with external tracking storage!")
    , m_schema(nullptr)
{}

//=========================================================================
// Create
// [1/28/2011]
//=========================================================================
bool
BestFitExternalMapAllocator::Create(const Descriptor& desc)
{
    AZ_Assert(IsReady() == false, "BestFitExternalMapAllocator was already created!");
    if (IsReady())
    {
        return false;
    }

    bool isReady = true;

    m_desc = desc;
    BestFitExternalMapSchema::Descriptor schemaDesc;
    schemaDesc.m_mapAllocator = desc.m_mapAllocator;
    schemaDesc.m_memoryBlock = desc.m_memoryBlock;
    schemaDesc.m_memoryBlockByteSize = desc.m_memoryBlockByteSize;

    m_schema = azcreate(BestFitExternalMapSchema, (schemaDesc), SystemAllocator);
    if (m_schema == nullptr)
    {
        isReady = false;
    }

    return isReady;
}

//=========================================================================
// Destroy
// [1/28/2011]
//=========================================================================
void
BestFitExternalMapAllocator::Destroy()
{
    azdestroy(m_schema, SystemAllocator);
    m_schema = nullptr;
}

AllocatorDebugConfig BestFitExternalMapAllocator::GetDebugConfig()
{
    return AllocatorDebugConfig()
        .ExcludeFromDebugging(!m_desc.m_allocationRecords)
        .StackRecordLevels(m_desc.m_stackRecordLevels)
        .MarksUnallocatedMemory(false)
        .UsesMemoryGuards(false);
}

//=========================================================================
// Allocate
// [1/28/2011]
//=========================================================================
BestFitExternalMapAllocator::pointer_type
BestFitExternalMapAllocator::Allocate(size_type byteSize, size_type alignment, int flags, const char* name, const char* fileName, int lineNum, unsigned int suppressStackRecord)
{
    (void)suppressStackRecord;

    AZ_Assert(byteSize > 0, "You can not allocate 0 bytes!");
    AZ_Assert((alignment & (alignment - 1)) == 0, "Alignment must be power of 2!");
    byteSize = MemorySizeAdjustedUp(byteSize);

    BestFitExternalMapAllocator::pointer_type address = m_schema->Allocate(byteSize, alignment, flags);
    if (address == nullptr)
    {
        if (!OnOutOfMemory(byteSize, alignment, flags, name, fileName, lineNum))
        {
            if (GetRecords())
            {
                EBUS_EVENT(Debug::MemoryDrillerBus, DumpAllAllocations);
            }
        }
    }

    AZ_Assert(address != nullptr, "BestFitExternalMapAllocator: Failed to allocate %d bytes aligned on %d (flags: 0x%08x) %s : %s (%d)!", byteSize, alignment, flags, name ? name : "(no name)", fileName ? fileName : "(no file name)", lineNum);
    AZ_MEMORY_PROFILE(ProfileAllocation(address, byteSize, alignment, name, fileName, lineNum, suppressStackRecord + 1));

    return address;
}

//=========================================================================
// DeAllocate
// [1/28/2011]
//=========================================================================
void
BestFitExternalMapAllocator::DeAllocate(pointer_type ptr, size_type byteSize, size_type alignment)
{
    byteSize = MemorySizeAdjustedUp(byteSize);
    AZ_MEMORY_PROFILE(ProfileDeallocation(ptr, byteSize, alignment, nullptr));

    (void)byteSize;
    (void)alignment;
    m_schema->DeAllocate(ptr);
}

//=========================================================================
// Resize
// [1/28/2011]
//=========================================================================
BestFitExternalMapAllocator::size_type
BestFitExternalMapAllocator::Resize(pointer_type ptr, size_type newSize)
{
    (void)ptr;
    (void)newSize;
    /* todo */
    return 0;
}

//=========================================================================
// ReAllocate
// [9/13/2011]
//=========================================================================
BestFitExternalMapAllocator::pointer_type
BestFitExternalMapAllocator::ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment)
{
    (void)ptr;
    (void)newSize;
    (void)newAlignment;
    AZ_Assert(false, "Not supported!");
    return nullptr;
}

//=========================================================================
// AllocationSize
// [1/28/2011]
//=========================================================================
BestFitExternalMapAllocator::size_type
BestFitExternalMapAllocator::AllocationSize(pointer_type ptr)
{
    return MemorySizeAdjustedDown(m_schema->AllocationSize(ptr));
}

//=========================================================================
// NumAllocatedBytes
// [1/28/2011]
//=========================================================================
BestFitExternalMapAllocator::size_type
BestFitExternalMapAllocator::NumAllocatedBytes() const
{
    return m_schema->NumAllocatedBytes();
}

//=========================================================================
// Capacity
// [1/28/2011]
//=========================================================================
BestFitExternalMapAllocator::size_type
BestFitExternalMapAllocator::Capacity() const
{
    return m_schema->Capacity();
}

//=========================================================================
// GetMaxAllocationSize
// [1/28/2011]
//=========================================================================
BestFitExternalMapAllocator::size_type
BestFitExternalMapAllocator::GetMaxAllocationSize() const
{
    return m_schema->GetMaxAllocationSize();
}

auto BestFitExternalMapAllocator::GetMaxContiguousAllocationSize() const -> size_type
{
    return m_schema->GetMaxContiguousAllocationSize();
}

//=========================================================================
// GetSubAllocator
// [1/28/2011]
//=========================================================================
IAllocatorAllocate*
BestFitExternalMapAllocator::GetSubAllocator()
{
    return m_schema->GetSubAllocator();
}
