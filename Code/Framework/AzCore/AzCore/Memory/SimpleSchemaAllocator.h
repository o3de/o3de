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
#include <AzCore/Memory/AllocatorManager.h>
#include <AzCore/Debug/MemoryProfiler.h>

namespace AZ
{
    class SimpleSchemaAllocatorBase
        : public AllocatorBase
    {
    public:
        AZ_RTTI(SimpleSchemaAllocatorBase, "{6B1DB724-B861-41A0-9FCF-6A5943007CA0}", AllocatorBase);
        using AllocatorBase::AllocatorBase;
        virtual IAllocator* GetSchema() const = 0;
    };

    /**
    * A basic, default allocator implementation using a custom schema.
    */
    template<class Schema, bool ProfileAllocations = true, bool ReportOutOfMemory = true>
    class SimpleSchemaAllocator
        : public SimpleSchemaAllocatorBase
    {
    public:
        AZ_RTTI((SimpleSchemaAllocator, "{32019C72-6E33-4EF9-8ABA-748055D94EB2}", Schema), SimpleSchemaAllocatorBase);

        using pointer = typename Schema::pointer;
        using size_type = typename Schema::size_type;
        using difference_type = typename Schema::difference_type;

        SimpleSchemaAllocator()
        {
            SetProfilingActive(ProfileAllocations);
            Create();

            // Register the SimpleSchemaAllocator with the Allocator Manager
            PostCreate();
        }

        ~SimpleSchemaAllocator() override
        {
            PreDestroy();
            if (m_schema)
            {
                reinterpret_cast<Schema*>(&m_schemaStorage)->~Schema();
            }
            m_schema = nullptr;
        }

        bool Create()
        {
            m_schema = new (&m_schemaStorage) Schema();

            // As the Simple Schema Allocator is registered
            // with the Allocator Manager, unregister the Schema from the manager
            auto& allocatorManager = AllocatorManager::Instance();
            allocatorManager.UnRegisterAllocator(m_schema);
            return m_schema != nullptr;
        }

        AllocatorDebugConfig GetDebugConfig() override
        {
            return AllocatorDebugConfig();
        }

        //---------------------------------------------------------------------
        // IAllocator
        //---------------------------------------------------------------------
        AllocateAddress allocate(size_type byteSize, size_type alignment) override
        {
            byteSize = MemorySizeAdjustedUp(byteSize);
            const AllocateAddress ptr = m_schema->allocate(byteSize, alignment);
            m_totalAllocatedBytes += ptr.GetAllocatedBytes();

            if constexpr (ProfileAllocations)
            {
                AZ_MEMORY_PROFILE(ProfileAllocation(ptr, byteSize, alignment, 1));
            }

            if constexpr (ReportOutOfMemory)
            {
                if (ptr == nullptr)
                {
                    OnOutOfMemory(byteSize, alignment);
                }
            }

            return ptr;
        }

        size_type deallocate(pointer ptr, size_type byteSize = 0, size_type alignment = 0) override
        {
            byteSize = MemorySizeAdjustedUp(byteSize);

            if constexpr (ProfileAllocations)
            {
                AZ_PROFILE_MEMORY_FREE(MemoryReserved, ptr);
                AZ_MEMORY_PROFILE(ProfileDeallocation(ptr, byteSize, alignment, nullptr));
            }

            const size_type bytesDeallocated = m_schema->deallocate(ptr, byteSize, alignment);
            m_totalAllocatedBytes -= bytesDeallocated;
            return bytesDeallocated;
        }

        AllocateAddress reallocate(pointer ptr, size_type newSize, size_type newAlignment = 1) override
        {
            if constexpr (ProfileAllocations)
            {
                AZ_PROFILE_MEMORY_FREE(MemoryReserved, ptr);
            }

            newSize = MemorySizeAdjustedUp(newSize);

            const size_type oldAllocatedSize = get_allocated_size(ptr, 1);
            AllocateAddress newPtr = m_schema->reallocate(ptr, newSize, newAlignment);
            m_totalAllocatedBytes += newPtr.GetAllocatedBytes() - oldAllocatedSize;

            if constexpr (ProfileAllocations)
            {
                AZ_PROFILE_MEMORY_ALLOC(MemoryReserved, newPtr, newSize, GetName());
                AZ_MEMORY_PROFILE(ProfileReallocation(ptr, newPtr, newSize, newAlignment));
            }

            if constexpr (ReportOutOfMemory)
            {
                if (newSize && newPtr == nullptr)
                {
                    OnOutOfMemory(newSize, newAlignment);
                }
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
            AZ_Assert(
                m_totalAllocatedBytes >= 0,
                R"(SchemaAllocator "%s": Total allocated bytes is less than zero with a value of %td. Was deallocate() invoked with an address )"
                "that is not associated with the allocator? This should never occur",
                GetName(), m_totalAllocatedBytes.load());
            return static_cast<size_type>(m_totalAllocatedBytes);
        }

        IAllocator* GetSchema() const override
        {
            return m_schema;
        }

    protected:
        IAllocator* m_schema{};
    private:
        AZStd::aligned_storage_for_t<Schema> m_schemaStorage;
        AZStd::atomic<ptrdiff_t> m_totalAllocatedBytes{};
    };
}
