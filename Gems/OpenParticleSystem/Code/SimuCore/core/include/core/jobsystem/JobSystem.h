/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <cassert>
#include <condition_variable>
#include <memory>
#include <algorithm>
#include <mutex>
#include <thread>
#include <functional>
#include <atomic>
#include <string>
#include <deque>
#include <vector>
#include "SpinLock.h"

namespace SimuCore {
    class JobSystem 
    {
    public:
        // Counter used for execution state, can be waited on
        struct Counter {
            std::atomic<uint32_t> counter { 0 };
        };

        struct JobInfo {
            uint32_t jobId;
            uint32_t groupId;
            uint32_t groupIndex;
            bool firstJobInGroup;
            bool lastJobInGroup;
            // stack memory shared within the current group  for serially jobs
            void *sharedmemory;
        };

        JobSystem(const JobSystem &) = delete;
        JobSystem(JobSystem &&) = delete;
        JobSystem &operator=(const JobSystem &) = delete;
        JobSystem &operator=(JobSystem &&) = delete;

        void Initialize(uint32_t threadCount = ~0u);
        void AddJob(Counter &ctx, const std::function<void(JobInfo)> &func);
        void ForkTask(Counter &ctx, uint32_t jobCount, uint32_t groupSize, const std::function<void(JobInfo)> &func,
            size_t memorySize = 0);
        uint32_t GetGroupCount(uint32_t jobCount, uint32_t groupSize) const;
        void WaitCounter(const Counter &ctx);
        bool IsBusy(const Counter &ctx);
        uint32_t GetThreadCount() const;

        static JobSystem &GetInstance()
        {
            static JobSystem instance = JobSystem();
            return instance;
        }

    private:
        struct WorkerState {
            std::condition_variable wakeCondition;
            std::atomic_bool alive { true };
            std::mutex wakeMutex;
        };

        struct Job {
            std::function<void(JobInfo)> func;
            Counter *counter;
            uint32_t groupId;
            uint32_t groupJobOffset;
            uint32_t groupJobEnd;
            size_t memorySize;
        };

        struct JobQueue {
            std::atomic_bool picking { false };
            std::deque<Job> jobQueue;
            SpinLock lck;

            inline bool Pop(Job &job)
            {
                std::scoped_lock lock(lck);
                if (jobQueue.empty()) {
                    picking.store(false);
                    return false;
                }
                job = std::move(jobQueue.front());
                jobQueue.pop_front();
                picking.store(true);
                return true;
            }

            inline void Push(const Job &job)
            {
                std::scoped_lock lock(lck);
                jobQueue.push_back(job);
            }
        };

        JobSystem() = default;
        ~JobSystem();
        void Run(uint32_t queueIndex);

        uint32_t numThreads = 0;
        std::vector<JobQueue> queues;
        std::shared_ptr<WorkerState> state = std::make_shared<WorkerState>();
        std::atomic<uint32_t> nextQueue { 0 };
        std::atomic<bool> initialized = false;
    };
}
