/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Jobs/Job.h>
#include <task/PxTask.h>

namespace PhysX
{
    //! Handles PhysX tasks in the Open 3D Engine job scheduler.
    class PhysXJob
        : public AZ::Job
    {
    public:
        AZ_CLASS_ALLOCATOR(PhysXJob, AZ::ThreadPoolAllocator);

        PhysXJob(physx::PxBaseTask& pxTask, AZ::JobContext* context = nullptr);
        ~PhysXJob() = default;

    protected:
        void Process() override;

    private:
        physx::PxBaseTask& m_pxTask;
    };
}
