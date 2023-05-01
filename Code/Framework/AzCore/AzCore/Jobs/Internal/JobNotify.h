/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZCORE_JOBS_INTERNAL_JOBNOTIFY_H
#define AZCORE_JOBS_INTERNAL_JOBNOTIFY_H 1

#include <AzCore/Jobs/Job.h>

namespace AZ
{
    namespace Internal
    {
        /**
         * For internal use only, this allows us to set a flag to notify the JobManager that a job has been ran.
         * Usually this job will be a dependent of the job we want to know has completed. Flag must be stored external
         * to this job, to avoid auto-delete issues.
         */
        class JobNotify
            : public Job
        {
        public:
            AZ_CLASS_ALLOCATOR(JobNotify, ThreadPoolAllocator);

            JobNotify(AZStd::atomic<bool>* notifyFlag, JobContext* context = nullptr)
                : Job(false, context)
                , m_notifyFlag(notifyFlag)
            {
            }
        protected:
            void Process() override
            {
                m_notifyFlag->store(true, AZStd::memory_order_release);
            }

            AZStd::atomic<bool>* m_notifyFlag;
        };
    }
}

#endif
#pragma once
