/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SimpleSchemaAllocator.h>
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
        : public IAllocator
    {
    public:
        AZ_TYPE_INFO_WITH_NAME_DECL(PoolSchema);

        PoolSchema();
        ~PoolSchema();

        bool Create();
        bool Create(
            PoolSchema::size_type pageSize, PoolSchema::size_type minAllocationSize, PoolSchema::size_type maxAllocationSize);

        AllocateAddress allocate(size_type byteSize, size_type alignment) override;
        size_type deallocate(pointer ptr, size_type byteSize, size_type alignment) override;
        AllocateAddress reallocate(pointer ptr, size_type newSize, size_type newAlignment) override;
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
        : public IAllocator
    {
    public:
        // Functions for getting an instance of a ThreadPoolData when using thread local storage
        typedef ThreadPoolData* (* GetThreadPoolData)();
        typedef void(* SetThreadPoolData)(ThreadPoolData*);

        ThreadPoolSchema(GetThreadPoolData getThreadPoolData, SetThreadPoolData setThreadPoolData);
        ~ThreadPoolSchema();

        bool Create();
        bool Create(PoolSchema::size_type pageSize, PoolSchema::size_type minAllocationSize, PoolSchema::size_type maxAllocationSize);

        AllocateAddress allocate(size_type byteSize, size_type alignment) override;
        size_type deallocate(pointer ptr, size_type byteSize, size_type alignment) override;
        AllocateAddress reallocate(pointer ptr, size_type newSize, size_type newAlignment) override;
        size_type get_allocated_size(pointer ptr, align_type alignment) const override;
        /// Return unused memory to the OS. Don't call this too often because you will force unnecessary allocations.
        void GarbageCollect() override;

        size_type NumAllocatedBytes() const override;

    protected:
        ThreadPoolSchema(const ThreadPoolSchema&);
        ThreadPoolSchema& operator=(const ThreadPoolSchema&);

        GetThreadPoolData m_threadPoolGetter;
        SetThreadPoolData m_threadPoolSetter;
        class ThreadPoolSchemaImpl* m_impl;
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
        ThreadPoolSchemaHelper()
            : ThreadPoolSchema(&GetThreadPoolData, &SetThreadPoolData)
        {
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

namespace AZ::Internal
{
    template<class Allocator>
    class PoolAllocation;
    constexpr const char* PoolAllocatorHelperTemplateId = "{813b4b74-7381-4c62-b475-3f66efbcb615}";
    template<class Schema>
    class PoolAllocatorHelper;

    AZ_TYPE_INFO_TEMPLATE_WITH_NAME_DECL(PoolAllocatorHelper, AZ_TYPE_INFO_CLASS);

    /*!
    * Template you can use to create your own thread pool allocators, as you can't inherit from ThreadPoolAllocator.
    * This is the case because we use tread local storage and we need separate "static" instance for each allocator.
    */
    template<class Schema>
    class PoolAllocatorHelper
        : public SimpleSchemaAllocator<Schema, /* ProfileAllocations */ true, /* ReportOutOfMemory */ false>
    {
    public:
        using Base = SimpleSchemaAllocator<Schema, true, false>;
        using pointer = typename Base::pointer;
        using size_type = typename Base::size_type;
        using difference_type = typename Base::difference_type;

        AZ_RTTI_NO_TYPE_INFO_DECL();

        PoolAllocatorHelper()
        {
            static_cast<Schema*>(this->m_schema)->Create();
        }

        PoolAllocatorHelper(size_t pageSize, size_t minAllocationSize, size_t maxAllocationSize)
        {
            static_cast<Schema*>(this->m_schema)->Create(pageSize, minAllocationSize, maxAllocationSize);
        }

        ~PoolAllocatorHelper() override = default;

        //////////////////////////////////////////////////////////////////////////
        // IAllocator
        AllocateAddress reallocate(pointer ptr, size_type newSize, size_type newAlignment) override
        {
            (void)ptr;
            (void)newSize;
            (void)newAlignment;
            AZ_Assert(false, "Not supported!");
            return AllocateAddress{};
        }

        //////////////////////////////////////////////////////////////////////////

        PoolAllocatorHelper& operator=(const PoolAllocatorHelper&) = delete;
    };

    extern template class PoolAllocatorHelper<PoolSchema>;
}

namespace AZ
{
    // Extern the PoolAllocatorHelper<PoolSchema> AZ::AzTypeInfo template to
    // to reduce instantations
    extern template struct AzTypeInfo<Internal::PoolAllocatorHelper<PoolSchema>>;
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
        AZ_CLASS_ALLOCATOR(PoolAllocator, SystemAllocator);

        using Base = Internal::PoolAllocatorHelper<PoolSchema>;

        AZ_RTTI(PoolAllocator, "{D3DC61AF-0949-4BFA-87E0-62FA03A4C025}", Base);

        AllocatorDebugConfig GetDebugConfig() override;
    };

    template<class Allocator>
    using ThreadPoolBase = Internal::PoolAllocatorHelper<ThreadPoolSchemaHelper<Allocator> >;

    class ThreadPoolAllocator;
    namespace Internal
    {
        extern template class PoolAllocatorHelper<ThreadPoolSchemaHelper<ThreadPoolAllocator>>;
    }

    /*!
     * Thread safe pool allocator. If you want to create your own thread pool heap,
     * inherit from ThreadPoolBase, as we need unique static variable for allocator type.
     */
    class ThreadPoolAllocator final
        : public ThreadPoolBase<ThreadPoolAllocator>
    {
    public:
        AZ_CLASS_ALLOCATOR(ThreadPoolAllocator, SystemAllocator);

        using Base = ThreadPoolBase<ThreadPoolAllocator>;

        AZ_RTTI(ThreadPoolAllocator, "{05B4857F-CD06-4942-99FD-CA6A7BAE855A}", Base);

        AllocatorDebugConfig GetDebugConfig() override;
    };
} // namespace AZ
