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

#include <PhysX_precompiled.h>
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

