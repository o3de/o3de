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
    //! Helper macro to define a child allocator with a custom name for the allocator
    //! It provides the following types
    //! "<_ALLOCATOR_CLASS>" for use with the AZ::AllocatorInstance template and the AZ_CLASS_ALLOCATOR macro
    //! "<_ALLOCATOR_CLASS>_for_std_t" for use with AZStd container types such as AZStd::vector, AZStd::basic_string, etc...
    //! Ex. AZ_CHILD_ALLOCATOR_WITH_NAME(FooAllocator, "Foo", "{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}", AZ::SystemAllocator)
    //! Would create an allocator type which piggybacks off of the parent allocator schema named the following
    //! `FooAllocator` and `FooAllocator_for_std_t`
    //! The `FooAllocator` can be supplied as AZ_CLASS_ALLOCATOR for a class
    //! Such as `class VoxelCoordinates { AZ_CLASS_ALLOCATOR(VoxelCoordinates, FooAllocator); };`
    //!
    //! The `FooAllocator_for_std_t` can be supplied as the allocator argument to AZStd container such as an AZStd::vector
    //! Such as `AZStd::vector<AZ::Vector4, FooAllocator_for_std_t> m_var;`
#define AZ_CHILD_ALLOCATOR_WITH_NAME(_ALLOCATOR_CLASS, _ALLOCATOR_NAME, _ALLOCATOR_UUID, _ALLOCATOR_BASE) \
    class _ALLOCATOR_CLASS \
        : public AZ::ChildAllocatorSchema<_ALLOCATOR_BASE> \
    { \
        friend class AZ::AllocatorInstance<_ALLOCATOR_CLASS>; \
    public: \
        AZ_TYPE_INFO_WITH_NAME_DECL(_ALLOCATOR_CLASS); \
        using Base = AZ::ChildAllocatorSchema<_ALLOCATOR_BASE>; \
        AZ_RTTI_NO_TYPE_INFO_DECL(); \
    };\
    \
    using AZ_JOIN(_ALLOCATOR_CLASS, _for_std_t) = AZ::AZStdAlloc<_ALLOCATOR_CLASS>; \
    AZ_TYPE_INFO_WITH_NAME_IMPL_INLINE(_ALLOCATOR_CLASS, _ALLOCATOR_NAME, _ALLOCATOR_UUID); \
    AZ_RTTI_NO_TYPE_INFO_IMPL_INLINE(_ALLOCATOR_CLASS, _ALLOCATOR_CLASS ::Base);

    // Base class for ChildAllocatorSchema template to allow
    // rtti-cast to common type that indicates the allocator is a child allocator
    // This classes exposes a method to get the parent allocator
    class ChildAllocatorSchemaBase
        : public AllocatorBase
    {
    public:
        AZ_RTTI(ChildAllocatorSchemaBase, "{AF5C2C64-EED4-4BF7-BBD9-3328A81BBC00}", AllocatorBase);
        using AllocatorBase::AllocatorBase;
        virtual IAllocator* GetParentAllocator() const = 0;
    };

    // Schema which acts as a pass through to another allocator. This allows for allocators
    // which exist purely to categorize/track memory separately, piggy backing on the
    // structure of another allocator
    template <class ParentAllocator>
    class ChildAllocatorSchema
        : public ChildAllocatorSchemaBase
    {
    public:
        AZ_RTTI((ChildAllocatorSchema, "{2A28BEF4-278A-4A98-AC7D-5C1D6D190A36}", ParentAllocator), ChildAllocatorSchemaBase);

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
            : ChildAllocatorSchemaBase{ enableProfiling }
        {
            PostCreate();
        }

        //! @param enableProfiling determines whether the allocations should be recorded
        //! in an allocation records structure
        //! @param skipRegistration determines if this instance of the allocator
        //! should be registered with the allocator manager
        ChildAllocatorSchema(bool enableProfiling, bool skipRegistration)
            : ChildAllocatorSchemaBase{ enableProfiling }
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
                R"(Child Allocator "%s": Total allocated bytes is less than zero with a value of %td. Was deallocate() invoked with an address )"
                "that is not associated with this allocator? This should never occur",
                GetName(), m_totalAllocatedBytes.load());
            return static_cast<size_type>(m_totalAllocatedBytes);
        }

        IAllocator* GetParentAllocator() const override
        {
            return &AZ::AllocatorInstance<Parent>::Get();
        }

    private:
        AZStd::atomic<ptrdiff_t> m_totalAllocatedBytes{};
    };
}
