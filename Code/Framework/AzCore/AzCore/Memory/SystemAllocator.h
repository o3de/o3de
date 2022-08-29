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

        SystemAllocator() = default;
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
        void            GarbageCollect() override                 { m_subAllocator->GarbageCollect(); }

        size_type       NumAllocatedBytes() const override       { return m_subAllocator->NumAllocatedBytes(); }

        //////////////////////////////////////////////////////////////////////////

    protected:
        SystemAllocator(const SystemAllocator&);
        SystemAllocator& operator=(const SystemAllocator&);

        bool                        m_ownsOSAllocator = false;
        IAllocator* m_subAllocator;
    };
}



