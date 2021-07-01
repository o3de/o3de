/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/AllocatorBase.h>
#include <AzCore/Memory/HphaSchema.h>

#define AZCORE_SYS_ALLOCATOR_HPPA
//#define AZCORE_SYS_ALLOCATOR_MALLOC

#ifdef AZCORE_SYS_ALLOCATOR_HPPA
#   include <AzCore/Memory/HphaSchema.h>
#elif defined(AZCORE_SYS_ALLOCATOR_MALLOC)
#   include <AzCore/Memory/MallocSchema.h>
#else
#   include <AzCore/Memory/HeapSchema.h>
#endif

namespace AZ
{

#ifdef AZCORE_SYS_ALLOCATOR_HPPA
    typedef AZ::HphaSchema LegacyAllocatorSchema;
#elif defined(AZCORE_SYS_ALLOCATOR_MALLOC)
    typedef AZ::MallocSchema LegacyAllocatorSchema;
#else
    typedef AZ::HeapSchema LegacyAllocatorSchema;
#endif

    struct LegacyAllocatorDescriptor
        : public LegacyAllocatorSchema::Descriptor
    {
        LegacyAllocatorDescriptor()
        {
            // pull 32MB from the OS at a time
#ifdef AZCORE_SYS_ALLOCATOR_HPPA
            m_systemChunkSize = 32 * 1024 * 1024;
#endif
        }
    };

    class LegacyAllocator
        : public SimpleSchemaAllocator<AZ::HphaSchema, LegacyAllocatorDescriptor>
    {
    public:
        AZ_TYPE_INFO(LegacyAllocator, "{17FC25A4-92D9-48C5-BB85-7F860FCA2C6F}");

        using Descriptor = LegacyAllocatorDescriptor;
        using Base = SimpleSchemaAllocator<AZ::HphaSchema, LegacyAllocatorDescriptor>;
        
        LegacyAllocator()
            : Base("LegacyAllocator", "Allocator for Legacy CryEngine systems")
        {
        }

        pointer_type Allocate(size_type byteSize, size_type alignment, int flags = 0, const char* name = 0, const char* fileName = 0, int lineNum = 0, unsigned int suppressStackRecord = 0) override
        {
            if (alignment == 0)
            {
                // Some STL containers, like std::vector, are assuming a specific minimum alignment. seems to have a requirement
                // Take a look at _Allocate_manually_vector_aligned in xmemory0
                alignment = sizeof(void*) * 2; 
            }

            pointer_type ptr = m_schema->Allocate(byteSize, alignment, flags, name, fileName, lineNum, suppressStackRecord);
            AZ_PROFILE_MEMORY_ALLOC_EX(AZ::Debug::ProfileCategory::MemoryReserved, fileName, lineNum, ptr, byteSize, name ? name : GetName());
            AZ_MEMORY_PROFILE(ProfileAllocation(ptr, byteSize, alignment, name, fileName, lineNum, suppressStackRecord));
            AZ_Assert(ptr || byteSize == 0, "OOM - Failed to allocate %zu bytes from LegacyAllocator", byteSize);
            return ptr;
        }

        // DeAllocate with file/line, to track when allocs were freed from Cry
        void DeAllocate(pointer_type ptr, [[maybe_unused]] const char* file, [[maybe_unused]] const int line, size_type byteSize = 0, size_type alignment = 0)
        {
            AZ_PROFILE_MEMORY_FREE_EX(AZ::Debug::ProfileCategory::MemoryReserved, file, line, ptr);
            AZ_MEMORY_PROFILE(ProfileDeallocation(ptr, byteSize, alignment, nullptr));
            m_schema->DeAllocate(ptr, byteSize, alignment);
        }

        // Realloc with file/line, because Cry uses realloc(nullptr) and realloc(ptr, 0) to mimic malloc/free
        pointer_type ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment, [[maybe_unused]] const char* file, [[maybe_unused]] const int line)
        {
            if (newAlignment == 0)
            {
                // Some STL containers, like std::vector, are assuming a specific minimum alignment. seems to have a requirement
                // Take a look at _Allocate_manually_vector_aligned in xmemory0
                newAlignment = sizeof(void*) * 2;
            }

            AZ_MEMORY_PROFILE(ProfileReallocationBegin(ptr, newSize));
            AZ_PROFILE_MEMORY_FREE_EX(AZ::Debug::ProfileCategory::MemoryReserved, file, line, ptr);
            pointer_type newPtr = m_schema->ReAllocate(ptr, newSize, newAlignment);
            AZ_PROFILE_MEMORY_ALLOC_EX(AZ::Debug::ProfileCategory::MemoryReserved, file, line, newPtr, newSize, "LegacyAllocator Realloc");
            AZ_MEMORY_PROFILE(ProfileReallocationEnd(ptr, newPtr, newSize, newAlignment));
            AZ_Assert(newPtr || newSize == 0, "OOM - Failed to reallocate %zu bytes from LegacyAllocator", newSize);
            return newPtr;
        }

