/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <System/PhysXJob.h>
#include <AzCore/Debug/Profiler.h>

namespace PhysX
{
    PhysXJob::PhysXJob(physx::PxBaseTask& pxTask, AZ::JobContext* context)
        : AZ::Job(true, context)
        , m_pxTask(pxTask)
    {
    }

    void PhysXJob::Process()
    {
        AZ_PROFILE_SCOPE(Physics, m_pxTask.getName());
        m_pxTask.run();
        m_pxTask.release();
    }
}
