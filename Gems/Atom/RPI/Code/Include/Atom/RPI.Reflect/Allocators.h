/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Memory/ChildAllocatorSchema.h>

namespace AZ::RPI
{
    AZ_CHILD_ALLOCATOR_WITH_NAME(
        ImageMipChainAssetAllocator, "Atom::ImageMipChainAssetAllocator", "{E3F40786-8675-461A-BF96-70812B4CFAF5}", AZ::SystemAllocator);
    AZ_CHILD_ALLOCATOR_WITH_NAME(
        StreamingImageAssetAllocator, "Atom::StreamingImageAssetAllocator", "{FE311FD6-105B-484E-A603-C773BFF25BA6}", AZ::SystemAllocator);
    AZ_CHILD_ALLOCATOR_WITH_NAME(
        BufferAssetAllocator, "Atom::BufferAssetAllocator", "{3C9BE3B7-65C8-48E0-BF03-6BE228C1842F}", AZ::SystemAllocator);
} // namespace RPI
