/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZ_HPHA_ALLOCATION_SCHEME_ALLOCATOR_H
#define AZ_HPHA_ALLOCATION_SCHEME_ALLOCATOR_H

#include <AzCore/Memory/Memory.h>
#include <AzCore/std/typetraits/aligned_storage.h>

namespace AZ
{
    /**
    * Heap allocator schema, based on Dimitar Lazarov "High Performance Heap Allocator".
    */
    template<bool DebugAllocator = false>
    class HphaSchemaBase
        : public IAllocatorSchema
    {
    public:
        /**
        * Description - configure the heap allocator. By default
        * we will allocate system memory using system calls. You can
        * provide arena (memory block) with pre-allocated memory.
        */
        struct Descriptor
        {
            Descriptor()
                : m_fixedMemoryBlockAlignment(AZ_TRAIT_OS_DEFAULT_PAGE_SIZE)
                , m_pageSize(AZ_PAGE_SIZE)
                , m_poolPageSize(4*1024)
                , m_isPoolAllocations(true)
                , m_fixedMemoryBlockByteSize(0)
                , m_fixedMemoryBlock(nullptr)
                , m_subAllocator(nullptr)
                , m_systemChunkSize(0)
                , m_capacity(AZ_CORE_MAX_ALLOCATOR_SIZE)
            {}

            unsigned int            m_fixedMemoryBlockAlignment;
            unsigned int            m_pageSize;                             ///< Page allocation size must be 1024 bytes aligned.
            unsigned int            m_poolPageSize : 31;                    ///< Page size used to small memory allocations. Must be less or equal to m_pageSize and a multiple of it.
            unsigned int            m_isPoolAllocations : 1;                ///< True to allow allocations from pools, otherwise false.
            size_t                  m_fixedMemoryBlockByteSize;             ///< Memory block size, if 0 we use the OS memory allocation functions.
            void*                   m_fixedMemoryBlock;                     ///< Can be NULL if so the we will allocate memory from the subAllocator if m_memoryBlocksByteSize is != 0.
            IAllocatorSchema*       m_subAllocator;                         ///< Allocator that m_memoryBlocks memory was allocated from or should be allocated (if NULL).
            size_t                  m_systemChunkSize;                      ///< Size of chunk to request from the OS when more memory is needed (defaults to m_pageSize)
            size_t                  m_capacity;                             ///< Max size this allocator can grow to
        };


        HphaSchemaBase(const Descriptor& desc);
        virtual ~HphaSchemaBase();

        pointer_type    Allocate(size_type byteSize, size_type alignment, int flags = 0, const char* name = 0, const char* fileName = 0, int lineNum = 0, unsigned int suppressStackRecord = 0) override;
        void            DeAllocate(pointer_type ptr, size_type byteSize = 0, size_type alignment = 0) override;
        pointer_type    ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment) override;
        /// Resizes allocated memory block to the size possible and returns that size.
        size_type       Resize(pointer_type ptr, size_type newSize) override;
        size_type       AllocationSize(pointer_type ptr) override;

        size_type       NumAllocatedBytes() const override;
        size_type       Capacity() const override;
        size_type       GetMaxAllocationSize() const override;
        size_type       GetMaxContiguousAllocationSize() const override;
        size_type       GetUnAllocatedMemory(bool isPrint = false) const override;

        /// Return unused memory to the OS (if we don't use fixed block). Don't call this unless you really need free memory, it is slow.
        void            GarbageCollect() override;

        static size_t GetMemoryGuardSize();
        static size_t GetFreeLinkSize();

    private:
        // Forward declare HpAllocator class
        // It is a private class implemented in the cpp
        class HpAllocator;

        // this must be at least the max size of HpAllocator (defined in the cpp) + any platform compiler padding
        // A static assert inside of HphaSchema.cpp validates that this is the case
        // as of commit https://github.com/o3de/o3de/commit/92cd457c256e1ec91eeabe04b56d1d4c61f8b1af
        // When MULTITHREADED and USE_MUTEX_PER_BUCKET is defined
        // the largest sizeof for HpAllocator is 16640 on MacOS
        // On Windows the sizeof HpAllocator is 8384
        // Up this value to 18 KiB to be safe
        static constexpr size_t hpAllocatorStructureSize = 18 * 1024;

        Descriptor          m_desc;
        int                 m_pad;      // pad the Descriptor to avoid C4355
        size_type           m_capacity;                 ///< Capacity in bytes.
        HpAllocator*        m_allocator;
        AZStd::aligned_storage_t<hpAllocatorStructureSize, 16> m_hpAllocatorBuffer;    ///< Memory buffer for HpAllocator
        bool                m_ownMemoryBlock;
    };

    // Template is externed here and explicitly instantiated in the cpp file
    extern template class HphaSchemaBase<false>;
    extern template class HphaSchemaBase<true>;

    namespace Internal
    {
        // HphaSchema class defaults to disabling the allocator debug functionality
        constexpr bool HphaDebugAllocator = false;
    }

    class HphaSchema
        : public HphaSchemaBase<Internal::HphaDebugAllocator>
    {
    public:
        using HphaSchemaBase<Internal::HphaDebugAllocator>::HphaSchemaBase;
    };
} // namespace AZ

#endif // AZ_HPHA_ALLOCATION_SCHEME_ALLOCATOR_H
#pragma once
