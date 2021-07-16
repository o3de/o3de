/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/BufferMemoryPageAllocator.h>
#include <RHI/BufferMemoryView.h>
#include <RHI/MemoryTypeAllocator.h>
#include <Atom/RHI/MemorySubAllocator.h>


namespace AZ
{
    namespace Vulkan
    {
        using BufferMemoryFreeListSubAllocatorTraits = RHI::MemorySubAllocatorTraits<
            BufferMemory,
            BufferMemoryPageAllocator,
            RHI::FreeListAllocator>;

        using BufferMemoryAllocator = MemoryTypeAllocator<RHI::MemorySubAllocator<BufferMemoryFreeListSubAllocatorTraits>, BufferMemoryView>;
    }
}

