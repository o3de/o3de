/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZ_BEST_FIT_EXT_MAP_ALLOCATOR_H
#define AZ_BEST_FIT_EXT_MAP_ALLOCATOR_H

#include <AzCore/Memory/Memory.h>

namespace AZ
{
    class BestFitExternalMapSchema;
    /**
    * Best fit allocator is using external maps to store the memory tracking info.
    * External maps allows us to use this allocator with uncached memory, unaccessible memory and so on,
    * because the tracking node is stored outside the main chunk.
     */
    class BestFitExternalMapAllocator
        : public AllocatorBase
        , public IAllocatorAllocate
    {
    public:
        AZ_TYPE_INFO(BestFitExternalMapAllocator, "{36266C8B-9A2C-4E3E-9812-3DB260868A2B}")
        BestFitExternalMapAllocator();

        struct Descriptor
        {
            Descriptor()
                : m_memoryBlock(NULL)
                , m_memoryBlockByteSize(0)
                , m_mapAllocator(NULL)
                , m_allocationRecords(true)
                , m_stackRecordLevels(5) {}

            static const int        m_memoryBlockAlignment = 16;
            void*                   m_memoryBlock;          ///< Pointer to memory to allocate from. Can be uncached.
            unsigned int            m_memoryBlockByteSize;  ///< Sizes if the memory block.
            IAllocatorAllocate*     m_mapAllocator;         ///< Allocator for the free chunks map. If null the SystemAllocator will be used.

            bool                    m_allocationRecords;    ///< True if we want to track memory allocations, otherwise false.
            unsigned char           m_stackRecordLevels;    ///< If stack recording is enabled, how many stack levels to record.
        };

        bool Create(const Descriptor& desc);

        void Destroy() override;

        //////////////////////////////////////////////////////////////////////////
        // IAllocator
        AllocatorDebugConfig GetDebugConfig() override;

        //////////////////////////////////////////////////////////////////////////
        // IAllocatorAllocate
        pointer_type    Allocate(size_type byteSize, size_type alignment, int flags = 0, const char* name = 0, const char* fileName = 0, int lineNum = 0, unsigned int suppressStackRecord = 0) override;
        void            DeAllocate(pointer_type ptr, size_type byteSize = 0, size_type alignment = 0) override;
        size_type       Resize(pointer_type ptr, size_type newSize) override;
        pointer_type    ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment) override;
        size_type       AllocationSize(pointer_type ptr) override;

        size_type       NumAllocatedBytes() const override;
        size_type       Capacity() const override;
        size_type       GetMaxAllocationSize() const override;
        size_type       GetMaxContiguousAllocationSize() const override;
        IAllocatorAllocate*  GetSubAllocator() override;
        //////////////////////////////////////////////////////////////////////////

    protected:
        BestFitExternalMapAllocator(const BestFitExternalMapAllocator&);
        BestFitExternalMapAllocator& operator=(const BestFitExternalMapAllocator&);

        Descriptor m_desc;
        BestFitExternalMapSchema* m_schema;
    };
}

#endif // AZ_BEST_FIT_EXT_MAP_ALLOCATOR_H
#pragma once


