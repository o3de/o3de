/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/base.h>
#include <AzCore/std/typetraits/integral_constant.h>
#include <AzCore/std/typetraits/aligned_storage.h>
#include <AzCore/std/typetraits/alignment_of.h>

#define AZ_STACK_ALLOCATOR(_Variable, _Size) ::AZStd::stack_allocator _Variable(alloca(_Size), _Size);

#define AZ_ALLOCA(_Size) alloca(_Size)

namespace AZStd
{
    /**
     * Allocator that allocates memory from the stack (which you provide)
     * The memory chunk is grabbed on construction, you CAN'T new this object.
     * The memory will be alive only while the object exist.
     * DON'T USE this allocator unless you are SURE you know what you are doing, it's dangerous and tricky to manage.
     *
     * How to use:
     * void MyFunction()
     * {
     *     stack_allocator myAllocator(AZ_ALLOCA(size),size,"Name"); //
     *     ...
     *     // NO reference to any memory after myAllocator goes out of scope.
     * }
     */
    struct stack_allocator
    {
        using pointer = void*;
        using size_type = size_t;
        using difference_type = ptrdiff_t;

        AZ_FORCE_INLINE stack_allocator(void* data, size_t size)
            : m_data(static_cast<byte*>(data))
            , m_freeData(static_cast<byte*>(data))
            , m_size(size)
        {}

        [[nodiscard]] constexpr size_type max_size() const
        {
            return m_size;
        }

        AZ_FORCE_INLINE size_type get_allocated_size() const
        {
            return m_freeData - m_data;
        }

        [[nodiscard]] pointer allocate(
            size_type byteSize,
            size_type alignment,
            [[maybe_unused]] int flags = 0)
        {
            byte* address = AZ::PointerAlignUp(m_freeData, alignment);
            m_freeData = address + byteSize;
            AZ_Assert(size_t(m_freeData - m_data) <= m_size, "AZStd::stack_allocator - run out of memory!");
            return address;
        }

        AZ_FORCE_INLINE void deallocate(
            [[maybe_unused]] pointer ptr,
            [[maybe_unused]] size_type byteSize,
            [[maybe_unused]] size_type alignment)
        {
            // do nothing.
        }

        AZ_FORCE_INLINE size_type resize(
            [[maybe_unused]] pointer ptr,
            [[maybe_unused]] size_type newSize)
        {
            return 0;
        }

        AZ_FORCE_INLINE void reset()
        {
            m_freeData = m_data;
        }

        // Prevent heap allocation
        void* operator new(std::size_t) = delete;
        void* operator new[](std::size_t) = delete;
        void operator delete(void*) = delete;
        void operator delete[](void*) = delete;

        stack_allocator(const stack_allocator&) = delete;
        stack_allocator& operator=(const stack_allocator&) = delete;

        friend AZ_FORCE_INLINE bool operator==(const stack_allocator& a, const stack_allocator& b)
        {
            return &a == &b;
        }

    private:
        byte* m_data;
        byte* m_freeData;
        size_t m_size;
    };
}
