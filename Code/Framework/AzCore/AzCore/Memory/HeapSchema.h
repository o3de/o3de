/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZ_HEAP_ALLOCATION_SCHEME_ALLOCATOR_H
#define AZ_HEAP_ALLOCATION_SCHEME_ALLOCATOR_H

#include <AzCore/Memory/Memory.h>

namespace AZ
{
    /**
     * Heap allocator scheme
     * Internally uses use dlmalloc or version of it (nedmalloc, ptmalloc3).
     */
    class HeapSchema
        : public IAllocatorAllocate
    {
    public:
        typedef void*       pointer_type;
        typedef size_t      size_type;
        typedef ptrdiff_t   difference_type;

        /**
         * Description - configure the heap allocator. By default
         * we will allocate system memory using system calls. You can
         * provide arenas (spaces) with pre-allocated memory, and use the
         * flag to specify which arena you want to allocate from.
         */
        struct Descriptor
        {
            Descriptor()
                : m_numMemoryBlocks(0)
                , m_isMultithreadAlloc(true)
            {}

            static const int        m_memoryBlockAlignment = 64 * 1024;
            static const int        m_maxNumBlocks = 5;
            int                     m_numMemoryBlocks;                          ///< Number of memory blocks to use.
            void*                   m_memoryBlocks[m_maxNumBlocks];             ///< Pointers to provided memory blocks or NULL if you want the system to allocate them for you with the System Allocator.
            size_t                  m_memoryBlocksByteSize[m_maxNumBlocks];     ///< Sizes of different memory blocks, if m_memoryBlock is 0 the block will be allocated for you with the System Allocator.
            bool                    m_isMultithreadAlloc;                       ///< Set to true to enable multi threading safe allocation.
        };

        HeapSchema(const Descriptor& desc);
        virtual ~HeapSchema();

        pointer_type    Allocate(size_type byteSize, size_type alignment, int flags, const char* name = 0, const char* fileName = 0, int lineNum = 0, unsigned int suppressStackRecord = 0) override;
        void            DeAllocate(pointer_type ptr, size_type byteSize = 0, size_type alignment = 0) override;
        pointer_type    ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment) override { (void)ptr; (void)newSize; (void)newAlignment; return NULL; }
        size_type       Resize(pointer_type ptr, size_type newSize) override                  { (void)ptr; (void)newSize; return 0; }
        size_type       AllocationSize(pointer_type ptr) override;

        size_type       NumAllocatedBytes() const override               { return m_used; }
        size_type       Capacity() const override                        { return m_capacity; }
        size_type       GetMaxAllocationSize() const override;
        size_type       GetMaxContiguousAllocationSize() const override;
        IAllocatorAllocate* GetSubAllocator() override                   { return m_subAllocator; }
        void GarbageCollect() override                                   {}

    private:
        AZ_FORCE_INLINE size_type ChunckSize(pointer_type ptr);

        void*           m_memSpaces[Descriptor::m_maxNumBlocks];
        Descriptor      m_desc;
        size_type       m_capacity;         ///< Capacity in bytes.
        size_type       m_used;             ///< Number of bytes in use.
        IAllocatorAllocate* m_subAllocator;
        bool            m_ownMemoryBlock[Descriptor::m_maxNumBlocks];
    };
}

#endif // AZ_HEAP_ALLOCATION_SCHEME_ALLOCATOR_H
#pragma once


