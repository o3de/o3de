/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/MemoryPageAllocator.h>

#include <Atom/RHI/TileAllocator.h>
#include <Atom/RHI/PageTileAllocator.h>

namespace AZ
{
    namespace Vulkan
    {
        using TileAllocator = RHI::TileAllocator<MemoryPageAllocatorTraits>;
        using HeapTiles = RHI::TileAllocator<MemoryPageAllocatorTraits>::HeapTiles;
    }
}
