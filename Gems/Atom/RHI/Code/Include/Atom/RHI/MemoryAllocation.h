/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/MemoryEnums.h>

namespace AZ::RHI
{
    //! Represents a memory allocation of GPU memory. It contains a smart pointer to the Memory type.
    template <typename MemoryType>
    struct MemoryAllocation
    {
        MemoryAllocation(Ptr<MemoryType> memory, size_t offset, size_t size, size_t alignment);
        MemoryAllocation() = default;

        /// A ref-counting pointer to the base memory chunk.
        Ptr<MemoryType> m_memory;

        /// A byte offset into the memory chunk.
        size_t m_offset = 0;

        /// The size in bytes of the allocation.
        size_t m_size = 0;

        /// The alignment in bytes of the allocation.
        size_t m_alignment = 0;
    };

    template <typename MemoryType>
    MemoryAllocation<MemoryType>::MemoryAllocation(
        Ptr<MemoryType> memory,
        size_t offset,
        size_t size,
        size_t alignment)
        : m_memory(memory)
        , m_offset(offset)
        , m_size(size)
        , m_alignment(alignment)
    {           
    }
}
