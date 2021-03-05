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
            AZ_CLASS_ALLOCATOR(JobNotify, ThreadPoolAllocator, 0)

            JobNotify(AZStd::atomic<bool>* notifyFlag, JobContext* context = nullptr)
                : Job(false, context)
                , m_notifyFlag(notifyFlag)
            {
            }
        protected:
            virtual void Process()
            {
                m_notifyFlag->store(true, AZStd::memory_order_release);
            }

            AZStd::atomic<bool>* m_notifyFlag;
        };
    }
}

#endif
#pragma once