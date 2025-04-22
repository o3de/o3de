/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        AZ_CLASS_ALLOCATOR(JobCompletion, ThreadPoolAllocator);

        JobCompletion(JobContext* context = nullptr)
            : Job(false, context, true)
        {
        }

        /**
         * Call this function to start the job and block until the job has been completed.
         */
        void StartAndWaitForCompletion()
        {
            AZ_PROFILE_FUNCTION(AzCore);

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
