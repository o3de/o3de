/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZCORE_POOL_ALLOCATOR_H
#define AZCORE_POOL_ALLOCATOR_H

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Memory/PoolSchema.h>
#include <AzCore/Memory/AllocationRecords.h>

#include <AzCore/Memory/MemoryDrillerBus.h>


namespace AZ
{
    template<class Allocator>
    class PoolAllocation;
    namespace Internal
    {
        /*!
        * Template you can use to create your own thread pool allocators, as you can't inherit from ThreadPoolAllocator.
        * This is the case because we use tread local storage and we need separate "static" instance for each allocator.
        */
        template<class Schema>
        class PoolAllocatorHelper
            : public SimpleSchemaAllocator<Schema, typename Schema::Descriptor, /* ProfileAllocations */ true, /* ReportOutOfMemory */ false>
        {
        public:
            using Base = SimpleSchemaAllocator<Schema, typename Schema::Descriptor, true, false>;
            using pointer_type = typename Base::pointer_type;
            using size_type = typename Base::size_type;
            using difference_type = typename Base::difference_type;

            PoolAllocatorHelper(const char* name, const char* desc) : Base(name, desc)
            {
            }

            struct Descriptor
                : public Schema::Descriptor
            {
                Descriptor()
                    : m_allocationRecords(true)
                    , m_isMarkUnallocatedMemory(false)
                    , m_isMemoryGuards(false)
                    , m_stackRecordLevels(5)
                {}

                bool m_allocationRecords; ///< True if we want to track memory allocations, otherwise false.
                bool m_isMarkUnallocatedMemory; ///< Sets unallocated memory to default values (if m_allocationRecords is true)
                bool m_isMemoryGuards; ///< Enables memory guards for stomp detection. Keep in mind that this will change the memory "profile" a lot as we usually double every pool allocation.
                unsigned char m_stackRecordLevels; ///< If stack recording is enabled, how many stack levels to record. (if m_allocationRecords is true)
            };

            bool Create(const Descriptor& descriptor)
            {
                AZ_Assert(this->IsReady() == false, "Allocator was already created!");
                if (this->IsReady())
                {
                    return false;
                }

                m_desc = descriptor;

                if (m_desc.m_minAllocationSize < 8)
                {
                    m_desc.m_minAllocationSize = 8;
                }
                if (m_desc.m_maxAllocationSize < m_desc.m_minAllocationSize)
                {
                    m_desc.m_maxAllocationSize = m_desc.m_minAllocationSize;
                }

                if (m_desc.m_allocationRecords && m_desc.m_isMemoryGuards)
                {
                    m_desc.m_maxAllocationSize = AZ_SIZE_ALIGN_UP(m_desc.m_maxAllocationSize + AZ_SIZE_ALIGN_UP(sizeof(Debug::GuardValue), m_desc.m_minAllocationSize), m_desc.m_minAllocationSize);
                }

                bool isReady = static_cast<Base*>(this)->Create(m_desc);

                if (isReady)
                {
                    isReady = static_cast<Schema*>(this->m_schema)->Create(m_desc);
                }

                return isReady;
            }

            void Destroy() override
            {
                static_cast<Schema*>(this->m_schema)->Destroy();
                Base::Destroy();
            }
                        
            AllocatorDebugConfig GetDebugConfig() override
            {
                return AllocatorDebugConfig()
                    .ExcludeFromDebugging(!m_desc.m_allocationRecords)
                    .StackRecordLevels(m_desc.m_stackRecordLevels)
                    .MarksUnallocatedMemory(m_desc.m_isMarkUnallocatedMemory)
                    .UsesMemoryGuards(m_desc.m_isMemoryGuards);
            }

            //////////////////////////////////////////////////////////////////////////
            // IAllocatorAllocate
            pointer_type ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment) override
            {
                (void)ptr;
                (void)newSize;
                (void)newAlignment;
                AZ_Assert(false, "Not supported!");
                return nullptr;
            }

            size_type Resize(pointer_type ptr, size_type newSize) override
            {
                (void)ptr;
                (void)newSize;
                // \todo return the node size
                return 0;
            }

            //////////////////////////////////////////////////////////////////////////

            PoolAllocatorHelper& operator=(const PoolAllocatorHelper&) = delete;

        private:
            Descriptor m_desc;
        };
    }

    /*!
     * Pool allocator
     * Specialized allocation for extremely fast small object memory allocations.
     * It can allocate sized from m_minAllocationSize to m_maxPoolSize.
     * Pool Allocator is NOT thread safe, if you if need a thread safe version
     * use PoolAllocatorThreadSafe or do the sync yourself.
     */
    class PoolAllocator
        : public Internal::PoolAllocatorHelper<PoolSchema>
    {
    public:
        AZ_CLASS_ALLOCATOR(PoolAllocator, SystemAllocator, 0);
        AZ_TYPE_INFO(PoolAllocator, "{D3DC61AF-0949-4BFA-87E0-62FA03A4C025}");

        using Base = Internal::PoolAllocatorHelper<PoolSchema>;

        PoolAllocator(const char* name = "PoolAllocator", const char* desc = "Generic pool allocator for small objects")
            : Base(name, desc)
        {
        }
    };

    template<class Allocator>
    using ThreadPoolBase = Internal::PoolAllocatorHelper<ThreadPoolSchemaHelper<Allocator> >;

    /*!
     * Thread safe pool allocator. If you want to create your own thread pool heap,
     * inherit from ThreadPoolBase, as we need unique static variable for allocator type.
     */
    class ThreadPoolAllocator final
        : public ThreadPoolBase<ThreadPoolAllocator>
    {
    public:
        AZ_CLASS_ALLOCATOR(ThreadPoolAllocator, SystemAllocator, 0);
        AZ_TYPE_INFO(ThreadPoolAllocator, "{05B4857F-CD06-4942-99FD-CA6A7BAE855A}");

        using Base = ThreadPoolBase<ThreadPoolAllocator>;

        ThreadPoolAllocator()
            : Base("PoolAllocatorThreadSafe", "Generic thread safe pool allocator for small objects")
        {
        }

        //////////////////////////////////////////////////////////////////////////
    };
}

#endif // AZCORE_POOL_ALLOCATOR_H
#pragma once


