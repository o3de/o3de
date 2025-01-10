/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>

#include <AzCore/Memory/AllocatorManager.h>
#include <AzCore/Memory/AllocationRecords.h>
#include <AzCore/Memory/OSAllocator.h>

#include <AzCore/std/functional.h>

#include <AzCore/Debug/MemoryProfiler.h>
#include <AzCore/Debug/Profiler.h>
#include <memory>

// HPHA uses the high performance heap allocator for system allocations
#define AZCORE_SYSTEM_ALLOCATOR_HPHA 1

// malloc uses basic OS malloc for system allocations.  This is useful when using ASAN or other memory checking such as the CRT debug heap.
// when ASAN is enabled by CMake, this is set by default.  See AZCORE_SYSTEM_ALLOCATOR_MALLOC in AzCore's CMakeLists.txt
#define AZCORE_SYSTEM_ALLOCATOR_MALLOC 2

#if !defined(AZCORE_SYSTEM_ALLOCATOR)
    #define AZCORE_SYSTEM_ALLOCATOR AZCORE_SYSTEM_ALLOCATOR_HPHA
#endif

#if (AZCORE_SYSTEM_ALLOCATOR != AZCORE_SYSTEM_ALLOCATOR_HPHA) && (AZCORE_SYSTEM_ALLOCATOR != AZCORE_SYSTEM_ALLOCATOR_MALLOC)
    #error AZCORE_SYSTEM_ALLOCATOR is an invalid value, it needs to be either AZCORE_SYSTEM_ALLOCATOR_HPHA or AZCORE_SYSTEM_ALLOCATOR_MALLOC
#endif

#include <AzCore/Memory/HphaAllocator.h>

namespace AZ
{
    //////////////////////////////////////////////////////////////////////////
    AZ_TYPE_INFO_WITH_NAME_IMPL(SystemAllocator, "SystemAllocator", "{607C9CDF-B81F-4C5F-B493-2AD9C49023B7}");
    AZ_RTTI_NO_TYPE_INFO_IMPL(SystemAllocator, AllocatorBase);

    SystemAllocator::SystemAllocator()
    {
        AllocatorInstance<OSAllocator>::Get();
        Create();
        PostCreate();
    }

    //=========================================================================
    // ~SystemAllocator
    //=========================================================================
    SystemAllocator::~SystemAllocator()
    {
        PreDestroy();
        Destroy();
    }

    //=========================================================================
    // ~Create
    // [9/2/2009]
    //=========================================================================
    bool SystemAllocator::Create()
    {
        m_subAllocator = AZStd::make_unique<HphaSchema>();
        return true;
    }

    AllocatorDebugConfig SystemAllocator::GetDebugConfig()
    {
        return AllocatorDebugConfig()
            .StackRecordLevels(O3DE_STACK_CAPTURE_DEPTH)
            .UsesMemoryGuards()
            .MarksUnallocatedMemory()
            .ExcludeFromDebugging(false);
    }

    //=========================================================================
    // Allocate
    // [9/2/2009]
    //=========================================================================
    AllocateAddress SystemAllocator::allocate(size_type byteSize, size_type alignment)
    {
        if (byteSize == 0)
        {
            return AllocateAddress{};
        }

        AZ_Assert(byteSize > 0, "You can not allocate 0 or negative bytes!");
        AZ_Assert((alignment & (alignment - 1)) == 0, "Alignment must be power of 2!");

#if (AZCORE_SYSTEM_ALLOCATOR == AZCORE_SYSTEM_ALLOCATOR_MALLOC)
        AllocateAddress address(AZ_OS_MALLOC(byteSize, alignment), byteSize);
#else
        byteSize = MemorySizeAdjustedUp(byteSize);
        AllocateAddress address =
            m_subAllocator->allocate(byteSize, alignment);

        if (address == nullptr)
        {
            // Free all memory we can and try again!
            AllocatorManager::Instance().GarbageCollect();

            address = m_subAllocator->allocate(byteSize, alignment);
        }

        if (address == nullptr)
        {
            byteSize = MemorySizeAdjustedDown(byteSize); // restore original size
        }
#endif
        AZ_Assert(
            address != nullptr, "SystemAllocator: Failed to allocate %zu bytes aligned on %zu!", byteSize,
            alignment);

        AZ_PROFILE_MEMORY_ALLOC_EX(MemoryReserved, fileName, lineNum, address, byteSize, name);
        AZ_MEMORY_PROFILE(ProfileAllocation(address, byteSize, alignment, 1));

        return address;
    }

    //=========================================================================
    // DeAllocate
    // [9/2/2009]
    //=========================================================================
    auto SystemAllocator::deallocate(pointer ptr, size_type byteSize, [[maybe_unused]] size_type alignment) -> size_type
    {
        #if (AZCORE_SYSTEM_ALLOCATOR == AZCORE_SYSTEM_ALLOCATOR_MALLOC)
            AZ_PROFILE_MEMORY_FREE(MemoryReserved, ptr);
            AZ_MEMORY_PROFILE(ProfileDeallocation(ptr, byteSize, alignment, nullptr));
            AZ_OS_FREE(ptr);
            return byteSize;
        #else
            byteSize = MemorySizeAdjustedUp(byteSize);
            AZ_PROFILE_MEMORY_FREE(MemoryReserved, ptr);
            AZ_MEMORY_PROFILE(ProfileDeallocation(ptr, byteSize, alignment, nullptr));
            return m_subAllocator->deallocate(ptr, byteSize, alignment);
        #endif
    }

    //=========================================================================
    // ReAllocate
    // [9/13/2011]
    //=========================================================================
    AllocateAddress SystemAllocator::reallocate(pointer ptr, size_type newSize, size_type newAlignment)
    {
        #if (AZCORE_SYSTEM_ALLOCATOR == AZCORE_SYSTEM_ALLOCATOR_MALLOC)
            AZ_PROFILE_MEMORY_FREE(MemoryReserved, ptr);
            AllocateAddress newAddress(AZ_OS_REALLOC(ptr, newSize, newAlignment), newSize);
            [[maybe_unused]] const size_type allocatedSize = newSize;
        #else
            newSize = MemorySizeAdjustedUp(newSize);
            AZ_PROFILE_MEMORY_FREE(MemoryReserved, ptr);
            AllocateAddress newAddress = m_subAllocator->reallocate(ptr, newSize, newAlignment);
            [[maybe_unused]] const size_type allocatedSize = get_allocated_size(newAddress, 1);
        #endif

        AZ_PROFILE_MEMORY_ALLOC(MemoryReserved, newAddress, newSize, "SystemAllocator realloc");
        AZ_MEMORY_PROFILE(ProfileReallocation(ptr, newAddress, allocatedSize, newAlignment));

        return newAddress;
    }

    //=========================================================================
    //
    // [8/12/2011]
    //=========================================================================
    auto SystemAllocator::get_allocated_size(pointer ptr, align_type alignment) const -> size_type
    {
        #if (AZCORE_SYSTEM_ALLOCATOR == AZCORE_SYSTEM_ALLOCATOR_MALLOC)
            return AZ_OS_MSIZE(ptr, alignment);
        #else
            size_type allocSize = MemorySizeAdjustedDown(m_subAllocator->get_allocated_size(ptr, alignment));
            return allocSize;
        #endif
    }

} // namespace AZ
