/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/MemorySubAllocator.h>
#include <Atom/RHI/MemoryLinearSubAllocator.h>
#include <Atom/RHI/FreeListAllocator.h>
#include <Atom/RHI/PoolAllocator.h>
#include <RHI/MemoryPageAllocator.h>

namespace AZ
{
    namespace Metal
    {
        using MemoryFreeListSubAllocatorTraits = RHI::MemorySubAllocatorTraits<
            Memory,
            MemoryPageAllocator,
            RHI::FreeListAllocator>;

        using MemoryFreeListSubAllocator = RHI::MemorySubAllocator<MemoryFreeListSubAllocatorTraits>;

        using MemoryPoolSubAllocatorTraits = RHI::MemorySubAllocatorTraits<
            Memory,
            MemoryPageAllocator,
            RHI::PoolAllocator>;

        //[GFX TODO][ATOM-5894] - Consider using this allocator for Argument Buffer/Argument Buffer's Constant buffer
        using MemoryPoolSubAllocator = RHI::MemorySubAllocator<MemoryPoolSubAllocatorTraits>;

        using MemoryLinearSubAllocatorTraits = RHI::MemoryLinearSubAllocatorTraits<
            Memory,
            MemoryPageAllocator>;

        //[GFX TODO][ATOM-5894] - Consider using this allocator for staging memory for effeciency
        using MemoryLinearSubAllocator = RHI::MemorySubAllocator<MemoryLinearSubAllocatorTraits>;
    }
}
