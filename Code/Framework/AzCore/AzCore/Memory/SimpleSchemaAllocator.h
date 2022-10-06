/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>
#include <AzCore/std/base.h>
#include <AzCore/std/typetraits/aligned_storage.h>
#include <AzCore/std/typetraits/alignment_of.h>
#include <AzCore/Memory/AllocatorBase.h>
#include <AzCore/Debug/MemoryProfiler.h>

namespace AZ
{
    /**
    * A basic, default allocator implementation using a custom schema.
    */
    template <class Schema, bool ProfileAllocations=true, bool ReportOutOfMemory=true>
    class SimpleSchemaAllocator
        : public AllocatorBase
    {
    public:
        AZ_RTTI((SimpleSchemaAllocator, "{32019C72-6E33-4EF9-8ABA-748055D94EB2}", Schema), AllocatorBase)

        using pointer = typename Schema::pointer;
        using size_type = typename Schema::size_type;
        using difference_type = typename Schema::difference_type;

        SimpleSchemaAllocator()
        {
            SetProfilingActive(ProfileAllocations);
            Create();
        }

        ~SimpleSchemaAllocator() override
        {
            if (m_schema)
            {
                reinterpret_cast<Schema*>(&m_schemaStorage)->~Schema();
            }
            m_schema = nullptr;
        }

        bool Create()
        {
            m_schema = new (&m_schemaStorage) Schema();
            return m_schema != nullptr;
        }

        AllocatorDebugConfig GetDebugConfig() override
        {
            return AllocatorDebugConfig();
        }

        //---------------------------------------------------------------------
        // IAllocator
        //---------------------------------------------------------------------
        pointer allocate(size_type byteSize, size_type alignment) override
        {
            byteSize = MemorySizeAdjustedUp(byteSize);
            pointer ptr = m_schema->allocate(byteSize, alignment);

            if (ProfileAllocations)
            {
                AZ_MEMORY_PROFILE(ProfileAllocation(ptr, byteSize, alignment, 1));
            }

            AZ_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option") // conditional expression is constant
            if (ReportOutOfMemory && !ptr)
            AZ_POP_DISABLE_WARNING
            {
                OnOutOfMemory(byteSize, alignment);
            }

            return ptr;
        }

        void deallocate(pointer ptr, size_type byteSize = 0, size_type alignment = 0) override
        {
            byteSize = MemorySizeAdjustedUp(byteSize);

            if (ProfileAllocations)
            {
                AZ_PROFILE_MEMORY_FREE(MemoryReserved, ptr);
                AZ_MEMORY_PROFILE(ProfileDeallocation(ptr, byteSize, alignment, nullptr));
            }

            m_schema->deallocate(ptr, byteSize, alignment);
        }

        pointer reallocate(pointer ptr, size_type newSize, size_type newAlignment = 1) override
        {
            if (ProfileAllocations)
            {
                AZ_PROFILE_MEMORY_FREE(MemoryReserved, ptr);
            }

            newSize = MemorySizeAdjustedUp(newSize);

            pointer newPtr = m_schema->reallocate(ptr, newSize, newAlignment);

            if (ProfileAllocations)
            {
                AZ_PROFILE_MEMORY_ALLOC(MemoryReserved, newPtr, newSize, GetName());
                AZ_MEMORY_PROFILE(ProfileReallocation(ptr, newPtr, newSize, newAlignment));
            }

            AZ_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option") // conditional expression is constant
            if (ReportOutOfMemory && newSize && !newPtr)
            AZ_POP_DISABLE_WARNING
            {
                OnOutOfMemory(newSize, newAlignment);
            }

            return newPtr;
        }
                
        size_type get_allocated_size(pointer ptr, align_type alignment = 1) const override
        {
            return MemorySizeAdjustedDown(m_schema->get_allocated_size(ptr, alignment));
        }
        
        void GarbageCollect() override
        {
            m_schema->GarbageCollect();
        }

        size_type NumAllocatedBytes() const override
        {
            return m_schema->NumAllocatedBytes();
        }

    protected:
        IAllocator* m_schema{};
    private:
        typename AZStd::aligned_storage<sizeof(Schema), AZStd::alignment_of<Schema>::value>::type m_schemaStorage;
    };
}
