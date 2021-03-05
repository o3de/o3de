/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
