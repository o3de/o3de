/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <System/PhysXCpuDispatcher.h>
#include <System/PhysXJob.h>

namespace PhysX
{
    PhysXCpuDispatcher* PhysXCpuDispatcherCreate()
    {
        return aznew PhysXCpuDispatcher();
    }

    void PhysXCpuDispatcher::submitTask(physx::PxBaseTask& task)
    {
        auto azJob = aznew PhysXJob(task);
        azJob->Start();
    }

    physx::PxU32 PhysXCpuDispatcher::getWorkerCount() const
    {
        return AZ::JobContext::GetGlobalContext()->GetJobManager().GetNumWorkerThreads();
    }
} // namespace PhysX

