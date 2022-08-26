/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <PxPhysicsAPI.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Memory/AllocatorWrappers.h>

namespace PhysX
{
    //! System allocator to be used for all PhysX gem persistent allocations.
    AZ_ALLOCATOR_DEFAULT_GLOBAL_WRAPPER(PhysXAllocator, AZ::SystemAllocator, "{C07BA28C-F6AF-4AFA-A45C-6747476DE07F}");

    //! Implementation of the PhysX memory allocation callback interface using Open 3D Engine allocator.
    class PxAzAllocatorCallback
        : public physx::PxAllocatorCallback
    {
        void* allocate(size_t size, const char* typeName, const char* filename, int line) override;
        void deallocate(void* ptr) override;
    };
}
