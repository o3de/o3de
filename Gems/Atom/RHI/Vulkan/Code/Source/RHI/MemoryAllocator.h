/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/MemoryPageAllocator.h>
#include <RHI/MemoryTypeAllocator.h>
#include <RHI/MemoryView.h>
#include <Atom/RHI/MemorySubAllocator.h>

namespace AZ
{
    namespace Vulkan
    {
        using MemoryFreeListSubAllocatorTraits = RHI::MemorySubAllocatorTraits<
            Memory,
            MemoryPageAllocator,
            RHI::FreeListAllocator>;

        using MemoryAllocator = MemoryTypeAllocator<RHI::MemorySubAllocator<MemoryFreeListSubAllocatorTraits>, MemoryView>;
    }
}
