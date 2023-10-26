/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/AllocatorBase.h>
#include <AzCore/Memory/AllocatorInstance.h>
#include <AzCore/Memory/Config.h>

namespace AZ
{
    // Schema which acts as a pass through to another allocator. This allows for allocators
    // which exist purely to categorize/track memory separately, piggy backing on the
    // structure of another allocator
    template <class ParentAllocator>
    class ChildAllocatorSchema
        : public AllocatorBase
    {
    public:
        AZ_RTTI((ChildAllocatorSchema, "{2A28BEF4-278A-4A98-AC7D-5C1D6D190A36}", ParentAllocator), AllocatorBase);

        using Parent = ParentAllocator;


        //! Invoke the AllocatorBase PostCreate function to register
        //! the child schema allocator with the AZ::Allocator Manager
        //! as well as to create the Allocation Records for the ChildAllocatorSchema
        ChildAllocatorSchema()
        {
            PostCreate();
        }

        //! @param enableProfiling determines whether the allocations should be recorded
        //! in an allocation records structure
        explicit ChildAllocatorSchema(bool enableProfiling)
            : AllocatorBase{ enableProfiling }
        {
            PostCreate();
        }

        //! @param enableProfiling determines whether the allocations should be recorded
        //! in an allocation records structure
        //! @param skipRegistration determines if this instance of the allocator
        //! should be registered with the allocator manager
        ChildAllocatorSchema(bool enableProfiling, bool skipRegistration)
            : AllocatorBase{ enableProfiling }
        {
            if (skipRegistration)
            {
                DisableRegistration();
            }
            PostCreate();
        }

        //! Invoke the AllocatorBase PreDestroy function to deregister
        //! the child schema allocator with the AZ::Allocator Manager
        ~ChildAllocatorSchema() override
        {
            PreDestroy();
        }

        //---------------------------------------------------------------------
        // IAllocator
        //---------------------------------------------------------------------
        pointer allocate(size_type byteSize, size_type alignment) override
        {
            pointer address = AZ::AllocatorInstance<Parent>::Get().allocate(byteSize, alignment);
            // Record the allocation as well in the ChildAllocatorSchema
            AZ_MEMORY_PROFILE(ProfileAllocation(address, byteSize, alignment, 1));
            return address;
        }

        void deallocate(pointer ptr, size_type byteSize = 0, size_type alignment = 0) override
        {
            // Record de-allocations in the ChildAllocatorSchema Allocations
            // before calling the parent allocator to make sure the allocation records
            // are up-to-date
            AZ_MEMORY_PROFILE(ProfileDeallocation(ptr, byteSize, alignment, nullptr));

            AZ::AllocatorInstance<Parent>::Get().deallocate(ptr, byteSize, alignment);
        }

        pointer reallocate(pointer ptr, size_type newSize, size_type newAlignment) override
        {
            pointer newAddress = AZ::AllocatorInstance<Parent>::Get().reallocate(ptr, newSize, newAlignment);
            // The reallocation might have clamped the newSize to be least the minimum allocation size
            // used by the parent schema. For example the HphaSchemaBase has a minimum allocation size of 8 bytes
            [[maybe_unused]] const size_type allocatedSize = get_allocated_size(newAddress, 1);
            AZ_MEMORY_PROFILE(ProfileReallocation(ptr, newAddress, allocatedSize, newAlignment));
            return newAddress;
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
