/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "core/jobsystem/JobSystem.h"

namespace SimuCore {
    // Intialize function for the jobsystem
    void JobSystem::Initialize(uint32_t threadCount)
    {
        if (initialized) {
            return;
        }
        threadCount = std::max(1u, threadCount);
        uint32_t numCores = std::thread::hardware_concurrency();
        numThreads = std::min(threadCount, std::max(1u, numCores - 1));
        std::vector<JobQueue> temp(numThreads);
        queues.swap(temp);

        for (uint32_t i = 0; i < numThreads; ++i) {
            std::thread workerThread([this, i] {
                std::shared_ptr<WorkerState> workerState = state;
                while (workerState->alive.load(std::memory_order_relaxed)) {
                    Run(i);
                    std::unique_lock<std::mutex> lock(workerState->wakeMutex);
                    workerState->wakeCondition.wait(lock);
                }
            });
            workerThread.detach();
        }
        initialized = true;
    }

    // Execute job on the job queue.
    inline void JobSystem::Run(uint32_t queueIndex)
    {
        Job job;
        for (uint32_t i = 0; i < numThreads; ++i) {
            JobQueue &jobQueue = queues[queueIndex % numThreads];
            while (jobQueue.Pop(job)) {
                JobInfo args;
                args.groupId = job.groupId;
                if (job.memorySize > 0) {
                    thread_local static std::vector<uint8_t> sharedData {};
                    sharedData.reserve(job.memorySize);
                    args.sharedmemory = sharedData.data();
                } else {
                    args.sharedmemory = nullptr;
                }

                for (uint32_t id = job.groupJobOffset; id < job.groupJobEnd; ++id) {
                    args.jobId = id;
                    args.groupIndex = id - job.groupJobOffset;
                    args.firstJobInGroup = (id == job.groupJobOffset);
                    args.lastJobInGroup = (id == job.groupJobEnd - 1);
                    job.func(args);
                }

                (void)job.counter->counter.fetch_sub(1, std::memory_order_relaxed);
            }
            queueIndex++; // go to next queue
        }
    }

    // This is the api for add a job into the jobsystem queue
    void JobSystem::AddJob(Counter &ctx, const std::function<void(JobInfo)> &func)
    {
        // Context state is updated:
        (void)ctx.counter.fetch_add(1,std::memory_order_relaxed);

        Job job;
        job.counter = &ctx;
        job.groupId = 0;
        job.groupJobOffset = 0;
        job.groupJobEnd = 1;
        job.memorySize = 0;
        job.func = func;

        queues[nextQueue.fetch_add(1, std::memory_order_relaxed) % numThreads].Push(job);
        state->wakeCondition.notify_one();
    }

    bool JobSystem::IsBusy(const Counter &ctx)
    {
        // If counter greater than 0, means there is still has work to do
        return ctx.counter.load(std::memory_order_relaxed) > 0;
    }

    void JobSystem::WaitCounter(const Counter &ctx)
    {
        if (IsBusy(ctx)) {
            // pick up job on this thread
            Run(nextQueue.fetch_add(1, std::memory_order_relaxed) % numThreads);
            while (IsBusy(ctx)) {
                // job run on the other queue.
                std::this_thread::yield();
            }
        }
    }

    void JobSystem::ForkTask(Counter &ctx, uint32_t jobCount, uint32_t groupSize, const std::function<void(JobInfo)> &func,
        size_t memorySize)
    {
        if (groupSize == 0 || jobCount == 0) {
            return;
        }

        const uint32_t groupCount = GetGroupCount(jobCount, groupSize);
        (void)ctx.counter.fetch_add(groupCount, std::memory_order_relaxed);
        Job job;
        job.counter = &ctx;
        job.func = func;
        job.memorySize = memorySize;

        // split the task and add jobs
        for (uint32_t i = 0; i < groupCount; ++i) {
            job.groupId = i;
            job.groupJobOffset = i * groupSize;
            job.groupJobEnd = std::min(job.groupJobOffset + groupSize, jobCount);
            queues[nextQueue.fetch_add(1, std::memory_order_relaxed) % numThreads].Push(job);
        }
        state->wakeCondition.notify_all();
    }

    uint32_t JobSystem::GetThreadCount() const
    {
        return numThreads;
    }

    uint32_t JobSystem::GetGroupCount(uint32_t jobCount, uint32_t groupSize) const
    {
        return (jobCount + groupSize - 1) / groupSize;
    }

    JobSystem::~JobSystem()
    {
        state->alive.store(false);
        // wakes up the sleeping threads
        state->wakeCondition.notify_all();
        // wait all jobs finish:
        for (auto &item : queues) {
            while (item.picking.load()) {
                std::this_thread::yield();
            }
        }
    }
}
