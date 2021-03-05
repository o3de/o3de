/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

        virtual pointer_type    Allocate(size_type byteSize, size_type alignment, int flags, const char* name = 0, const char* fileName = 0, int lineNum = 0, unsigned int suppressStackRecord = 0);
        virtual void            DeAllocate(pointer_type ptr, size_type byteSize = 0, size_type alignment = 0);
        virtual pointer_type    ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment) { (void)ptr; (void)newSize; (void)newAlignment; return NULL; }
        virtual size_type       Resize(pointer_type ptr, size_type newSize)                  { (void)ptr; (void)newSize; return 0; }
        virtual size_type       AllocationSize(pointer_type ptr);

        virtual size_type       NumAllocatedBytes() const               { return m_used; }
        virtual size_type       Capacity() const                        { return m_capacity; }
        virtual size_type       GetMaxAllocationSize() const;
        virtual IAllocatorAllocate* GetSubAllocator()                   { return m_subAllocator; }
        virtual void GarbageCollect()                                   {}

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