        void DeAllocate(pointer_type ptr, size_type byteSize = 0, size_type alignment = 0) override
        {
            AZ_MEMORY_PROFILE(ProfileDeallocation(ptr, 0, 0, nullptr));
            Base::DeAllocate(ptr, byteSize, alignment);
        }

        pointer_type ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment) override
        {
            if (newAlignment == 0)
            {
                // Some STL containers, like std::vector, are assuming a specific minimum alignment. seems to have a requirement
                // Take a look at _Allocate_manually_vector_aligned in xmemory0
                newAlignment = sizeof(void*) * 2;
            }

            AZ_MEMORY_PROFILE(ProfileReallocationBegin(ptr, newSize));
            pointer_type newPtr = Base::ReAllocate(ptr, newSize, newAlignment);
            AZ_MEMORY_PROFILE(ProfileReallocationEnd(ptr, newPtr, newSize, newAlignment));
            AZ_Assert(newPtr || newSize == 0, "OOM - Failed to reallocate %zu bytes from LegacyAllocator", newSize);
            return newPtr;
        }
    };

    using StdLegacyAllocator = AZStdAlloc<LegacyAllocator>;

    // Specialize for the LegacyAllocator to provide one per module that does not use the
    // environment for its storage
    template <>
    class AllocatorInstance<LegacyAllocator> : public Internal::AllocatorInstanceBase<LegacyAllocator>
    {
    };

#if defined(AZ_PLATFORM_PROVO) || defined(AZ_PLATFORM_JASPER)
    struct GlobalAllocatorDescriptor
        : public AZ::HphaSchema::Descriptor
    {
        GlobalAllocatorDescriptor()
        {
            // pull 1MB from the OS at a time
            m_systemChunkSize = 1024 * 1024;
        }
    };

    class GlobalAllocator
        : public SimpleSchemaAllocator<AZ::HphaSchema, GlobalAllocatorDescriptor>
    {
    public:
        AZ_TYPE_INFO(GlobalAllocator, "{BC7861DA-AF7F-4FFD-A2F5-BAD89BDD77FD}");

        using Descriptor = GlobalAllocatorDescriptor;
        using Base = SimpleSchemaAllocator<AZ::HphaSchema, GlobalAllocatorDescriptor>;
        
        GlobalAllocator()
            : Base("GlobalAllocator", "Allocator for untracked new/delete/malloc/free")
        {
        }

        //---------------------------------------------------------------------
        // IAllocatorAllocate
        //---------------------------------------------------------------------
        pointer_type Allocate(size_type byteSize, size_type alignment, int flags = 0, const char* name = 0, const char* fileName = 0, int lineNum = 0, unsigned int suppressStackRecord = 0) override
        {
            // Note: We cannot put the asserts in the AllocateBase class because various allocators depend on allocations failing from some heap classes.
            pointer_type ptr = Base::Allocate(byteSize, alignment, flags, name, fileName, lineNum, suppressStackRecord);
            AZ_Assert(ptr, "OOM - Failed to allocate %zu bytes from GlobalAllocator", byteSize);
            return ptr;
        }

        pointer_type ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment) override
        {
            pointer_type newPtr = Base::ReAllocate(ptr, newSize, newAlignment);
            AZ_Assert(newPtr, "OOM - Failed to reallocate %zu bytes from GlobalAllocator", newSize);
            return newPtr;
        }

    };

    // Specialize for the GlobalAllocator to provide one per module that does not use the
    // environment for its storage
    template <>
    class AllocatorInstance<GlobalAllocator> : public Internal::AllocatorInstanceBase<GlobalAllocator, AllocatorStorage::ModuleStoragePolicy<GlobalAllocator, false>>
    {
    };
#endif
}
