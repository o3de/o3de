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

#include <platform.h>
#include "ThreadUtils.h"
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/bind/bind.h>
#include <AzCore/std/parallel/lock.h>
#include <CryAssert.h>

namespace ThreadUtils
{
    class SimpleWorker
    {
    public:
        SimpleWorker(SimpleThreadPool* pool, int index, bool trace)
            : m_pool(pool)
            , m_index(index)
            , m_trace(trace)
        {
        }

        void Start(int startTime)
        {
            m_lastStartTime = startTime;
            m_handle = AZStd::thread(AZStd::bind(SimpleWorker::ThreadFunc, (void*)this));
        }

        static unsigned int __stdcall ThreadFunc(void* param)
        {
            SimpleWorker* self = (SimpleWorker*)(param);
            self->Work();
            return 0;
        }

        void ExecuteJob(Job& job)
        {
            job.Run();
            if (m_trace)
            {
                int time = (int)GetTickCount();

                JobTrace trace;
                trace.m_job = job;
                trace.m_duration = time - m_lastStartTime;
                m_traces.push_back(trace);

                m_lastStartTime = time;
            }
        }

        void Work()
        {
            Job job;
            for (;; )
            {
                if (m_pool->GetJob(job, m_index))
                {
                    ExecuteJob(job);
                }
                else
                {
                    return;
                }
            }
        }

        // Called from main thread
        void Join(JobTraces& traces)
        {
            if(m_handle.joinable())
            {
                m_handle.join();
            }

            if (m_trace)
            {
                m_traces.swap(traces);
            }
        }

    private:
        SimpleThreadPool* m_pool;
        AZStd::thread m_handle;
        int m_index;
        bool m_trace;
        int m_lastStartTime;
        JobTraces m_traces;
        friend SimpleThreadPool;
    };

    // ---------------------------------------------------------------------------

    SimpleThreadPool::SimpleThreadPool(bool trace)
        : m_trace(trace)
        , m_started(false)
        , m_numProcessedJobs(0)
    {
    }

    SimpleThreadPool::~SimpleThreadPool()
    {
        WaitAllJobs();
    }

    void SimpleThreadPool::Start(int numThreads)
    {
        m_workers.resize(numThreads);
        for (int i = 0; i < numThreads; ++i)
        {
            m_workers[i] = new SimpleWorker(this, i, m_trace);
        }

        m_started = true;

        int startTime = (int)GetTickCount();
        for (int i = 0; i < numThreads; ++i)
        {
            m_workers[i]->Start(startTime);
        }
    }


    void SimpleThreadPool::WaitAllJobs()
    {
        size_t numThreads = m_workers.size();
        m_threadTraces.resize(numThreads);
        for (size_t i = 0; i < numThreads; ++i)
        {
            m_workers[i]->Join(m_threadTraces[i]);
        }

        for (size_t i = 0; i < numThreads; ++i)
        {
            delete m_workers[i];
        }
        m_workers.clear();

        m_started = false;
    }

    void SimpleThreadPool::Submit(const Job& job)
    {
        assert(!m_started);
        m_jobs.push_back(job);
    }

    bool SimpleThreadPool::GetJob(Job& job, [[maybe_unused]] int threadIndex)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_lockJobs);

        if (m_numProcessedJobs >= m_jobs.size())
        {
            return false;
        }

        job = m_jobs[m_numProcessedJobs];
        ++m_numProcessedJobs;
        return true;
    }
}
