/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZ_POOL_ALLOCATION_SCHEME_H
#define AZ_POOL_ALLOCATION_SCHEME_H

#include <AzCore/Memory/SystemAllocator.h>

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
        : public IAllocatorAllocate
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
            IAllocatorAllocate* m_pageAllocator;        ///< If you provide this interface we will use it for page allocations, otherwise SystemAllocator will be used.
        };

        PoolSchema(const Descriptor& desc = Descriptor());
        ~PoolSchema();

        bool Create(const Descriptor& desc);
        bool Destroy();

        pointer_type Allocate(size_type byteSize, size_type alignment, int flags, const char* name, const char* fileName, int lineNum, unsigned int suppressStackRecord) override;
        void DeAllocate(pointer_type ptr, size_type byteSize, size_type alignment) override;
        size_type Resize(pointer_type ptr, size_type newSize) override;
        pointer_type ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment) override;
        size_type AllocationSize(pointer_type ptr) override;

        /// Return unused memory to the OS. Don't call this too often because you will force unnecessary allocations.
        void GarbageCollect() override;

        size_type GetMaxContiguousAllocationSize() const override;
        size_type NumAllocatedBytes() const override;
        size_type Capacity() const override;
        IAllocatorAllocate* GetSubAllocator() override;

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
        : public IAllocatorAllocate
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

        pointer_type Allocate(size_type byteSize, size_type alignment, int flags, const char* name, const char* fileName, int lineNum, unsigned int suppressStackRecord) override;
        void DeAllocate(pointer_type ptr, size_type byteSize, size_type alignment) override;
        size_type Resize(pointer_type ptr, size_type newSize) override;
        pointer_type ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment) override;
        size_type AllocationSize(pointer_type ptr) override;
        /// Return unused memory to the OS. Don't call this too often because you will force unnecessary allocations.
        void GarbageCollect() override;

        size_type GetMaxContiguousAllocationSize() const override;
        size_type NumAllocatedBytes() const override;
        size_type Capacity() const override;
        IAllocatorAllocate* GetSubAllocator() override;

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

#endif // AZ_POOL_ALLOCATION_SCHEME_H
#pragma once


