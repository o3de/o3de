/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZCORE_JOBS_JOBEXECUTOR_H
#define AZCORE_JOBS_JOBEXECUTOR_H

#pragma once

#include <AzCore/Debug/Profiler.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/std/parallel/condition_variable.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ
{
    /**
    * Helper for porting legacy jobs that allows Starting and Waiting for multiple jobs asynchronously
    */
    class LegacyJobExecutor final
    {
    public:
        LegacyJobExecutor() = default;

        LegacyJobExecutor(const LegacyJobExecutor&) = delete;

        ~LegacyJobExecutor()
        {
            WaitForCompletion();
        }

        template <class Function>
        inline void StartJob(const Function& processFunction, JobContext* context = nullptr)
        {
            Job * job = aznew JobFunctionExecutorHelper<Function>(processFunction, *this, context);
            StartJobInternal(job);
        }

        // SetPostJob - This API exists to support backwards compatibility and is not a recommended pattern to be copied.
        //  Instead, create AZ::Jobs with appropriate dependencies on each other
        template <class Function>
        inline void SetPostJob(LegacyJobExecutor& postJobExecutor, const Function& processFunction, JobContext* context = nullptr)
        {
            AZStd::unique_ptr<JobExecutorHelper> postJob(aznew JobFunctionExecutorHelper<Function>(processFunction, postJobExecutor, context)); // Allocate outside the lock
            {
                LockGuard lockGuard(m_conditionLock);

                AZ_Assert(!m_postJob, "Post already set");
                AZ_Assert(!m_running, "LegacyJobExecutor::SetPostJob() must be called before starting any jobs");
                m_postJob = std::move(postJob);
                // Note: m_jobCount is not incremented until we push the post job
            }
        }

        inline void ClearPostJob()
        {
            LockGuard lockGuard(m_conditionLock);
            m_postJob.reset();
        }

        inline void Reset()
        {
            AZ_Assert(!IsRunning(), "LegacyJobExecutor::Reset() called while jobs in flight");
        }

        inline void WaitForCompletion()
        {
            AZStd::unique_lock<decltype(m_conditionLock)> uniqueLock(m_conditionLock);

            while (m_running)
            {
                AZ_PROFILE_FUNCTION(AzCore);
                m_completionCondition.wait(uniqueLock, [this] { return !this->m_running; });
            }
        }

        // Push a logical fence that will cause WaitForCompletion to wait until PopCompletionFence is called and all jobs are complete.  Analogue to the legacy API SJobState::SetStarted()
        // Note: this does NOT fence execution of jobs in relation to each other
        inline void PushCompletionFence()
        {
            IncJobCount();
        }

        // Pop a logical completion fence.  Analogue to the legacy API SJobState::SetStopped()
        inline void PopCompletionFence()
        {
            JobCompleteUpdate();
        }

        // Are there presently jobs in-flight (queued or running)?
        inline bool IsRunning()
        {
            return m_running;
        }

    private:
        void JobCompleteUpdate()
        {
            AZ_Assert(m_jobCount, "Invalid LegacyJobExecutor::m_jobCount.");
            if (--m_jobCount == 0) // note: m_jobCount is atomic, so only the last completing job will take the count to zero
            {
                JobExecutorHelper* postJob = nullptr;
                {
                    // All state transitions to and from running must be serialized through the condition lock
                    LockGuard lockGuard(m_conditionLock);

                    // Test count again as another job may have started before we got the lock
                    if (!m_jobCount)
                    {
                        m_running = false;
                        postJob = m_postJob.release();
                        m_completionCondition.notify_all();
                    }                    
                }

                // outside the lock (this pointer is no longer valid)...
                if (postJob)
                {
                    postJob->StartOnExecutor();
                }
            }
        }

        void StartJobInternal(Job * job)
        {
            IncJobCount();
            job->Start();
        }

        void IncJobCount()
        {
            if (m_jobCount++ == 0)
            {
                // All state transitions to and from running must be serialized through the condition lock (Even though m_running is atomic)
                LockGuard lockGuard(m_conditionLock);
                m_running = true;
            }
        }

        class JobExecutorHelper
        {
        public:
            virtual ~JobExecutorHelper() = default;
            virtual void StartOnExecutor() = 0;
        };

        /**
        * Private Job type that notifies the owning LegacyJobExecutor of completion
        */
        template<class Function>
        class JobFunctionExecutorHelper : public JobFunction<Function>, public JobExecutorHelper
        {
            using Base = JobFunction<Function>;
        public:
            AZ_CLASS_ALLOCATOR(JobFunctionExecutorHelper, ThreadPoolAllocator, 0)

                JobFunctionExecutorHelper(typename JobFunction<Function>::FunctionCRef processFunction, LegacyJobExecutor& executor, JobContext* context)
                : JobFunction<Function>(processFunction, true /* isAutoDelete */, context)
                , m_executor(executor)
            {
            }

            void StartOnExecutor() override
            {
                m_executor.StartJobInternal(this);
            }

            void Process() override
            {
                Base::Process();

                m_executor.JobCompleteUpdate();
            }

        private:
            LegacyJobExecutor& m_executor;
        };

        template<class Function>
        friend class JobFunctionExecutorHelper; // For JobCompleteUpdate, StartJobInternal

        using Lock = AZStd::mutex;
        using LockGuard = AZStd::lock_guard<Lock>;

        AZStd::condition_variable m_completionCondition;
        Lock m_conditionLock;
        AZStd::unique_ptr<JobExecutorHelper> m_postJob;

        AZStd::atomic_uint m_jobCount{0};
        AZStd::atomic_bool m_running{false};
    };
}

#endif
