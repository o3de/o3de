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
#include "StealingThreadPool.h"
#include "ThreadUtils.h"
#include <AzCore/std/bind/bind.h>
#include <AzCore/std/parallel/thread.h>
#include <Cry_Math.h>

namespace ThreadUtils {
    class StealingWorker
    {
    public:
        StealingWorker(StealingThreadPool* pool, int index, bool trace, AZStd::condition_variable& jobsCV)
            : m_pool(pool)
            , m_index(index)
            , m_tracingEnabled(trace)
            , m_lastStartTime(0)
            , m_exitFlag(0)
            , m_jobsCV(jobsCV)
        {
        }

        static unsigned int __stdcall ThreadFunc(void* param)
        {
            StealingWorker* self = (StealingWorker*)(param);
            self->Work();
            return 0;
        }

        void Start(int startTime)
        {
            m_lastStartTime = startTime;
            
            string threadName;
            threadName.Format("StealingWorker %d", m_index);
            
            AZStd::thread_desc threadDesc;
            threadDesc.m_name = threadName.c_str();
            m_thread = AZStd::thread(AZStd::bind(StealingWorker::ThreadFunc, (void*)this), &threadDesc);
            
        }

        bool GetJobLockless(Job& job)
        {
            if (m_jobs.empty())
            {
                return false;
            }
            job = m_jobs.front();
            m_jobs.pop_front();

            return true;
        }

