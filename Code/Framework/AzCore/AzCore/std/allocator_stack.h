/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZSTD_ALLOCATOR_STACK_H
#define AZSTD_ALLOCATOR_STACK_H

#include <AzCore/std/base.h>
#include <AzCore/std/typetraits/integral_constant.h>
#include <AzCore/std/typetraits/aligned_storage.h>
#include <AzCore/std/typetraits/alignment_of.h>

#define AZ_STACK_ALLOCATOR(_Variable, _Size)     AZStd::stack_allocator _Variable(alloca(_Size), _Size);

#define AZ_ALLOCA(_Size)    alloca(_Size)

namespace AZ
{
    namespace Internal
    {
        struct AllocatorDummy;
    }
}

namespace AZStd
{
    /**
     * Allocator that allocates memory from the stack (which you provide)
     * The memory chunk is grabbed on construction, you CAN'T
     * new this object. The memory will be alive only while the object
     * exist. DON'T USE this allocator unless you are SURE you know what you are
     * doing, it's dangerous and tricky to manage.
     * How to use:
     * void MyFunction()
     * {
     *     stack_allocator myAllocator(AZ_ALLOCA(size),size,"Name"); //
     *     ...
     *     // NO reference to any memory after myAllocator goes out of scope.
     * }
     */
    class stack_allocator
    {
        typedef stack_allocator this_type;
    public:
        using pointer = void*;
        using size_type = AZStd::size_t;
        using difference_type = AZStd::ptrdiff_t;

        AZ_FORCE_INLINE stack_allocator(void* data, size_t size, const char* name = "AZStd::stack_allocator")
            : m_name(name)
            , m_data(reinterpret_cast<char*>(data))
            , m_freeData(reinterpret_cast<char*>(data))
            , m_size(size)
        {}

        AZ_FORCE_INLINE const char*  get_name() const            { return m_name; }
        AZ_FORCE_INLINE void         set_name(const char* name)  { m_name = name; }
        constexpr size_type          max_size() const            { return m_size; }
        AZ_FORCE_INLINE size_type    get_allocated_size() const  { return m_freeData - m_data; }

        pointer allocate(size_type byteSize, size_type alignment, int flags = 0)
        {
            (void)flags;
            char* address = AZ::PointerAlignUp(m_freeData, alignment);
            m_freeData = address + byteSize;
            AZ_Assert(size_t(m_freeData - m_data) <= m_size, "AZStd::stack_allocator - run out of memory!");
            return address;
        }

        AZ_FORCE_INLINE void  deallocate(pointer ptr, size_type byteSize, size_type alignment)
        {
            // do nothing.
            (void)ptr;
            (void)byteSize;
            (void)alignment;
        }
        AZ_FORCE_INLINE size_type    resize(pointer ptr, size_type newSize)
        {
            (void)ptr;
            (void)newSize;
            return 0;
        }

        AZ_FORCE_INLINE void  reset()
        {
            m_freeData = m_data;
        }

    private:
        // Prevent heap allocation
        void*  operator new(std::size_t size, const AZ::Internal::AllocatorDummy*);
        void   operator delete(void* ptr, const AZ::Internal::AllocatorDummy*);
        void*  operator new[](std::size_t size, const AZ::Internal::AllocatorDummy*);
        void   operator delete[](void* ptr, const AZ::Internal::AllocatorDummy*);
        void* operator new      (size_t);
        void* operator new[]    (size_t);
        void   operator delete   (void*);
        void   operator delete[] (void*);
        // no copy either
        stack_allocator(const this_type& rhs);
        this_type& operator=(const this_type& rhs);

        const char*    m_name;
        char*           m_data;
        char*           m_freeData;
        size_t          m_size;
    };

    AZ_FORCE_INLINE bool operator==(const stack_allocator& a, const stack_allocator& b)
    {
        if (&a == &b)
        {
            return true;
        }
        return false;
    }

    AZ_FORCE_INLINE bool operator!=(const stack_allocator& a, const stack_allocator& b)
    {
        if (&a != &b)
        {
            return true;
        }
        return false;
    }
}

#endif // AZSTD_ALLOCATOR_STACK_H
#pragma once
