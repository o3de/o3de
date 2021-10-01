/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Jobs/JobManager.h>

#include <AzCore/Jobs/Job.h>
#include <AzCore/Jobs/Internal/JobNotify.h>

#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/functional.h>

#include <AzCore/Debug/Profiler.h>

#ifdef JOBMANAGER_ENABLE_STATS
#   include <stdio.h>
#endif

using namespace AZ;
using namespace AZ::Internal;

bool CompareJobPriorities(AZ::s8 value, const Job* job)
{
    return value > job->GetPriority();
}

void WorkQueue::LocalInsert(Job* job)
{
    LockGuard lock(m_lock);
    const AZStd::deque<Job*>::const_iterator locationToinsert = AZStd::upper_bound(m_queue.begin(),
                                                                                   m_queue.end(),
                                                                                   job->GetPriority(),
                                                                                   CompareJobPriorities);
    m_queue.insert(locationToinsert, job);
}

Job* WorkQueue::LocalPopFront()
{
    LockGuard lock(m_lock);

    Job* result = nullptr;
    if (!m_queue.empty())
    {
        result = m_queue.front();
        m_queue.pop_front();
    }

    return result;
}

Job* WorkQueue::TryStealFront()
{
    AZStd::exponential_backoff backoff;
    for (unsigned attempCount = 0; attempCount < TryStealSpinAttemps; ++attempCount)
    {
        // Do a bounded spin with backoff to acquire the lock
        if (m_lock.try_lock())
        {
            Job* result = nullptr;
            if (!m_queue.empty())
            {
                result = m_queue.front();
                m_queue.pop_front();
            }

            m_lock.unlock();
            return result;
        }

        backoff.wait();
    }

    return nullptr;
}


AZ_THREAD_LOCAL JobManagerWorkStealing::ThreadInfo* JobManagerWorkStealing::m_currentThreadInfo = nullptr;

JobManagerWorkStealing::JobManagerWorkStealing(const JobManagerDesc& desc)
    : m_isAsynchronous(!desc.m_workerThreads.empty())
    , m_workerThreads(AZStd::move(CreateWorkerThreads(desc.m_workerThreads)))
{
    //allow workers to begin processing after they have all been created, needed to wait since they may access each others queues
    m_initSemaphore.release(static_cast<unsigned int>(desc.m_workerThreads.size()));
}

JobManagerWorkStealing::~JobManagerWorkStealing()
{
    //kill worker threads
    if (!m_workerThreads.empty())
    {
        m_quitRequested = true;

        for (ThreadInfo* thread : m_workerThreads)
        {
            thread->m_waitEvent.release();
            thread->m_thread.join();
        }
    }

    //cleanup all threads
    for (ThreadInfo* thread : m_threads)
    {
        delete thread;
    }
}