        bool GetJob(Job& job)
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_lockJobs);
            return GetJobLockless(job);
        }

        void ExecuteJob(Job& job)
        {
            --m_pool->m_numJobsWaitingForExecution;
            job.Run();

            if (m_tracingEnabled)
            {
                int time = (int)GetTickCount();

                JobTrace trace;
                trace.m_job = job;
                trace.m_duration = time - m_lastStartTime;
                m_traces.push_back(trace);

                m_lastStartTime = time;
            }

            --m_pool->m_numJobs;
            m_pool->m_jobFinishedCV.notify_all();
        }

        bool TryToStealJob(Job& job)
        {
            while (true)
            {
                StealingWorker* victim = m_pool->FindBestVictim(m_index);
                if (!victim)
                {
                    return false;
                }
                if (StealJobs(job, victim))
                {
                    return true;
                }
            }
        }

        void Work()
        {
            Job job;

            while (true)
            {
                AZStd::mutex loadMutex;
                AZStd::unique_lock<AZStd::mutex> loadLock(loadMutex, AZStd::defer_lock_t());

                while (m_pool->m_numJobsWaitingForExecution == 0)
                {
                    
                    m_jobsCV.wait(loadLock);
                    
                    if (m_exitFlag == 1)
                    {
                        return;
                    }
                }

                if (GetJob(job))
                {
                    ExecuteJob(job);
                }
                else if (TryToStealJob(job))
                {
                    ExecuteJob(job);
                }
            }
        }

        // Called from different worker thread
        bool StealJobs(Job& job, StealingWorker* victim)
        {
            if (victim == this)
            {
                assert(0 && "Trying to steal own jobs");
                return false;
            }

            bool order = m_index < victim->m_index;
            AZStd::lock_guard<AZStd::mutex> lock1(order ? m_lockJobs : victim->m_lockJobs);
            AZStd::lock_guard<AZStd::mutex> lock2(order ? victim->m_lockJobs : m_lockJobs);
            
            if (victim->m_jobs.empty())
            {
                return false;
            }

            int numJobs = (int)victim->m_jobs.size();
            size_t stealUntil = numJobs - numJobs / 2;
            Jobs::iterator begin = victim->m_jobs.begin();
            Jobs::iterator end = victim->m_jobs.begin() + stealUntil;

            m_jobs.insert(m_jobs.end(), begin, end);
            victim->m_jobs.erase(begin, end);

            return GetJobLockless(job);
        }

        // Called from any thread
        void Submit(const Job& job)
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_lockJobs);

            m_jobs.push_back(job);
            m_jobs.back().m_debugInitialThread = m_index;

            m_jobsCV.notify_one();
        }

        // Called from any thread
        void Submit(const Jobs& jobs)
        {
            const size_t numJobs = jobs.size();

            AZStd::lock_guard<AZStd::mutex> lock(m_lockJobs);

            m_jobs.insert(m_jobs.begin(), jobs.begin(), jobs.end());
            for (size_t i = 0; i < numJobs; ++i)
            {
                m_jobs[i].m_debugInitialThread = m_index;
            }

            m_jobsCV.notify_one();
        }

        long NumJobsPending() const
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_lockJobs);
            return m_jobs.size();
        }

        // Called from main thread
        void SignalExit()
        {
            CryInterlockedCompareExchange(&m_exitFlag, 1, 0);
        }

        void GetTraces(JobTraces& traces)
        {
            if (m_tracingEnabled)
            {
                m_traces.swap(traces);
            }
        }

    private:
        StealingThreadPool* m_pool;
        AZStd::thread m_thread;
        int m_index;
        bool m_tracingEnabled;
        int m_lastStartTime;
        JobTraces m_traces;

        Jobs m_jobs;
        mutable AZStd::mutex m_lockJobs;
        AZStd::condition_variable& m_jobsCV;
        
        LONG m_exitFlag;
        friend class StealingThreadPool;
    };

    // ---------------------------------------------------------------------------

    StealingThreadPool::StealingThreadPool(int numThreads, bool enableTracing)
        : m_numThreads(numThreads)
        , m_numJobs(0)
        , m_numJobsWaitingForExecution(0)
        , m_enableTracing(enableTracing)
    {
        m_workers.resize(numThreads);
        for (int i = 0; i < numThreads; ++i)
        {
            m_workers[i] = new StealingWorker(this, i, m_enableTracing, m_jobsCV);
        }
    }

    StealingThreadPool::~StealingThreadPool()
    {
        WaitAllJobs();

        size_t numThreads = m_workers.size();
        for (size_t i = 0; i < numThreads; ++i)
        {
            m_workers[i]->SignalExit();
        }
        m_jobsCV.notify_all();

        m_threadTraces.resize(numThreads);
        for (size_t i = 0; i < numThreads; ++i)
        {
            m_workers[i]->GetTraces(m_threadTraces[i]);
        }
    }

    void StealingThreadPool::Start()
    {
        int startTime = (int)GetTickCount();
        size_t numThreads = m_workers.size();
        for (int i = 0; i < numThreads; ++i)
        {
            m_workers[i]->Start(startTime);
        }
    }

    void StealingThreadPool::WaitAllJobs()
    {
        AZStd::mutex loadMutex;
        AZStd::unique_lock<AZStd::mutex> loadLock(loadMutex, AZStd::defer_lock_t());

        while (m_numJobs > 0)
        {
            m_jobsCV.wait(loadLock);
        }
    }

    // Called from any thread
    void StealingThreadPool::Submit(const Job& job)
    {
        ++m_numJobs;
        ++m_numJobsWaitingForExecution;
        
        if (StealingWorker* worker = FindWorstWorker())
        {
            worker->Submit(job);
        }
    }

    // Called from any thread
    void StealingThreadPool::Submit(const Jobs& jobs)
    {
        m_numJobs += jobs.size();
        m_numJobsWaitingForExecution += jobs.size();
        if (StealingWorker* worker = FindWorstWorker())
        {
            worker->Submit(jobs);
        }
    }

    JobGroup* StealingThreadPool::CreateJobGroup(JobFunc func, void* data)
    {
        return new JobGroup(this, func, data);
    }

    StealingWorker* StealingThreadPool::FindBestVictim(int exceptFor) const
    {
        int maxJobs = 0;
        StealingWorker* bestVictim = 0;
        for (size_t i = 0; i < m_workers.size(); ++i)
        {
            if (i == exceptFor)
            {
                continue;
            }
            StealingWorker* worker = m_workers[i];
            long numJobs = worker->NumJobsPending();
            if (numJobs > maxJobs)
            {
                maxJobs = numJobs;
                bestVictim = worker;
            }
        }
        return bestVictim;
    }

    StealingWorker* StealingThreadPool::FindWorstWorker() const
    {
        if (m_workers.empty())
        {
            return 0;
        }

        int minJobs = INT_MAX;
        StealingWorker* worstWorker = m_workers[0];
        for (size_t i = 0; i < m_workers.size(); ++i)
        {
            StealingWorker* worker = m_workers[i];
            long numJobs = worker->NumJobsPending();
            if (numJobs < minJobs)
            {
                minJobs = numJobs;
                worstWorker = worker;
            }
        }
        return worstWorker;
    }

    static bool WriteString(FILE* f, const char* str)
    {
        return fwrite(str, strlen(str), 1, f) == 1;
    }

    static int Interpolate(int a, int b, float phase)
    {
        return int(float(a) + float(b - a) * phase);
    }

    static int InterpolateColor(int c1, int c2, float phase)
    {
        const int r1 = (c1 & 0x0000ff);
        const int g1 = (c1 & 0x00ff00) >> 8;
        const int b1 = (c1 & 0xff0000) >> 16;
        const int r2 = (c2 & 0x0000ff);
        const int g2 = (c2 & 0x00ff00) >> 8;
        const int b2 = (c2 & 0xff0000) >> 16;

        const int r = min(255, max(0, Interpolate(r1, r2, phase)));
        const int g = min(255, max(0, Interpolate(g1, g2, phase)));
        const int b = min(255, max(0, Interpolate(b1, b2, phase)));

        return r + (g << 8) + (b << 16);
    }

    static const int g_animColors[] = {
        0xff0000, 0x0000ff, 0x00ff00,
        0xffff00, 0xff00ff, 0x00ffff,
        0xff8080, 0x8080ff, 0x80ff80,
        0xffff80, 0xff80ff, 0x80ffff
    };

    static int ColorizeJobTrace(const ThreadUtils::JobTrace& trace)
    {
        const int numColors = sizeof(g_animColors) / sizeof(g_animColors[0]);
        const int initialThread = trace.m_job.m_debugInitialThread;
        const int index = initialThread % numColors;
        const float brightness = aznumeric_cast<float>(pow(0.5f, initialThread / numColors));
        return InterpolateColor(0, InterpolateColor(g_animColors[index], 0xffffff, 0.5f), brightness);
    }

    bool StealingThreadPool::SaveTracesGraph(const char* filename)
    {
        if (!m_enableTracing)
        {
            return false;
        }

        const float screenWidth = 1240.0f;

        float duration = 0;
        for (size_t t = 0; t < m_threadTraces.size(); ++t)
        {
            float threadDuration = 0;
            const JobTraces& traces = m_threadTraces[t];
            for (int i = 0; i < traces.size(); ++i)
            {
                threadDuration += traces[i].m_duration;
            }
            duration = max(threadDuration, duration);
        }

        const float padding = 10.0f;
        const float rowHeight = 60.0f;
        const float xScale = fabsf(duration) > FLT_EPSILON ? (screenWidth - padding * 2.0f) / duration : 1.0f;

        const float width = screenWidth;
        const float height = (m_threadTraces.size() + 0.5f) * rowHeight;

        FILE* f = nullptr; 
        azfopen(&f, filename, "wt");
        if (!f)
        {
            return false;
        }

        char buf[4096];
        azsnprintf(buf, sizeof(buf),
            "<?xml version='1.0' encoding='UTF-8' standalone='no'?>\n"
            "<svg\n"
            "   xmlns:dc='http://purl.org/dc/elements/1.1/'\n"
            "   xmlns:cc='http://creativecommons.org/ns#'\n"
            "   xmlns:rdf='http://www.w3.org/1999/02/22-rdf-syntax-ns#'\n"
            "   xmlns:svg='http://www.w3.org/2000/svg'\n"
            "   xmlns='http://www.w3.org/2000/svg'\n"
            "   xmlns:sodipodi='http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd'\n"
            "   xmlns:inkscape='http://www.inkscape.org/namespaces/inkscape'\n"
            "   width='%f'\n"
            "   height='%f'\n"
            "   id='svg2'\n"
            "   version='1.1'\n"
            "     >\n",
            width, height
            );

        if (!WriteString(f, buf))
        {
            return false;
        }

        for (size_t t = 0; t < m_threadTraces.size(); ++t)
        {
            float x = padding;
            float y = rowHeight * 0.5f + rowHeight * t;

            azsnprintf(buf, sizeof(buf),
                "    <text\n"
                "       xml:space='preserve'\n"
                "       style='font-size:40px;font-style:normal;font-weight:normal;line-height:125%%;letter-spacing:0px;word-spacing:0px;fill:#000000;fill-opacity:1;stroke:none;font-family:Sans'\n"
                "       x='%f'\n"
                "       y='%f'\n"
                "       sodipodi:linespacing='125%%'><tspan sodipodi:role='line' x='%f' y='%f' style='font-size:12px;fill:#000000'>Thread %i</tspan></text>\n",
                x, y, x, y, static_cast<int>(t + 1));

            if (!WriteString(f, buf))
            {
                return false;
            }


            y += padding;

            const ThreadUtils::JobTraces& traces = m_threadTraces[t];
            for (int i = 0; i < traces.size(); ++i)
            {
                const float width2 = traces[i].m_duration * xScale;
                const float height2 = rowHeight * 0.5f;

                const int color = ColorizeJobTrace(traces[i]);
                const int strokeColor = 0;
                azsnprintf(buf, sizeof(buf),
                    "    <rect\n"
                    "       style='fill:#%06x;fill-rule:evenodd;stroke:#%06x;stroke-width:0.25px;stroke-linecap:butt;stroke-linejoin:miter;stroke-opacity:1'\n"
                    "       width='%f'\n"
                    "       height='%f'\n"
                    "       x='%f'\n"
                    "       y='%f' />\n",
                    color, strokeColor, width2, height2, x, y);

                if (!WriteString(f, buf))
                {
                    return false;
                }

                x += width2;
            }

            y += rowHeight;
        }

        if (!WriteString(f, "\n</svg>\n"))
        {
            return false;
        }

        fclose(f);
        return true;
    }

    // ---------------------------------------------------------------------------

    void JobGroup::Process(JobGroup::GroupInfo* info)
    {
        info->m_job.Run();

        long jobsLeft = --info->m_group->m_numJobsRunning;
        assert(jobsLeft >= 0);
        if (jobsLeft == 0)
        {
            info->m_group->m_finishJob.Run();
            delete info->m_group;
        }
    }

    JobGroup::JobGroup(StealingThreadPool* pool, JobFunc func, void* data)
        : m_pool(pool)
        , m_numJobsRunning(0)
        , m_finishJob(func, data)
        , m_submited(false)
    {
    }

    void JobGroup::Submit()
    {
        if (m_submited)
        {
            assert(0);
            return;
        }

        if (m_numJobsRunning == 0)
        {
            m_pool->Submit(m_finishJob);
            return;
        }

        Jobs jobs;
        jobs.resize(m_infos.size());
        for (size_t i = 0; i < m_infos.size(); ++i)
        {
            jobs[i] = Job((JobFunc) & JobGroup::Process, &m_infos[i]);
        }

        m_pool->Submit(jobs);
    }

    void JobGroup::Add(JobFunc func, void* data)
    {
        if (m_submited)
        {
            assert(0);
            return;
        }

        GroupInfo info;
        info.m_job = Job(func, data);
        info.m_group = this;

        m_infos.push_back(info);
        ++m_numJobsRunning;
    }
}
