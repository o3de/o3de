/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Jobs/JobExecutor.h>
#include <AzCore/Jobs/JobGraph.h>

#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/queue.h>
#include <AzCore/std/parallel/binary_semaphore.h>
#include <AzCore/std/parallel/semaphore.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/scoped_lock.h>
#include <AzCore/std/parallel/thread.h>

#include <random>

namespace AZ
{
    constexpr static size_t PRIORITY_COUNT = static_cast<size_t>(JobPriority::PRIORITY_COUNT);

    namespace Internal
    {
        CompiledJobGraph::CompiledJobGraph(
            AZStd::vector<TypeErasedJob>&& jobs,
            AZStd::unordered_map<uint32_t, AZStd::vector<uint32_t>>& links,
            size_t linkCount,
            bool retained)
            : m_remaining{ jobs.size() }
            , m_retained{ retained }
        {
            m_jobs = AZStd::move(jobs);
            m_dependencyCounts = reinterpret_cast<AZStd::atomic<uint32_t>*>(azcalloc(sizeof(AZStd::atomic<uint32_t>) * m_jobs.size()));
            m_successors.resize(linkCount);

            uint32_t* cursor = m_successors.data();

            for (size_t i = 0; i != m_jobs.size(); ++i)
            {
                TypeErasedJob& job = m_jobs[i];
                job.m_successorOffset = cursor - m_successors.data();
                cursor += job.m_outboundLinkCount;

                AZ_Assert(job.m_outboundLinkCount == links[i].size(), "Job outbound link information mismatch");

                for (uint32_t j = 0; j != job.m_outboundLinkCount; ++j)
                {
                    m_successors[static_cast<size_t>(job.m_successorOffset) + j] = links[i][j];
                }

                if (job.m_inboundLinkCount > 0)
                {
                    m_dependencyCounts[i].store(job.m_inboundLinkCount, AZStd::memory_order_release);
                }
            }

            // TODO: Check for dependency cycles
        }

        CompiledJobGraph::~CompiledJobGraph()
        {
            if (m_dependencyCounts)
            {
                azfree(m_dependencyCounts);
            }
        }

        void CompiledJobGraph::Release()
        {
            if (--m_remaining == 0)
            {
                if (m_retained)
                {
                    m_remaining = m_jobs.size();
                    for (size_t i = 0; i != m_jobs.size(); ++i)
                    {
                        TypeErasedJob& job = m_jobs[i];
                        if (job.m_inboundLinkCount > 0)
                        {
                            m_dependencyCounts[i].store(job.m_inboundLinkCount, AZStd::memory_order_release);
                        }
                    }
                }

                if (m_waitEvent)
                {
                    m_waitEvent->m_submitted = false;
                    m_waitEvent->Signal();
                }

                if (!m_retained)
                {
                    azdestroy(this);
                }
            }
        }

        struct QueueStatus
        {
            AZStd::atomic<uint16_t> head;
            AZStd::atomic<uint16_t> tail;
            AZStd::atomic<uint16_t> reserve;
        };

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
            constexpr static uint8_t PriorityLevelCount = static_cast<uint8_t>(JobPriority::PRIORITY_COUNT);

            JobQueue() = default;
            JobQueue(const JobQueue&) = delete;
            JobQueue& operator=(const JobQueue&) = delete;

            bool Enqueue(TypeErasedJob* job);
            TypeErasedJob* TryDequeue();

        private:
            QueueStatus m_status[PriorityLevelCount] = {};
            TypeErasedJob* m_queues[PriorityLevelCount][MaxQueueSize] = {};
        };

        bool JobQueue::Enqueue(TypeErasedJob* job)
        {
            uint8_t priority = job->GetPriorityNumber();
            QueueStatus& status = m_status[priority];

            while (true)
            {
                uint16_t reserve = status.reserve.load();
                uint16_t head = status.head.load();

                // Enqueuing is done in two phases because we cannot atomically write the job to the slot we reserve
                // and simulataneously publish the fact that the slot is now available.
                if (reserve != head - 1)
                {
                    // Try to reserve a slot
                    if (status.reserve.compare_exchange_weak(reserve, reserve + 1))
                    {
                        m_queues[priority][reserve] = job;

                        uint16_t expectedReserve = reserve;

                        // Increment the tail to advertise the new job
                        while (!status.tail.compare_exchange_weak(expectedReserve, reserve + 1))
                        {
                            expectedReserve = reserve;
                        }

                        return status.head == status.tail - 1; 
                    }

                    // We failed to reserve a slot, try again
                }
                else
                {
                    // TODO need exponential backup here
                    AZStd::this_thread::sleep_for(AZStd::chrono::microseconds{ 100 });
                }
            }
        }