void JobManagerWorkStealing::AddPendingJob(Job* job)
{
    AZ_Assert(job->GetDependentCount() == 0, ("Job has a non-zero ready count, it should not be being added yet"));

    ThreadInfo* info = m_currentThreadInfo;
#ifndef AZ_MONOLITHIC_BUILD
    if (!info)
    {
        info = CrossModuleFindAndSetWorkerThreadInfo();
    }
#endif

    AZ_PROFILE_INTERVAL_START(JobManagerDetailed, job, "AzCore Job Queued Awaiting Execute");

    if (job->IsCompletion())
    {
        // This is a completion job.  Process it in place, as it only signals (no work).
        AZ::Job* currentJob = nullptr;
        if (info)
        {
            currentJob = info->m_currentJob;
            info->m_currentJob = job;
        }
        Process(job);
        if (info)
        {
            info->m_currentJob = currentJob;
#ifdef JOBMANAGER_ENABLE_STATS
            ++info->m_jobsDone;
#endif
        }
    }
    else if (info && info->m_isWorker && (info->m_owningManager == this))
    {
        //current thread is a worker, insert into the local queue based on the job's priority
        info->m_pendingJobs.LocalInsert(job);
#ifdef JOBMANAGER_ENABLE_STATS
        ++info->m_jobsForked;
#endif
        // if there are threads asleep wake one up
        ActivateWorker();
    }
    else
    {
        //current thread is not a worker thread, insert into the global queue based on the job's priority
        if (IsAsynchronous())
        {
            AZStd::lock_guard<GlobalQueueMutexType> lock(m_globalJobQueueMutex);
            const GlobalJobQueue::const_iterator locationToinsert = AZStd::upper_bound(m_globalJobQueue.begin(),
                                                                                       m_globalJobQueue.end(),
                                                                                       job->GetPriority(),
                                                                                       CompareJobPriorities);
            m_globalJobQueue.insert(locationToinsert, job);

            //checking/changing global queue empty state or worker availability must be done atomically while holding the global queue lock
            ActivateWorker();
        }
        else
        {
            {
                AZStd::lock_guard<GlobalQueueMutexType> lock(m_globalJobQueueMutex);
                const GlobalJobQueue::const_iterator locationToinsert = AZStd::upper_bound(m_globalJobQueue.begin(),
                                                                                           m_globalJobQueue.end(),
                                                                                           job->GetPriority(),
                                                                                           CompareJobPriorities);
                m_globalJobQueue.insert(locationToinsert, job);
            }

            //no workers, so must process the jobs right now
            if (!info)  //unless we're already processing
            {
                ProcessJobsSynchronous(GetCurrentOrCreateThreadInfo(), nullptr, nullptr);
            }
        }
    }
}

void JobManagerWorkStealing::SuspendJobUntilReady(Job* job)
{
    ThreadInfo* info = GetCurrentOrCreateThreadInfo();
    AZ_Assert(info->m_currentJob == job, ("Can't suspend a job which isn't currently running"));

    info->m_currentJob = nullptr; //clear current job

    if (IsAsynchronous())
    {
        ProcessJobsAssist(info, job, nullptr);
    }
    else
    {
        ProcessJobsSynchronous(info, job, nullptr);
    }

    info->m_currentJob = job; //restore current job
}

void JobManagerWorkStealing::StartJobAndAssistUntilComplete(Job* job)
{
    ThreadInfo* info = GetCurrentOrCreateThreadInfo();
    AZ_Assert(!m_currentThreadInfo, "This thread is already assisting, you should use regular child jobs instead");
    AZ_Assert(!info->m_isWorker, "Can't assist using a worker thread");
    AZ_Assert(!info->m_currentJob, "Can't assist when a job is already processing on this thread");

    //We require the notify job, because the other job may auto-delete when it is complete.
    AZStd::atomic<bool> notifyFlag(false);
    Internal::JobNotify notifyJob(&notifyFlag, job->GetContext());
    job->SetDependent(&notifyJob);
    notifyJob.Start();
    job->Start();

    //the processing functions will return when the empty job dependent count has reached 1
    if (IsAsynchronous())
    {
        ProcessJobsAssist(info, nullptr, &notifyFlag);
    }
    else
    {
        ProcessJobsSynchronous(info, nullptr, &notifyFlag);
    }

    AZ_Assert(!m_currentThreadInfo, "");
    AZ_Assert(notifyFlag.load(AZStd::memory_order_acquire), "");
}

void JobManagerWorkStealing::ClearStats()
{
#ifdef JOBMANAGER_ENABLE_STATS
    for (unsigned int i = 0; i < m_threads.size(); ++i)
    {
        ThreadInfo* info = m_threads[i];
        info->m_globalJobs = 0;
        info->m_jobsForked = 0;
        info->m_jobsDone = 0;
        info->m_jobsStolen = 0;
        info->m_jobTime = 0;
        info->m_stealTime = 0;
    }
#endif
}

