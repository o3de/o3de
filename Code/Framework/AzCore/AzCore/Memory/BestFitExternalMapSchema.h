/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZ_BEST_FIT_EXT_MAP_ALLOCATION_SCHEME_H
#define AZ_BEST_FIT_EXT_MAP_ALLOCATION_SCHEME_H

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Memory/Memory.h>

#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/unordered_map.h>

namespace AZ
{
    /**
     * Best fit allocation scheme using external map.
     * External map allows us to use this allocator with uncached memory,
     * because the tracking node is stored outside the main chunk.
     */
    class BestFitExternalMapSchema
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
                : m_memoryBlock(NULL)
                , m_memoryBlockByteSize(0)
                , m_mapAllocator(NULL)
            {}

            static const int        m_memoryBlockAlignment = 16;
            void*                   m_memoryBlock;              ///< Pointer to memory to allocate from. Can be uncached.
            unsigned int            m_memoryBlockByteSize;      ///< Sizes if the memory block.
            IAllocatorAllocate*     m_mapAllocator;             ///< Allocator for the free chunks map. If null the SystemAllocator will be used.
        };

        BestFitExternalMapSchema(const Descriptor& desc);

        pointer_type    Allocate(size_type byteSize, size_type alignment, int flags);
        void            DeAllocate(pointer_type ptr);
        size_type       AllocationSize(pointer_type ptr);

        AZ_FORCE_INLINE size_type           NumAllocatedBytes() const               { return m_used; }
        AZ_FORCE_INLINE size_type           Capacity() const                        { return m_desc.m_memoryBlockByteSize; }
        size_type                           GetMaxAllocationSize() const;
        size_type                           GetMaxContiguousAllocationSize() const;
        AZ_FORCE_INLINE IAllocatorAllocate* GetSubAllocator() const                 { return m_desc.m_mapAllocator; }

        /**
         * Since we don't consolidate chucnks at free time (too expensive) we will do it as we need or when we can't
         * allocate memory. This function is at least O(nlogn) where 'n' are the free chunks.
         */
        void                                GarbageCollect();

    private:
        AZ_FORCE_INLINE size_type ChunckSize(pointer_type ptr);

        Descriptor      m_desc;             ///<
        size_type       m_used;             ///< Number of bytes in use.
        typedef AZStd::multimap<size_t, char*, AZStd::less<size_t>, AZStdIAllocator> FreeMapType;
        typedef AZStd::unordered_map<char*, size_t, AZStd::hash<char*>, AZStd::equal_to<char*>, AZStdIAllocator> AllocMapType;
        FreeMapType         m_freeChunksMap;
        AllocMapType        m_allocChunksMap;
    };
}

#endif // AZ_BEST_FIT_EXT_MAP_ALLOCATION_SCHEME_H
#pragma once


