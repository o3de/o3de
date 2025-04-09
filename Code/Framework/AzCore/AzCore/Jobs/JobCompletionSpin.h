/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZCORE_JOBS_JOBCOMPLETION_SPIN_H
#define AZCORE_JOBS_JOBCOMPLETION_SPIN_H 1

#include <AzCore/Jobs/Job.h>
#include <AzCore/std/parallel/spin_mutex.h>

namespace AZ
{
    /**
     * Job which allows caller to block until it is completed using spin lock.
     * This should be in special cases where the jobs are super short and semaphore
     * creation is too expensive.
     * IMPORTANT: Don't use JobCompletionSpin by default, use JobCompletion! This class
     * is provided only for special cases. You should always verify with profiling that
     * is it actually faster when you use it, as spins (mutex or jobs) introduce a whole
     * new set of problems.
     */
    class JobCompletionSpin
        : public Job
    {
    public:
        AZ_CLASS_ALLOCATOR(JobCompletionSpin, ThreadPoolAllocator);

        JobCompletionSpin(JobContext* context = nullptr)
            : Job(false, context, true)
            , m_mutex(true) //locked initially
        {
        }

        /**
         * Call this function to start the job and block until the job has been completed.
         */
        void StartAndWaitForCompletion()
        {
            Start();

            // Wait until we can lock it again.
            m_mutex.lock();
        }

        virtual void Reset(bool isClearDependent)
        {
            Job::Reset(isClearDependent);

            // make sure the mutex is locked
            m_mutex.try_lock();
        }

    protected:
        virtual void Process()
        {
            //unlock the mutex, allowing WaitForCompletion to lock it and continue. Note that this can be
            //called before this job has even been started.
            m_mutex.unlock();
        }

        AZStd::spin_mutex m_mutex;
    };
}

#endif
#pragma once