void JobManagerWorkStealing::PrintStats()
{
#ifdef JOBMANAGER_ENABLE_STATS
    char str[256];
    printf("===================================================\n");
    printf("Job System Stats:\n");
    printf("Thread   Global jobs    Forks/dependents   Jobs done   Jobs stolen    Job time (ms)  Steal time (ms)  Total time (ms)\n");
    printf("------   -------------  -----------------  ----------  ------------   -------------  ---------------  ---------------\n");
    for (unsigned int i = 0; i < m_threads.size(); ++i)
    {
        ThreadInfo* info = m_threads[i];
        double jobTime = 1000.0f * static_cast<double>(info->m_jobTime) / AZStd::GetTimeTicksPerSecond();
        double stealTime = 1000.0f * static_cast<double>(info->m_stealTime) / AZStd::GetTimeTicksPerSecond();
        azsnprintf(str, AZ_ARRAY_SIZE(str),  " %d:        %5d          %5d           %5d         %5d            %3.2f           %3.2f         %3.2f\n",
            i, info->m_globalJobs, info->m_jobsForked, info->m_jobsDone, info->m_jobsStolen, jobTime, stealTime, jobTime + stealTime);
        printf(str);
    }
#endif
}


Job* JobManagerWorkStealing::GetCurrentJob() const
{
    const ThreadInfo* info = m_currentThreadInfo;
#ifndef AZ_MONOLITHIC_BUILD
    if (!info)
    {
        //we could be in a different module where m_currentThreadInfo has not been set yet (on a worker or user thread assisting with jobs)
        info = FindCurrentThreadInfo();
    }
#endif
    return info ? info->m_currentJob : nullptr;
}

AZ::u32 JobManagerWorkStealing::GetWorkerThreadId() const
{
    const ThreadInfo* info = m_currentThreadInfo;
#ifndef AZ_MONOLITHIC_BUILD
    if (!info)
    {
        info = CrossModuleFindAndSetWorkerThreadInfo();
    }
#endif
    return info ? info->m_workerId : JobManagerBase::InvalidWorkerThreadId;
}



void JobManagerWorkStealing::ProcessJobsWorker(ThreadInfo* info)
{
    //block until all workers are created, we don't want to steal from a thread that hasn't been created yet
    m_initSemaphore.acquire();

    //setup thread-local storage
    m_currentThreadInfo = info;

    ProcessJobsInternal(info, nullptr, nullptr);

    m_currentThreadInfo = nullptr;
}

void JobManagerWorkStealing::ProcessJobsAssist(ThreadInfo* info, Job* suspendedJob, AZStd::atomic<bool>* notifyFlag)
{
    ThreadInfo* oldInfo = m_currentThreadInfo;
    m_currentThreadInfo = info;

    ProcessJobsInternal(info, suspendedJob, notifyFlag);

    m_currentThreadInfo = oldInfo; //restore previous ThreadInfo, necessary as must be NULL when returning to user code to support multiple job contexts
}

