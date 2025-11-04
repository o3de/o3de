/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/HeapAllocator.h>

#include <Atom/RHI/TileAllocator.h>
#include <Atom/RHI/PageTileAllocator.h>

namespace AZ
{
    namespace DX12
    {
        using TileAllocator = RHI::TileAllocator<HeapAllocatorTraits>;
        using HeapTiles = RHI::TileAllocator<HeapAllocatorTraits>::HeapTiles;
    }
}
