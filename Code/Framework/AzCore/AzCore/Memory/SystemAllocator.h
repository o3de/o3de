/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/Memory.h>

namespace AZ
{
    class HphaSchema;

    /**
     * System allocator
     * The system allocator is the base allocator for
     * AZ memory lib. It is a singleton (like all other allocators), but
     * must be initialized first and destroyed last. All other allocators
     * will use them for internal allocations. This doesn't mean all other allocators
     * will be sub allocators, because we might have different memory system on consoles.
     * But the allocator utility system will use the system allocator.
     */
    class SystemAllocator
        : public AllocatorBase
    {
    public:
        AZ_RTTI(SystemAllocator, "{607C9CDF-B81F-4C5F-B493-2AD9C49023B7}", AllocatorBase)

        SystemAllocator();
        ~SystemAllocator() override;

        /**
         * Description - configure the system allocator. By default
         * we will allocate system memory using system calls. You can
         * provide arenas (spaces) with pre-allocated memory, and use the
         * flag to specify which arena you want to allocate from.
         * You are also allowed to supply IAllocator, but if you do
         * so you will need to take care of all allocations, we will not use
         * the default HphaSchema.
         * \ref HphaSchema::Descriptor
         */
        struct Descriptor
        {
            Descriptor()
                : m_custom(0)
                , m_allocationRecords(true)
                , m_stackRecordLevels(5)
            {}
            IAllocator*         m_custom;   ///< You can provide our own allocation scheme. If NULL a HphaScheme will be used with the provided Descriptor.

            struct Heap
            {
                Heap()
                    : m_pageSize(m_defaultPageSize)
                    , m_poolPageSize(m_defaultPoolPageSize)
                    , m_isPoolAllocations(true)
                    , m_subAllocator(nullptr)
                    , m_systemChunkSize(0)
                {}
                static const int        m_defaultPageSize = AZ_TRAIT_OS_DEFAULT_PAGE_SIZE;
                static const int        m_defaultPoolPageSize = 4 * 1024;
                static const int        m_memoryBlockAlignment = m_defaultPageSize;
                unsigned int            m_pageSize;                                 ///< Page allocation size must be 1024 bytes aligned. (default m_defaultPageSize)
                unsigned int            m_poolPageSize;                             ///< Page size used to small memory allocations. Must be less or equal to m_pageSize and a multiple of it. (default m_defaultPoolPageSize)
                bool                    m_isPoolAllocations;                        ///< True (default) if we use pool for small allocations (< 256 bytes), otherwise false. IMPORTANT: Changing this to false will degrade performance!
                IAllocator*       m_subAllocator;                             ///< Allocator that m_memoryBlocks memory was allocated from or should be allocated (if NULL).
                size_t                  m_systemChunkSize;                          ///< Size of chunk to request from the OS when more memory is needed (defaults to m_pageSize)
            }                           m_heap;
            bool                        m_allocationRecords;    ///< True if we want to track memory allocations, otherwise false.
            unsigned char               m_stackRecordLevels;    ///< If stack recording is enabled, how many stack levels to record.
        };

        bool Create(const Descriptor& desc);

        void Destroy() override;

        //////////////////////////////////////////////////////////////////////////
        // IAllocator
        AllocatorDebugConfig GetDebugConfig() override;

        //////////////////////////////////////////////////////////////////////////
        // IAllocator

        pointer         allocate(size_type byteSize, size_type alignment) override;
        void            deallocate(pointer ptr, size_type byteSize = 0, size_type alignment = 0) override;
        pointer         reallocate(pointer ptr, size_type newSize, size_type newAlignment) override;
        size_type get_allocated_size(pointer ptr, size_type alignment) const override;
        void            GarbageCollect() override                 { GetSchema()->GarbageCollect(); }

        size_type       NumAllocatedBytes() const override       { return GetSchema()->NumAllocatedBytes(); }

        //////////////////////////////////////////////////////////////////////////

    protected:
        SystemAllocator(const SystemAllocator&);
        SystemAllocator& operator=(const SystemAllocator&);

        Descriptor                  m_desc;
        bool                        m_isCustom;
        bool                        m_ownsOSAllocator;
    };
}



