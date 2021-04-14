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
#include <PxPhysicsAPI.h>
#include <System/PhysXAllocator.h>

namespace PhysX
{
    //! CPU dispatcher which directs tasks submitted by PhysX to the Open 3D Engine scheduling system.
    class PhysXCpuDispatcher
        : public physx::PxCpuDispatcher
    {
    public:
        AZ_CLASS_ALLOCATOR(PhysXCpuDispatcher, PhysXAllocator, 0);

        PhysXCpuDispatcher() = default;
        ~PhysXCpuDispatcher() = default;
        
    private:
        // PxCpuDispatcher implementation
        void submitTask(physx::PxBaseTask& task) override;
        physx::PxU32 getWorkerCount() const override;
    };

    //! Creates a CPU dispatcher which directs tasks submitted by PhysX to the Open 3D Engine scheduling system.
    PhysXCpuDispatcher* PhysXCpuDispatcherCreate();
} // namespace PhysX