void JobManagerWorkStealing::ProcessJobsInternal(ThreadInfo* info, Job* suspendedJob, AZStd::atomic<bool>* notifyFlag)
{
    AZ_Assert(IsAsynchronous(), "ProcessJobs is only to be used when we have worker threads (can be called on non-workers too though)");

    //get thread local job queue
    WorkQueue* pendingJobs = info->m_isWorker ? &info->m_pendingJobs : nullptr;
    unsigned int victim = ((m_workerThreads.size() > 1) && (m_workerThreads[0] == info)) ? 1 : 0;

    while (true)
    {
        //check if suspended job is ready, before we try to get a new job
        if ((suspendedJob && (suspendedJob->GetDependentCount() == 0)) ||
            (notifyFlag && notifyFlag->load(AZStd::memory_order_acquire)))
        {
            return;
        }

        //Try to get an initial job.
        Job* job = nullptr;
        {
            //go to sleep if the global queue is empty (but only if this thread is a worker and does not have a suspended job)
            if (info->m_isWorker && !suspendedJob)
            {
                if (m_quitRequested)
                {
                    return;
                }

                bool shouldSleep = false;
                {
                    //checking/changing global queue empty state or worker availability must be done atomically while holding the global queue lock
                    AZStd::lock_guard<GlobalQueueMutexType> lock(m_globalJobQueueMutex);
                    if (m_globalJobQueue.empty())
                    {
                        shouldSleep = true;

                        //going to sleep, increment the sleep counter.
                        const AZ::u32 priorAvailible = m_numAvailableWorkers.fetch_add(1, AZStd::memory_order_acq_rel);
                        (void)priorAvailible;
                        AZ_Assert(priorAvailible < m_workerThreads.size(), "invalid number of availible job workers");

                        const bool wasAvailable = info->m_isAvailable.exchange(true, AZStd::memory_order_acq_rel);
                        AZ_Verify(!wasAvailable, "available flag should have been false as we are processing jobs!");
                    }
                }

                if (shouldSleep)
                {
                    //no available work, so go to sleep (or we have already been signaled by another thread and will acquire the semaphore but not actually sleep)
                    info->m_waitEvent.acquire();
                    AZ_PROFILE_INTERVAL_END(JobManagerDetailed, info);

                    if (m_quitRequested)
                    {
                        return;
                    }
                }
            }

            //check if suspended job is ready, before we try to get a new job
            if ((suspendedJob && (suspendedJob->GetDependentCount() == 0)) ||
                (notifyFlag && notifyFlag->load(AZStd::memory_order_acquire)))
            {
                return;
            }

            {
                AZStd::lock_guard<GlobalQueueMutexType> lock(m_globalJobQueueMutex);
                if (!m_globalJobQueue.empty())
                {
                    job = m_globalJobQueue.front();
                    m_globalJobQueue.pop_front();
#ifdef JOBMANAGER_ENABLE_STATS
                    ++info->m_globalJobs;
#endif
                }
            }
        }

        if (!job && pendingJobs)
        {
            //nothing on the global queue, try to pop from the local queue
            job = pendingJobs->LocalPopFront();
        }

        bool isTerminated = false;
        while (!isTerminated)
        {
#ifdef JOBMANAGER_ENABLE_STATS
            AZStd::sys_time_t jobStartTime = AZStd::GetTimeNowTicks();
#endif
            //run current job and jobs from the local queue until it is empty
            while (job)
            {
                info->m_currentJob = job;
                Process(job);
                info->m_currentJob = nullptr;

                //...after calling Process we cannot use the job pointer again, the job has completed and may not exist anymore
#ifdef JOBMANAGER_ENABLE_STATS
                ++info->m_jobsDone;
#endif
                //check if our suspended job is ready, before we try running a new job
                if ((suspendedJob && (suspendedJob->GetDependentCount() == 0)) ||
                    (notifyFlag && notifyFlag->load(AZStd::memory_order_acquire)))
                {
                    return;
                }

                //pop a new job from the local queue
                if (pendingJobs)
                {
                    job = pendingJobs->LocalPopFront();
                    if (job)
                    {
                        // not necessary, just an optimization - wakeup sleeping threads, there's work to be done
                        ActivateWorker();
                    }
                }
                else
                {
                    job = nullptr;
                }
            }

#ifdef JOBMANAGER_ENABLE_STATS
            AZStd::sys_time_t jobEndTime = AZStd::GetTimeNowTicks();
            info->m_jobTime += jobEndTime - jobStartTime;
#endif
            if (m_workerThreads.size() < 2)
            {
                isTerminated = true;
            }
            else
            {
                //attempt to steal a job from another thread's queue
                AZ_PROFILE_SCOPE(AzCore, "JobManagerWorkStealing::ProcessJobsInternal:WorkStealing");

                unsigned int numStealAttempts = 0;
                const unsigned int maxStealAttempts = (unsigned int)m_workerThreads.size() * 3; //try every thread a few times before giving up
                while (!job)
                {
                    //check if our suspended job is ready, before we try stealing a new job
                    if ((suspendedJob && (suspendedJob->GetDependentCount() == 0)) ||
                        (notifyFlag && notifyFlag->load(AZStd::memory_order_acquire)))
                    {
                        return;
                    }

                    //select a victim thread, using the same victim as the previous successful steal if possible
                    WorkQueue* victimQueue = &m_workerThreads[victim]->m_pendingJobs;

                    //attempt the steal
                    job = victimQueue->TryStealFront();
                    if (job)
                    {
                        //success, continue with the stolen job
#ifdef JOBMANAGER_ENABLE_STATS
                        ++info->m_jobsStolen;
#endif
                        break;
                    }

                    ++numStealAttempts;
                    if (numStealAttempts > maxStealAttempts)
                    {
                        //Time to give up, it's likely all the local queues are empty. Note that this does not mean all the jobs
                        // are done, some jobs may be in progress, or we may have had terrible luck with our steals. There may be
                        // more jobs coming, another worker could create many new jobs right now. But the only way this thread
                        // will get a new job is from the global queue or by a steal, so we're going to sleep until a new job is
                        // queued.
                        // The important thing to note is that all jobs will be processed, even if this thread goes to sleep while
                        // jobs are pending.

                        isTerminated = true;
                        break;
                    }

                    //steal failed, choose a new victim for next time
                    victim = (victim + 1) % m_workerThreads.size();
                    if (m_workerThreads[victim] == info)
                    {
                        //don't steal from ourselves
                        victim = (victim + 1) % m_workerThreads.size();
                    }
                }
            }
#ifdef JOBMANAGER_ENABLE_STATS
            info->m_stealTime += AZStd::GetTimeNowTicks() - jobEndTime;
#endif
        }
    }
}

