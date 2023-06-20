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
    /**
     * Wrapper for a virtual interface of an allocator.
     *
     */
    class AllocatorPointerWrapper : public IAllocator
    {
        friend bool operator==(const AllocatorPointerWrapper&, const AllocatorPointerWrapper&);
        friend bool operator!=(const AllocatorPointerWrapper&, const AllocatorPointerWrapper&);

    public:
        AllocatorPointerWrapper(IAllocator* allocator = nullptr)
            : m_allocator(allocator)
        {
        }

        AllocatorPointerWrapper(IAllocator& allocator)
            : m_allocator(&allocator)
        {
        }
        ~AllocatorPointerWrapper() override = default;

        AllocatorPointerWrapper(const AllocatorPointerWrapper& aOther)
        {
            m_allocator = aOther.m_allocator;
        }

        pointer allocate(size_type byteSize, align_type alignment = 1) override
        {
            return m_allocator->allocate(byteSize, alignment);
        }

        void deallocate(pointer ptr, size_type byteSize = 0, align_type = 0) override
        {
            m_allocator->deallocate(ptr, byteSize);
        }

        pointer reallocate(pointer ptr, size_type newSize, align_type alignment = 1) override
        {
            return m_allocator->reallocate(ptr, newSize, alignment);
        }

        size_type get_allocated_size(pointer ptr, align_type alignment = 1) const override
        {
            return m_allocator->get_allocated_size(ptr, alignment);
        }

        AllocatorPointerWrapper& operator=(IAllocator* allocator)
        {
            m_allocator = allocator;
            return *this;
        }

        AllocatorPointerWrapper& operator=(IAllocator& allocator)
        {
            m_allocator = &allocator;
            return *this;
        }

        AllocatorPointerWrapper& operator=(const AllocatorPointerWrapper& allocator)
        {
            m_allocator = allocator.m_allocator;
            return *this;
        }
    private:
        IAllocator* m_allocator;
    };

    AZ_FORCE_INLINE bool operator==(const AllocatorPointerWrapper& a, const AllocatorPointerWrapper& b)
    {
        return a.m_allocator == b.m_allocator;
    }

    AZ_FORCE_INLINE bool operator!=(const AllocatorPointerWrapper& a, const AllocatorPointerWrapper& b)
    {
        return a.m_allocator != b.m_allocator;
    }

    /**
     * Allocator that uses a global instance for allocation instead of a specific instance.
     * This allocator can be copied/moved/etc and will always use the global instance.
     */
    template<typename Allocator>
    class AllocatorGlobalWrapper : public IAllocator
    {
    public:
        AllocatorGlobalWrapper() = default;
        AllocatorGlobalWrapper(const AllocatorGlobalWrapper&) {}
        AllocatorGlobalWrapper(AllocatorGlobalWrapper&&)  {}
        AllocatorGlobalWrapper& operator=(const AllocatorGlobalWrapper&) { return *this; }
        AllocatorGlobalWrapper& operator=(AllocatorGlobalWrapper&&) { return *this; }

        pointer allocate(size_type byteSize, align_type alignment = 1) override
        {
            return AllocatorInstance<Allocator>::Get().allocate(byteSize, alignment);
        }

        void deallocate(pointer ptr, size_type byteSize = 0, align_type alignment = 0) override
        {
            AllocatorInstance<Allocator>::Get().deallocate(ptr, byteSize, alignment);
        }

        pointer reallocate(pointer ptr, size_type newSize, align_type alignment = 1) override
        {
            return AllocatorInstance<Allocator>::Get().reallocate(ptr, newSize, alignment);
        }

        size_type get_allocated_size(pointer ptr, align_type alignment = 1) const override
        {
            return AllocatorInstance<Allocator>::Get().get_allocated_size(ptr, alignment);
        }

        void Create() {}
    };

    template<typename Allocator>
    AZ_FORCE_INLINE bool operator==(const AllocatorGlobalWrapper<Allocator>&, const AllocatorGlobalWrapper<Allocator>&)
    {
        return true;
    }

    template<typename Allocator>
    AZ_FORCE_INLINE bool operator!=(const AllocatorGlobalWrapper<Allocator>&, const AllocatorGlobalWrapper<Allocator>&)
    {
        return false;
    }

    AZ_TYPE_INFO_TEMPLATE(AllocatorGlobalWrapper, "{0994AE22-B98C-427B-A8EC-110F50D1ECC1}", AZ_TYPE_INFO_TYPENAME);
} // namespace AZ

#define AZ_ALLOCATOR_DEFAULT_GLOBAL_WRAPPER(AllocatorName, AllocatorGlobalType, AllocatorGUID)                                             \
    class AllocatorName : public AZ::AllocatorGlobalWrapper<AllocatorGlobalType>                                                           \
    {                                                                                                                                      \
    public:                                                                                                                                \
        AZ_RTTI(AllocatorName, AllocatorGUID, IAllocator)                                                                                  \
    };
