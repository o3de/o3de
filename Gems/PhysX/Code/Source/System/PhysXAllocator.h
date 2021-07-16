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
#include <AzCore/RTTI/TypeInfo.h>

namespace PhysX
{
    //! System allocator to be used for all PhysX gem persistent allocations.
    class PhysXAllocator
        : public AZ::SystemAllocator
    {
        friend class AZ::AllocatorInstance<PhysXAllocator>;

    public:

        AZ_TYPE_INFO(PhysXAllocator, "{C07BA28C-F6AF-4AFA-A45C-6747476DE07F}");

        const char* GetName() const override { return "PhysX System Allocator"; }
        const char* GetDescription() const override { return "PhysX general memory allocator"; }
    };

    //! Implementation of the PhysX memory allocation callback interface using Open 3D Engine allocator.
    class PxAzAllocatorCallback
        : public physx::PxAllocatorCallback
    {
        void* allocate(size_t size, const char* typeName, const char* filename, int line) override;
        void deallocate(void* ptr) override;
    };
}
