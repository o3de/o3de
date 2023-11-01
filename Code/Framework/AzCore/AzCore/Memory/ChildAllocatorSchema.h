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
        AllocateAddress allocate(size_type byteSize, size_type alignment) override
        {
            const AllocateAddress allocateAddress = AZ::AllocatorInstance<Parent>::Get().allocate(byteSize, alignment);
            m_totalAllocatedBytes += allocateAddress.GetAllocatedBytes();
            AZ_MEMORY_PROFILE(ProfileAllocation(allocateAddress.GetAddress(), byteSize, alignment, 1));
            return allocateAddress;
        }

        size_type deallocate(pointer ptr, size_type byteSize = 0, size_type alignment = 0) override
        {
            // Record de-allocations in the ChildAllocatorSchema Allocations
            // before calling the parent allocator to make sure the allocation records
            // are up-to-date
            AZ_MEMORY_PROFILE(ProfileDeallocation(ptr, byteSize, alignment, nullptr));

            const size_type bytesDeallocated = AZ::AllocatorInstance<Parent>::Get().deallocate(ptr, byteSize, alignment);
            m_totalAllocatedBytes -= bytesDeallocated;
            return bytesDeallocated;
        }

        AllocateAddress reallocate(pointer ptr, size_type newSize, size_type newAlignment) override
        {
            const size_type oldAllocatedSize = get_allocated_size(ptr, 1);
            AllocateAddress newAddress = AZ::AllocatorInstance<Parent>::Get().reallocate(ptr, newSize, newAlignment);
            // The reallocation might have clamped the newSize to be at least the minimum allocation size
            // used by the parent schema. For example the HphaSchemaBase has a minimum allocation size of 8 bytes
            AZ_MEMORY_PROFILE(ProfileReallocation(ptr, newAddress, newAddress.GetAllocatedBytes(), newAlignment));
            m_totalAllocatedBytes += newAddress.GetAllocatedBytes() - oldAllocatedSize;
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
            AZ_Assert(
                m_totalAllocatedBytes >= 0,
                "Total allocated bytes is less than zero with a value of %td. Was deallocate() invoked with an address "
                "that is not associated with this allocator. This should never occur",
                m_totalAllocatedBytes.load());
            return static_cast<size_type>(m_totalAllocatedBytes);
        }

    private:
        AZStd::atomic<ptrdiff_t> m_totalAllocatedBytes{};
    };
}
