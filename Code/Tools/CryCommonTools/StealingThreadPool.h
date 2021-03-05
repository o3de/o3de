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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_STEALINGTHREADPOOL_H
#define CRYINCLUDE_CRYCOMMONTOOLS_STEALINGTHREADPOOL_H
#pragma once


#include "ThreadUtils.h"
#include <AzCore/std/parallel/condition_variable.h>
#include <AzCore/std/parallel/atomic.h>
#include <BaseTypes.h>
#include <AzCore/Casting/numeric_cast.h>
#if AZ_TRAIT_OS_PLATFORM_APPLE
#include "AppleSpecific.h"
#endif

namespace ThreadUtils {
    class StealingWorker;
    class JobGroup;

    // Simple stealing thread pool
    class StealingThreadPool
    {
    public:
        explicit StealingThreadPool(int numThreads, bool enableTracing = false);
        ~StealingThreadPool();

        void Start();
        void WaitAllJobs();

        const std::vector<JobTraces>& Traces() const{ return m_threadTraces; }
        bool SaveTracesGraph(const char* filename);

        // Submits single independent job
        template<class T>
        void Submit(void(* jobFunc)(T*), T* data)
        {
            Submit(Job((JobFunc)jobFunc, data));
        }

        // Create a group of jobs. A group of jobs can be followed by one "finishing" job.
        // It is a way to express dependencies between jobs.
        template<class T>
        JobGroup* CreateJobGroup(void(* jobFunc)(T*), T* data)
        {
            return CreateJobGroup((JobFunc)jobFunc, (void*)data);
        }

        uint GetNumThreads() const { return aznumeric_cast<uint>(m_numThreads); }

    private:
        StealingWorker* FindBestVictim(int exceptFor) const;
        StealingWorker* FindWorstWorker() const;

        void Submit(const Job& job);
        void Submit(const Jobs& jobs);
        JobGroup* CreateJobGroup(JobFunc, void* data);

        size_t m_numThreads;
        typedef std::vector<class StealingWorker*> ThreadWorkers;
        ThreadWorkers m_workers;

        bool m_enableTracing;
        std::vector<JobTraces> m_threadTraces;

        AZStd::atomic_long m_numJobsWaitingForExecution;
        AZStd::atomic_long m_numJobs;
        AZStd::condition_variable m_jobsCV;
        AZStd::condition_variable m_jobFinishedCV;
        

        friend class JobGroup;
        friend class StealingWorker;
    };


    // JobGroup represents a group of jobs that can be followed by one "finishing"
    // job. This is a way to express dependencies between jobs.
    class JobGroup
    {
    public:
        template<class T>
        void Add(void(* jobFunc)(T*), T* data)
        {
            Add((JobFunc)jobFunc, data);
        }

        // Submits group to thread pool
        void Submit();
    private:
        struct GroupInfo
        {
            Job m_job;
            JobGroup* m_group;
        };
        typedef std::vector<GroupInfo> GroupInfos;

        JobGroup(StealingThreadPool* pool, JobFunc func, void* data);

        static void Process(JobGroup::GroupInfo* job);
        void Add(JobFunc func, void* data);

        volatile LONG m_numJobsRunning;
        StealingThreadPool* m_pool;
        GroupInfos m_infos;
        Job m_finishJob;
        bool m_submited;
        friend class StealingThreadPool;
    };
}

#endif // CRYINCLUDE_CRYCOMMONTOOLS_STEALINGTHREADPOOL_H
