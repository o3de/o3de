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

#include <AzCore/Jobs/Job.h>
#include <task/PxTask.h>

namespace PhysX
{
    //! Handles PhysX tasks in the Open 3D Engine job scheduler.
    class PhysXJob
        : public AZ::Job
    {
    public:
        AZ_CLASS_ALLOCATOR(PhysXJob, AZ::ThreadPoolAllocator, 0);

        PhysXJob(physx::PxBaseTask& pxTask, AZ::JobContext* context = nullptr);
        ~PhysXJob() = default;

    protected:
        void Process() override;

    private:
        physx::PxBaseTask& m_pxTask;
    };
}