void JobManagerWorkStealing::ProcessJobsSynchronous(ThreadInfo* info, Job* suspendedJob, AZStd::atomic<bool>* notifyFlag)
{
    AZ_Assert(!IsAsynchronous(), "ProcessJobsSynchronous should not be used when we have worker threads");

    ThreadInfo* oldInfo = m_currentThreadInfo;
    m_currentThreadInfo = info;

    while (!m_globalJobQueue.empty())
    {
        Job* job = m_globalJobQueue.front();
        m_globalJobQueue.pop_front();

        info->m_currentJob = job;
        Process(job);
        info->m_currentJob = nullptr;

        //...after calling Process we cannot use the job pointer again, the job has completed and may not exist anymore
#ifdef JOBMANAGER_ENABLE_STATS
        ++info->m_jobsDone;
#endif

        if ((suspendedJob && (suspendedJob->GetDependentCount() == 0)) ||
            (notifyFlag && notifyFlag->load(AZStd::memory_order_acquire)))
        {
            return;
        }
    }

    m_currentThreadInfo = oldInfo; //restore previous ThreadInfo, necessary as must be NULL when returning to user code to support multiple job contexts
}

JobManagerWorkStealing::ThreadInfo* JobManagerWorkStealing::GetCurrentOrCreateThreadInfo()
{
    ThreadInfo* info = m_currentThreadInfo;
    if (!info)
    {
        info = FindCurrentThreadInfo();
        if (!info)
        {
            info = aznew ThreadInfo;
            info->m_threadId = AZStd::this_thread::get_id();

            AZStd::lock_guard<AZStd::mutex> lock(m_threadsMutex);
            m_threads.push_back(info);
        }
    }

    return info;
}

