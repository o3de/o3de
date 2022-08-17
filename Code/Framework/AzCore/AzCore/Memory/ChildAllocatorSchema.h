/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

namespace AZ
{
    // Schema which acts as a pass through to another allocator. This allows for allocators
    // which exist purely to categorize/track memory separately, piggy backing on the
    // structure of another allocator
    template <class ParentAllocator>
    class ChildAllocatorSchema
        : public IAllocatorSchema
    {
    public:
        AZ_TYPE_INFO(ChildAllocatorSchema, "{2A28BEF4-278A-4A98-AC7D-5C1D6D190A36}")

        // No descriptor is necessary, as the parent allocator is expected to already
        // be created and configured
        struct Descriptor {
            AZ_TYPE_INFO(Descriptor, "{EEDE559F-A2B2-40AE-8536-88A81FC1F853}")
        };
        using Parent = ParentAllocator;

        ChildAllocatorSchema(const Descriptor&) {}

        //---------------------------------------------------------------------
        // IAllocatorSchema
        //---------------------------------------------------------------------
        pointer_type Allocate(size_type byteSize, size_type alignment, int flags = 0, const char* name = 0, const char* fileName = 0, int lineNum = 0, unsigned int suppressStackRecord = 0) override
        {
            return AZ::AllocatorInstance<Parent>::Get().Allocate(byteSize, alignment, flags, name, fileName, lineNum, suppressStackRecord);
        }

        void DeAllocate(pointer_type ptr, size_type byteSize = 0, size_type alignment = 0) override
        {
            AZ::AllocatorInstance<Parent>::Get().DeAllocate(ptr, byteSize, alignment);
        }

        size_type Resize(pointer_type ptr, size_type newSize) override
        {
            return AZ::AllocatorInstance<Parent>::Get().Resize(ptr, newSize);
        }

        pointer_type ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment) override
        {
            return AZ::AllocatorInstance<Parent>::Get().ReAllocate(ptr, newSize, newAlignment);
        }

        size_type AllocationSize(pointer_type ptr) override
        {
            return AZ::AllocatorInstance<Parent>::Get().AllocationSize(ptr);
        }

        void GarbageCollect() override
        {
            AZ::AllocatorInstance<Parent>::Get().GarbageCollect();
        }

        size_type NumAllocatedBytes() const override
        {
            return AZ::AllocatorInstance<Parent>::Get().NumAllocatedBytes();
        }

        size_type Capacity() const override
        {
            return AZ::AllocatorInstance<Parent>::Get().Capacity();
        }

        size_type GetMaxAllocationSize() const override
        {
            return AZ::AllocatorInstance<Parent>::Get().GetMaxAllocationSize();
        }

        size_type GetMaxContiguousAllocationSize() const override
        {
            return AZ::AllocatorInstance<Parent>::Get().GetMaxContiguousAllocationSize();
        }

        size_type               GetUnAllocatedMemory(bool isPrint = false) const override
        {
            return AZ::AllocatorInstance<Parent>::Get().GetUnAllocatedMemory(isPrint);
        }
    };
}