        TypeErasedJob* JobQueue::TryDequeue()
        {
            for (size_t priority = 0; priority != PriorityLevelCount; ++priority)
            {
                QueueStatus& status = m_status[priority];
                while (true)
                {
                    uint16_t head = status.head.load();
                    uint16_t tail = status.tail.load();
                    if (head == tail)
                    {
                        // Queue empty
                        break;
                    }
                    else
                    {
                        TypeErasedJob* job = m_queues[priority][status.head];
                        if (status.head.compare_exchange_weak(head, head + 1))
                        {
                            return job;
                        }
                    }
                }
            }

            return nullptr;
        }

        class JobWorker
        {
        public:
            void Spawn(::AZ::JobExecutor& executor, size_t id, AZStd::semaphore& initSemaphore, bool affinitize)
            {
                m_executor = &executor;

                AZStd::string threadName = AZStd::string::format("JobWorker %zu", id);
                AZStd::thread_desc desc = {};
                desc.m_name = threadName.c_str();
                if (affinitize)
                {
                    desc.m_cpuId = 1 << id;
                }
                m_active.store(true, AZStd::memory_order_release);

                m_thread = AZStd::thread{ [this, &initSemaphore]
                                          {
                                              initSemaphore.release();
                                              Run();
                                          },
                                          &desc };
            }

            void Join()
            {
                m_active.store(false, AZStd::memory_order_release);
                m_semaphore.release();
                m_thread.join();
            }

            void Enqueue(TypeErasedJob* job)
            {
                if (m_queue.Enqueue(job))
                {
                    // The queue was empty prior to enqueueing the job, release the semaphore
                    m_semaphore.release();
                }
            }

        private:
            void Run()
            {
                while (m_active)
                {
                    m_semaphore.acquire();
                    // m_semaphore.try_acquire_for(AZStd::chrono::microseconds{ 10 });

                    if (!m_active)
                    {
                        return;
                    }

                    TypeErasedJob* job = m_queue.TryDequeue();
                    while (job)
                    {
                        job->Invoke();
                        // Decrement counts for all job successors
                        for (size_t j = 0; j != job->m_outboundLinkCount; ++j)
                        {
                            uint32_t successorIndex = job->m_graph->m_successors[job->m_successorOffset + j];
                            if (--job->m_graph->m_dependencyCounts[successorIndex] == 0)
                            {
                                m_executor->Submit(job->m_graph->m_jobs[successorIndex]);
                            }
                        }

                        job->m_graph->Release();
                        --m_executor->m_remaining;

                        job = m_queue.TryDequeue();
                    }
                }
            }

            AZStd::thread m_thread;
            AZStd::atomic<bool> m_active;
            AZStd::binary_semaphore m_semaphore;

            ::AZ::JobExecutor* m_executor;
            JobQueue m_queue;
        };
    } // namespace Internal

    JobExecutor& JobExecutor::Instance()
    {
        // TODO: Create the default executor as part of a component (as in JobManagerComponent)
        static JobExecutor executor;
        return executor;
    }

    JobExecutor::JobExecutor(uint32_t threadCount)
    {
        // TODO: Configure thread count + affinity based on configuration
        m_threadCount = threadCount == 0 ? AZStd::thread::hardware_concurrency() : threadCount;

        m_workers = reinterpret_cast<Internal::JobWorker*>(azmalloc(m_threadCount * sizeof(Internal::JobWorker)));

        bool affinitize = m_threadCount == AZStd::thread::hardware_concurrency();

        AZStd::semaphore initSemaphore;

        for (size_t i = 0; i != m_threadCount; ++i)
        {
            new (m_workers + i) Internal::JobWorker{};
            m_workers[i].Spawn(*this, i, initSemaphore, affinitize);
        }

        for (size_t i = 0; i != m_threadCount; ++i)
        {
            initSemaphore.acquire();
        }
    }

    JobExecutor::~JobExecutor()
    {
        for (size_t i = 0; i != m_threadCount; ++i)
        {
            m_workers[i].Join();
            m_workers[i].~JobWorker();
        }

        azfree(m_workers);
    }

    void JobExecutor::Submit(Internal::CompiledJobGraph& graph)
    {
        for (Internal::TypeErasedJob& job : graph.Jobs())
        {
            job.AttachToJobGraph(graph);
        }

        // Submit all jobs that have no inbound edges
        for (Internal::TypeErasedJob& job : graph.Jobs())
        {
            if (job.IsRoot())
            {
                Submit(job);
            }
        }
    }

    void JobExecutor::Submit(Internal::TypeErasedJob& job)
    {
        // TODO: Something more sophisticated is likely needed here.
        // First, we are completely ignoring affinity.
        // Second, some heuristics on core availability will help distribute work more effectively
        ++m_remaining;
        m_workers[++m_lastSubmission % m_threadCount].Enqueue(&job);
    }

    void JobExecutor::Drain()
    {
        while (m_remaining > 0)
        {
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds{ 100 });
        }
    }
} // namespace AZ