#ifndef AZ_MONOLITHIC_BUILD
JobManagerWorkStealing::ThreadInfo* JobManagerWorkStealing::CrossModuleFindAndSetWorkerThreadInfo() const
{
    AZ_Assert(!m_currentThreadInfo, "m_currentThreadInfo != nullptr, but this method should be used as a lookup in the TLS-miss case");

    ThreadInfo* info = nullptr;

    //no lock required, m_workerThreads is stable after construction
    const AZStd::thread::id thisId = AZStd::this_thread::get_id();
    for (ThreadInfo* currWorkerThreadInfo : m_workerThreads)
    {
        if (currWorkerThreadInfo->m_threadId == thisId)
        {
            info = currWorkerThreadInfo;
            m_currentThreadInfo = currWorkerThreadInfo; //worker threads only belong to one job context, so m_currentThreadInfo can always be set
            break;
        }
    }

    return info;
}
#endif

JobManagerWorkStealing::ThreadInfo* JobManagerWorkStealing::FindCurrentThreadInfo() const
{
    AZ_Assert(!m_currentThreadInfo, "m_currentThreadInfo != nullptr, but this method should be used as a lookup in the TLS-miss case");
    AZ_Assert(m_threads.size() >= m_workerThreads.size(), "m_threads must contain all workers first");

    ThreadInfo* info = nullptr;
#ifndef AZ_MONOLITHIC_BUILD
    info = CrossModuleFindAndSetWorkerThreadInfo();
    if (!info)
#endif
    {
        //look in user threads
        const AZStd::thread::id thisId = AZStd::this_thread::get_id();

        //start looking at the first non-worker
        const size_t firstNonWorkerIndex = m_workerThreads.size();

        AZStd::lock_guard<AZStd::mutex> lock(m_threadsMutex);
        for (size_t i = firstNonWorkerIndex; i < m_threads.size(); ++i)
        {
            AZ_Assert(!m_threads[i]->m_isWorker, "Worker thread in user thread range");
            if (m_threads[i]->m_threadId == thisId)
            {
                info = m_threads[i];
                break;
            }   
        }
    }

    return info;
}

JobManagerWorkStealing::ThreadList JobManagerWorkStealing::CreateWorkerThreads(const JobManagerDesc::DescList& workerDescList)
{
    ThreadList workerThreads(workerDescList.size());
    m_threads.reserve(workerDescList.size());

    for (unsigned int iThread = 0; iThread < workerDescList.size(); ++iThread)
    {
        const JobManagerThreadDesc& desc = workerDescList[iThread];

        ThreadInfo* info = aznew ThreadInfo;
        info->m_isWorker = true;
        info->m_owningManager = this;
        info->m_workerId = iThread;

        AZStd::thread_desc threadDesc;
        threadDesc.m_name = "AZ JobManager worker thread";
        threadDesc.m_cpuId = desc.m_cpuId;
        threadDesc.m_priority = desc.m_priority;
        if (desc.m_stackSize != 0)
        {
            threadDesc.m_stackSize = desc.m_stackSize;
        }

        info->m_thread = AZStd::thread(
            threadDesc,
            [this, info]()
            {
                this->ProcessJobsWorker(info);
            }
        );

        info->m_threadId = info->m_thread.get_id();

        workerThreads[iThread] = info;
        m_threads.push_back(info);
    }

    return workerThreads;
}

inline void JobManagerWorkStealing::ActivateWorker()
{
    // find an available worker thread (we do it brute force because the number of threads is small)
    while (m_numAvailableWorkers.load(AZStd::memory_order_acquire) > 0)
    {
        for (size_t i = 0; i < m_workerThreads.size(); ++i)
        {
            ThreadInfo* info = m_workerThreads[i];
            if (info->m_isAvailable.exchange(false, AZStd::memory_order_acq_rel) == true)
            {
                // decrement number of available workers
                m_numAvailableWorkers.fetch_sub(1, AZStd::memory_order_acq_rel);
                // resume the thread execution

                AZ_PROFILE_INTERVAL_START(JobManagerDetailed, info, "AzCore WakeJobThread %d", info->m_workerId);
                info->m_waitEvent.release();
                return;
            }
        }
    }
}

