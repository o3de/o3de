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
#ifndef AZCORE_JOBS_JOBCOMPLETION_H
#define AZCORE_JOBS_JOBCOMPLETION_H 1

#include <AzCore/Debug/Profiler.h>
#include <AzCore/Jobs/Job.h>
#include <AzCore/std/parallel/semaphore.h>

namespace AZ
{
    /**
     * Job which allows caller to block until it is completed. Useful as the last job in a series.
     */
    class JobCompletion
        : public Job
    {
    public:
        AZ_CLASS_ALLOCATOR(JobCompletion, ThreadPoolAllocator, 0)

        JobCompletion(JobContext* context = nullptr)
            : Job(false, context, true)
        {
        }

        /**
         * Call this function to start the job and block until the job has been completed.
         */
        void StartAndWaitForCompletion()
        {
            AZ_PROFILE_FUNCTION_STALL(AZ::Debug::ProfileCategory::AzCore);

            // start the job
            Start();

            // Wait till the process finishes
            m_semaphore.acquire();
        }

        virtual void Reset(bool isClearDependent)
        {
            Job::Reset(isClearDependent);

            // make sure semaphore is set to 0 (if the job was started with start, nobody will acquire the released counter).
            while (m_semaphore.try_acquire_for(AZStd::chrono::microseconds(0)))
            {
            }
        }

    protected:
        virtual void Process()
        {
            //release the semaphore, allowing WaitForCompletion to acquire it and continue.
            m_semaphore.release();
        }

        AZStd::semaphore m_semaphore;
    };
}

#endif
#pragma once
