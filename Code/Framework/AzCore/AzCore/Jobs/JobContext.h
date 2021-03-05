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
#ifndef AZCORE_JOBS_JOBCONTEXT_H
#define AZCORE_JOBS_JOBCONTEXT_H 1

#include <AzCore/Jobs/JobCancelGroup.h>

namespace AZ
{
    class JobManager;

    /**
     * A job context stores information about the execution environment of jobs, a single context should be shared
     * between many jobs.
     */
    class JobContext
    {
    public:
        AZ_CLASS_ALLOCATOR(JobContext, ThreadPoolAllocator, 0)

        JobContext(JobManager& jobManager)
            : m_jobManager(jobManager)
            , m_cancelGroup(NULL) { }

        JobContext(JobManager& jobManager, JobCancelGroup& cancelGroup)
            : m_jobManager(jobManager)
            , m_cancelGroup(&cancelGroup) { }

        JobContext(const JobContext& rhs)
            : m_jobManager(rhs.m_jobManager)
            , m_cancelGroup(rhs.m_cancelGroup) { }

        JobContext& operator=(const JobContext&) = delete;

        JobManager& GetJobManager() const { return m_jobManager; }

        /**
         * Call this only before jobs using this context have been created, it is not threadsafe.
         */
        void SetCancelGroup(JobCancelGroup* cancelGroup) { m_cancelGroup = cancelGroup; }

        JobCancelGroup* GetCancelGroup() const { return m_cancelGroup; }

        /**
         * Sets the global job context, this is what will be used when creating a top-level job without specifying
         * the context explicitly.
         */
        static void SetGlobalContext(JobContext* context);
        static JobContext* GetGlobalContext();

        /**
         * Gets the context of the currently processing job, or the global context if this is a top-level job. Note
         * that this requires a global context to have been set, and only returns the current job from that context,
         * if there are other JobManagers they are not used.
         */
        static JobContext* GetParentContext();

    private:

        JobManager& m_jobManager;
        JobCancelGroup* m_cancelGroup;
    };
}

#endif
#pragma once
