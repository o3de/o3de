/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZCORE_JOBS_TASK_GROUP_H
#define AZCORE_JOBS_TASK_GROUP_H 1

#include <AzCore/Jobs/JobEmpty.h>
#include <AzCore/Jobs/JobFunction.h>

namespace AZ
{
    /**
     * This is the new pseudo-standard interface for running jobs, can be found in VS2010 and Intel's Threading
     * Building Blocks. It is reasonably efficient, but it uses child jobs to implement the wait() function, which are
     * usually less efficient than explicit dependencies, and can have stack depth issues.
     */
    class structured_task_group
    {
    public:
        AZ_CLASS_ALLOCATOR(structured_task_group, ThreadPoolAllocator);

        /**
         * This default constructor obtains the parent JobContext from the global context. The parent context can
         * be also specified explicitly.
         */
        explicit structured_task_group()
            : m_parentContext(JobContext::GetParentContext())
            , m_cancelGroup(m_parentContext->GetCancelGroup())
            , m_context(m_parentContext->GetJobManager(), m_cancelGroup)
            , m_empty(false, m_parentContext)
        {
        }

        /**
         * Differs from the 'standard' in that it requires a context to be specified, we do not have a global default
         * context for jobs.
         */
        structured_task_group(JobContext* parentContext)
            : m_parentContext(parentContext ? parentContext : JobContext::GetParentContext())
            , m_cancelGroup(m_parentContext->GetCancelGroup())
            , m_context(m_parentContext->GetJobManager(), m_cancelGroup)
            , m_empty(false, m_parentContext)
        {
        }

        /**
         * Starts running the specified function asynchronously as a job. The function f should have void return type
         * and take no parameters.
         */
        template<typename F>
        void run(const F& f)
        {
            Job* job = CreateJobFunction(f, true, &m_context);
            job->SetDependent(&m_empty);
            job->Start();
        }

        /**
         * Starts running the specified function asynchronously as a job, and then waits until all the jobs on this
         * structured_task_group have completed. The function f should have void return type and take no parameters.
         * If this is called from within a job then the function will be executed immediately instead of scheduling
         * it.
         */
        template<typename F>
        void run_and_wait(const F& f)
        {
            Job* currentJob = m_context.GetJobManager().GetCurrentJob();
            if (currentJob)
            {
                f();
            }
            else
            {
                run(f);
            }
            wait();
        }

        /**
         * Waits until all the jobs started on this task_group have completed. Note that this does not automatically
         * include jobs which were launched by other jobs, unless the job which spawned them also waited for them to
         * complete.
         */
        void wait()
        {
            m_empty.StartAndWaitForCompletion();
        }

        /**
         * Cancels all the jobs which were started on this task_group.
         */
        void cancel()
        {
            m_cancelGroup.Cancel();
        }

        /**
         * Checks if this task_group has been cancelled.
         */
        bool is_canceling() const
        {
            return m_cancelGroup.IsCancelled();
        }

    private:
        //non-copyable
        structured_task_group(const structured_task_group&);
        structured_task_group& operator=(const structured_task_group&);

        JobContext* m_parentContext;

        JobCancelGroup m_cancelGroup;
        JobContext m_context;

        //an empty job is required because we do not know whether wait() will need to block in a user thread or
        //suspend in a worker thread
        JobEmpty m_empty;
    };

    /**
     * task_group not currently implemented, the available documentation is not entirely clear on the exact differences
     * between this and structured_task_group.
     */
    //class task_group
    //{
    //};
}

#endif
#pragma once
