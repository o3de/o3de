/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Memory/AllocationRecords.h>

namespace AZ
{
    template<class Allocator>
    class PoolAllocation;

    /**
     * Pools allocator schema
     * Specialized allocation where we pool objects at cost of using more memory (most of the time).
     * Pool Allocator is NOT thread safe, if you if need a thread safe version
     * use ThreadPool Schema or do the sync yourself.
     */
    class PoolSchema 
        : public IAllocatorSchema
    {
    public:
        /**
        * Pool allocator descriptor.
        * We will create buckets for each allocation size, we will have m_maxAllocationSize / m_minAllocationSize buckets.
        * You need to make sure that you can divide m_maxAllocationSize to m_minAllocationSize without fraction.
        * Keep in mind that the pool allocator is a careful balance between pagesize, min and max allocations,
        * as we can waste lots of memory with too many buckets.
        */
        struct Descriptor
        {
            AZ_TYPE_INFO(Descriptor, "{DB802BA9-33E0-4E7A-A79B-CC6EBC39DC82}")
            Descriptor()
                : m_pageSize(4 * 1024)
                , m_minAllocationSize(8)
                , m_maxAllocationSize(512)
                , m_isDynamic(true)
                , m_numStaticPages(0)
                , m_pageAllocator(nullptr)

            {}
            size_t              m_pageSize;             ///< Page size in bytes.
            size_t              m_minAllocationSize;    ///< Minimal allocation size >= 8 bytes and power of 2
            size_t              m_maxAllocationSize;    ///< Maximum allocation size
            bool                m_isDynamic;            ///< True if we allocate pages at runtime, false if we allocate at create.
            /**
             * Number of static pages defined how many pages will be allocated at the start. If the m_isDynamic is true
             * this is the minimum number of pages we will have allocated at all times, otherwise the total number of pages supported.
             */
            unsigned int        m_numStaticPages;
            IAllocatorSchema*   m_pageAllocator;        ///< If you provide this interface we will use it for page allocations, otherwise SystemAllocator will be used.
        };
        AZ_TYPE_INFO(PoolSchema, "{3BFAC20A-DBE9-4C94-AC20-8417FD9C9CB2}")

        PoolSchema(const Descriptor& desc = Descriptor());
        ~PoolSchema();

        bool Create(const Descriptor& desc);
        bool Destroy();

        pointer allocate(size_type byteSize, size_type alignment) override;
        void deallocate(pointer ptr, size_type byteSize, size_type alignment) override;
        pointer reallocate(pointer ptr, size_type newSize, size_type newAlignment) override;
        size_type get_allocated_size(pointer ptr, align_type alignment) const override;

        /// Return unused memory to the OS. Don't call this too often because you will force unnecessary allocations.
        void GarbageCollect() override;

        size_type NumAllocatedBytes() const override;

    protected:
        PoolSchema(const PoolSchema&);
        PoolSchema& operator=(const PoolSchema&);

        class PoolSchemaImpl*             m_impl;
    };

    struct ThreadPoolData;

    /**
        * Thread safe pool allocator. For pool details \ref PoolSchema.
        * IMPORTNAT: Keep in mind the thread pool allocator will create separate pools,
        * for each thread. So there will be some memory overhead, especially if you use fixed pool sizes.
        */
    class ThreadPoolSchema
        : public IAllocatorSchema
    {
    public:
        // Functions for getting an instance of a ThreadPoolData when using thread local storage
        typedef ThreadPoolData* (* GetThreadPoolData)();
        typedef void(* SetThreadPoolData)(ThreadPoolData*);

        /**
        * Pool allocator descriptor.
        */
        typedef PoolSchema::Descriptor Descriptor;

        ThreadPoolSchema(GetThreadPoolData getThreadPoolData, SetThreadPoolData setThreadPoolData);
        ~ThreadPoolSchema();

        bool Create(const Descriptor& desc);
        bool Destroy();

        pointer allocate(size_type byteSize, size_type alignment) override;
        void deallocate(pointer ptr, size_type byteSize, size_type alignment) override;
        pointer reallocate(pointer ptr, size_type newSize, size_type newAlignment) override;
        size_type get_allocated_size(pointer ptr, align_type alignment) const override;
        /// Return unused memory to the OS. Don't call this too often because you will force unnecessary allocations.
        void GarbageCollect() override;

        size_type NumAllocatedBytes() const override;

    protected:
        ThreadPoolSchema(const ThreadPoolSchema&);
        ThreadPoolSchema& operator=(const ThreadPoolSchema&);

        class ThreadPoolSchemaImpl* m_impl;
        GetThreadPoolData m_threadPoolGetter;
        SetThreadPoolData m_threadPoolSetter;
    };

    /**
     * Helper class to allow multiple instances of ThreadPool that can
     * operate independent from each other. Your thread pool allocator should inherit from that class.
     */
    template<class Allocator>
    class ThreadPoolSchemaHelper
        : public ThreadPoolSchema
    {
    public:
        ThreadPoolSchemaHelper(const Descriptor& desc = Descriptor())
            : ThreadPoolSchema(&GetThreadPoolData, &SetThreadPoolData)
        {
            // Descriptor is ignored here; Create() must be called directly on the schema
            (void)desc;
        }

        AZ_TYPE_INFO(ThreadPoolSchemaHelper, "{43DFADCF-DE57-4056-88CB-04790A140FB3}")

    protected:

        static ThreadPoolData* GetThreadPoolData()
        {
            return m_threadData;
        }

        static void SetThreadPoolData(ThreadPoolData* data)
        {
            m_threadData = data;
        }

        static AZ_THREAD_LOCAL ThreadPoolData*  m_threadData;
    };

    template<class Allocator>
    AZ_THREAD_LOCAL ThreadPoolData* ThreadPoolSchemaHelper<Allocator>::m_threadData = 0;
}

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
            using pointer = typename Base::pointer;
            using size_type = typename Base::size_type;
            using difference_type = typename Base::difference_type;

            AZ_RTTI((PoolAllocatorHelper, "{813b4b74-7381-4c62-b475-3f66efbcb615}", Schema), Base)

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
            // IAllocatorSchema
            pointer reallocate(pointer ptr, size_type newSize, size_type newAlignment) override
            {
                (void)ptr;
                (void)newSize;
                (void)newAlignment;
                AZ_Assert(false, "Not supported!");
                return nullptr;
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

        using Base = Internal::PoolAllocatorHelper<PoolSchema>;

        AZ_RTTI(PoolAllocator, "{D3DC61AF-0949-4BFA-87E0-62FA03A4C025}", Base)
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

        using Base = ThreadPoolBase<ThreadPoolAllocator>;

        AZ_RTTI(ThreadPoolAllocator, "{05B4857F-CD06-4942-99FD-CA6A7BAE855A}", Base)
    };
} // namespace AZ
