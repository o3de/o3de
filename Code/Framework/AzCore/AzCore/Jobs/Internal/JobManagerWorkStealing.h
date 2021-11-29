/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZCORE_JOBS_INTERNAL_JOBMANAGER_WORKSTEALING_H
#define AZCORE_JOBS_INTERNAL_JOBMANAGER_WORKSTEALING_H 1

// Included directly from JobManager.h

#include <AzCore/Jobs/Internal/JobManagerBase.h>
#include <AzCore/Jobs/JobManagerDesc.h>
#include <AzCore/Memory/PoolAllocator.h>

#include <AzCore/std/containers/queue.h>
#include <AzCore/std/parallel/shared_mutex.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/semaphore.h>
#include <AzCore/std/parallel/binary_semaphore.h>
#include <AzCore/std/parallel/thread.h>

namespace AZ
{
    class Job;

    namespace Internal
    {
        class WorkQueue final
        {
        public:
            void LocalInsert(Job *job);
            Job* LocalPopFront();
            Job* TryStealFront();

        private:
            enum
            {
                TryStealSpinAttemps = 16,
            };
            using LockType = AZStd::shared_mutex;
            using LockGuard = AZStd::lock_guard<LockType>;

            AZStd::deque<Job*> m_queue;
            LockType m_lock;
        };

        /**
         * Work stealing is in practice a very efficient way for processing fine grained jobs.
         * IMPORTANT: Because we want to put worker threads to sleep we do have extra locks and condition
         * variable in the code. In addition we are constantly kicking sleeping threads when we add jobs,
         * this is NOT efficient. Once we have heavier job loads (in practice) try to optimize and remove
         * those sticky points (if they are a problem). In addition we can think about writing a more advanced
         * scheduler or use some off the shelf.
         */
        class JobManagerWorkStealing final
            : public JobManagerBase
        {
        public:
            JobManagerWorkStealing(const JobManagerDesc& desc);
            ~JobManagerWorkStealing();

            AZ_FORCE_INLINE bool IsAsynchronous() const { return m_isAsynchronous; }

            void AddPendingJob(Job* job);

            void SuspendJobUntilReady(Job* job);

            void StartJobAndAssistUntilComplete(Job* job);

            void ClearStats();
            void PrintStats();

            void CollectGarbage() {}

            Job* GetCurrentJob() const;

            AZ::u32 GetNumWorkerThreads() const { return static_cast<AZ::u32>(m_workerThreads.size()); }

            AZ::u32 GetWorkerThreadId() const;

        private:

            void ActivateWorker();

            struct ThreadInfo
            {
                AZ_CLASS_ALLOCATOR(ThreadInfo, ThreadPoolAllocator, 0)

                AZStd::thread::id m_threadId;
                bool m_isWorker = false;
                Job* m_currentJob = nullptr; //job which is currently processing on this thread
                void* m_owningManager = nullptr; // pointer to the job manager that owns this thread. only used for comparisons, not to call functions.

                // valid only on workers (TODO: Use some lazy initialization as we don't need that data for non worker threads)
                AZStd::thread m_thread;
                AZStd::atomic_bool m_isAvailable{false};
                AZStd::binary_semaphore m_waitEvent;
                WorkQueue m_pendingJobs;
                unsigned int m_workerId = JobManagerBase::InvalidWorkerThreadId;

#ifdef JOBMANAGER_ENABLE_STATS
                unsigned int m_globalJobs = 0;
                unsigned int m_jobsForked = 0;
                unsigned int m_jobsDone = 0;
                unsigned int m_jobsStolen = 0;
                u64 m_jobTime = 0;
                u64 m_stealTime = 0;
#endif
            };
            using ThreadList = AZStd::vector<ThreadInfo*>;

            void ProcessJobsWorker(ThreadInfo* info);
            void ProcessJobsAssist(ThreadInfo* info, Job* suspendedJob, AZStd::atomic<bool>* notifyFlag);
            void ProcessJobsSynchronous(ThreadInfo* info, Job* suspendedJob, AZStd::atomic<bool>* notifyFlag);
            void ProcessJobsInternal(ThreadInfo* info, Job* suspendedJob, AZStd::atomic<bool>* notifyFlag);
            ThreadList CreateWorkerThreads(const JobManagerDesc& jmDesc);
#ifndef AZ_MONOLITHIC_BUILD
            ThreadInfo* CrossModuleFindAndSetWorkerThreadInfo() const;
#endif
            ThreadInfo* FindCurrentThreadInfo() const;
            ThreadInfo* GetCurrentOrCreateThreadInfo();

            bool m_isAsynchronous;

            ThreadList m_threads;
            mutable AZStd::mutex m_threadsMutex;

            AZStd::semaphore m_initSemaphore;

            const ThreadList m_workerThreads; //no mutex required for this list, it's only assigned during startup, must be declared after m_threads and m_initSemaphore

            using GlobalJobQueue = AZStd::deque<Job*>;
            using GlobalQueueMutexType = AZStd::mutex;

            GlobalJobQueue              m_globalJobQueue;
            GlobalQueueMutexType        m_globalJobQueueMutex;

            volatile bool               m_quitRequested = false;
            AZStd::atomic_uint          m_numAvailableWorkers{0};

            //thread-local pointer to the info for this thread. This is set for worker threads all the time,
            //and user threads only while they are processing jobs
            static AZ_THREAD_LOCAL ThreadInfo* m_currentThreadInfo;
        };
    }
}

#endif
#pragma once
