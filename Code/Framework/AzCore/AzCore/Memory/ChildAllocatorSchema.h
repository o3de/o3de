/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/IAllocator.h>
#include <AzCore/Memory/AllocatorInstance.h>

namespace AZ
{
    // Schema which acts as a pass through to another allocator. This allows for allocators
    // which exist purely to categorize/track memory separately, piggy backing on the
    // structure of another allocator
    template <class ParentAllocator>
    class ChildAllocatorSchema
        : public IAllocator
    {
    public:
        AZ_TYPE_INFO(ChildAllocatorSchema, "{2A28BEF4-278A-4A98-AC7D-5C1D6D190A36}")

        using Parent = ParentAllocator;

        ChildAllocatorSchema() = default;

        //---------------------------------------------------------------------
        // IAllocator
        //---------------------------------------------------------------------
        pointer allocate(size_type byteSize, size_type alignment) override
        {
            return AZ::AllocatorInstance<Parent>::Get().allocate(byteSize, alignment);
        }

        void deallocate(pointer ptr, size_type byteSize = 0, size_type alignment = 0) override
        {
            AZ::AllocatorInstance<Parent>::Get().deallocate(ptr, byteSize, alignment);
        }

        pointer reallocate(pointer ptr, size_type newSize, size_type newAlignment) override
        {
            return AZ::AllocatorInstance<Parent>::Get().reallocate(ptr, newSize, newAlignment);
        }

        size_type get_allocated_size(pointer ptr, align_type alignment) const override
        {
            return AZ::AllocatorInstance<Parent>::Get().get_allocated_size(ptr, alignment);
        }

        void GarbageCollect() override
        {
            AZ::AllocatorInstance<Parent>::Get().GarbageCollect();
        }

        size_type NumAllocatedBytes() const override
        {
            return AZ::AllocatorInstance<Parent>::Get().NumAllocatedBytes();
        }
    };
}
