/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

    //! Implementation of the PhysX memory allocation callback interface using Lumberyard allocator.
    class PxAzAllocatorCallback
        : public physx::PxAllocatorCallback
    {
        void* allocate(size_t size, const char* typeName, const char* filename, int line) override;
        void deallocate(void* ptr) override;
    };
}
