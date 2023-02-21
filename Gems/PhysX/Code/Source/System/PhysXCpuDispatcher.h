/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        AZ_CLASS_ALLOCATOR(PhysXCpuDispatcher, PhysXAllocator);

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
