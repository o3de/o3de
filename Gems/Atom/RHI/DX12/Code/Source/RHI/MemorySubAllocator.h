/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
#include <RHI/MemoryView.h>

namespace AZ
{
    namespace DX12
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

        using MemoryPoolSubAllocator = RHI::MemorySubAllocator<MemoryPoolSubAllocatorTraits>;

        using MemoryLinearSubAllocatorTraits = RHI::MemoryLinearSubAllocatorTraits<
            Memory,
            MemoryPageAllocator>;

        using MemoryLinearSubAllocator = RHI::MemorySubAllocator<MemoryLinearSubAllocatorTraits>;
    }
}
