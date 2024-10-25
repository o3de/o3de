/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI.Reflect/Allocators.h>

namespace AZ::RPI
{
    // Allocator name,                      Display Name,                       Allocator type,                 UUID
#define RPI_ALLOCATORS \
    ((ImageMipChainAssetAllocator)  ("RPI::ImageMipChainAssetAllocator")    (RHI::SystemAllocatorBase)  ("{E3F40786-8675-461A-BF96-70812B4CFAF5}")) \
    ((StreamingImageAssetAllocator) ("RPI::StreamingImageAssetAllocator")   (RHI::SystemAllocatorBase)  ("{FE311FD6-105B-484E-A603-C773BFF25BA6}")) \
    ((BufferAssetAllocator)         ("RPI::BufferAssetAllocator")           (RHI::SystemAllocatorBase)  ("{3C9BE3B7-65C8-48E0-BF03-6BE228C1842F}")) \
    // if it exceeds 50, needs adjustments in AzCore/Preprocessor/Sequences.h

    // Here we create all the classes for all the items in the above table (Step 1)
    AZ_SEQ_FOR_EACH(RHI_ALLOCATOR_DECL, RPI_ALLOCATORS)

} // namespace RPI
