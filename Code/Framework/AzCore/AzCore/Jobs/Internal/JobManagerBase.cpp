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

#include <AzCore/Debug/Profiler.h>
#include <AzCore/Jobs/Internal/JobManagerBase.h>
#include <AzCore/Jobs/Job.h>

using namespace AZ;
using namespace AZ::Internal;

void JobManagerBase::Process(Job* job)
{
#ifdef AZ_DEBUG_JOB_STATE
    AZ_Assert(job->m_state == Job::STATE_PENDING, "Job must be in pending state to be processed, current state %d", job->m_state);
    job->SetState(Job::STATE_PROCESSING);
#endif

    Job* dependent = job->GetDependent();
    bool isDelete = job->IsAutoDelete();

    AZ_PROFILE_INTERVAL_END(AZ::Debug::ProfileCategory::JobManagerDetailed, job);
    if (!job->IsCancelled())
    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzCore, "AZ::JobManagerBase::Process Job");
        job->Process();
    }

    if (isDelete)
    {
        delete job;
    }

    if (dependent)
    {
        dependent->DecrementDependentCount();
    }
}
