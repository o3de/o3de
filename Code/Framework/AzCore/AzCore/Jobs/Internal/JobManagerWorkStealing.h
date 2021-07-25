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
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/parallel/atomic.h>
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
        #define AZ_USE_JOBQUEUE_LOCK
        // The Job Queue is a lock free 4-priority queue. Its basic operation is as follows:
        // Each priority level is associated with a different queue, corresponding to the maximum size of a uint16_t.
        // Each queue is implemented as a ring buffer, and a 64 bit atomic maintains the following state per queue:
        // - offset to the "head" of the ring, from where we acquire elements
        // - offset to the "tail" of the ring, which tracks where new elements should be enqueued
        // - offset to a tail reservation index, which is used to reserve a slot to enqueue elements
        class JobQueue final
        {
        public:
            // Preallocating upfront allows us to reserve slots to insert jobs without locks.
            // Each thread allocated by the job manager consumes ~2 MB.
            constexpr static uint16_t MaxQueueSize = 0xffff;
            constexpr static uint8_t PriorityLevelCount = 4;

            JobQueue() = default;
            JobQueue(const JobQueue&) = delete;
            JobQueue& operator=(const JobQueue&) = delete;

            void Enqueue(Job* job);
            Job* TryDequeue();

        private:
#if defined AZ_USE_JOBQUEUE_LOCK
            AZStd::mutex m_mutex;
            uint64_t m_status[PriorityLevelCount] = {};
#else

            // NOTE: Implementation expects
            // MSB                                                 LSB
            // +--------------------PAYLOAD--------------------------+
            // | unused : 2 | tail_reserve : 2 | tail : 2 | head : 2 |
            // +-----------------------------------------------------+
            AZStd::atomic<uint64_t> m_status[PriorityLevelCount] = {};
#endif

            Job* m_queues[PriorityLevelCount][MaxQueueSize] = {};
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

            void QueueUnbounded(Job* job, bool shouldActivateWorker) override;

            void SuspendJobUntilReady(Job* job);

            void StartJobAndAssistUntilComplete(Job* job);

            void ClearStats();
            void PrintStats();

            void CollectGarbage() {}

            Job* GetCurrentJob() const;

            AZ::u32 GetNumWorkerThreads() const { return static_cast<AZ::u32>(m_workerThreads.size()); }

            AZ::u32 GetWorkerThreadId() const;

            void ActivateWorker();

        private:
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
                AZStd::unique_ptr<JobQueue> m_pendingJobs;

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
            ThreadList CreateWorkerThreads(const JobManagerDesc::DescList& workerDescList);
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
