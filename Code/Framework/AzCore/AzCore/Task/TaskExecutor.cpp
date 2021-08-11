/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Task/TaskExecutor.h>
#include <AzCore/Task/TaskGraph.h>

#include <AzCore/std/containers/queue.h>
#include <AzCore/std/parallel/binary_semaphore.h>
#include <AzCore/std/parallel/exponential_backoff.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/scoped_lock.h>
#include <AzCore/std/parallel/semaphore.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Module/Environment.h>

#include <random>

namespace AZ
{
    namespace Internal
    {
        CompiledTaskGraph::CompiledTaskGraph(
            AZStd::vector<Task>&& tasks,
            AZStd::unordered_map<uint32_t, AZStd::vector<uint32_t>>& links,
            size_t linkCount,
            TaskGraph* parent)
            : m_parent{ parent }
        {
            m_tasks = AZStd::move(tasks);
            m_successors.resize(linkCount);

            Task** cursor = m_successors.data();

            for (uint32_t i = 0; i != m_tasks.size(); ++i)
            {
                Task& task = m_tasks[i];
                task.m_graph = this;
                task.m_successorOffset = static_cast<uint32_t>(cursor - m_successors.data());
                cursor += task.m_outboundLinkCount;

                AZ_Assert(task.m_outboundLinkCount == links[i].size(), "Task outbound link information mismatch");

                for (uint32_t j = 0; j != task.m_outboundLinkCount; ++j)
                {
                    m_successors[static_cast<size_t>(task.m_successorOffset) + j] = &m_tasks[links[i][j]];
                }
            }

            // TODO: Check for dependency cycles
        }

        uint32_t CompiledTaskGraph::Release()
        {
            uint32_t remaining = --m_remaining;

            if (m_parent)
            {
                if (remaining == 1)
                {
                    // Allow the parent graph to be submitted again
                    m_parent->m_submitted = false;
                }
            }
            else if (remaining == 0)
            {
                if (m_waitEvent)
                {
                    m_waitEvent->Signal();
                }

                azdestroy(this);
                return remaining;
            }

            if (m_waitEvent && remaining == (m_parent ? 1 : 0))
            {
                m_waitEvent->Signal();
            }

            return remaining;
        }

        struct QueueStatus
        {
            AZStd::atomic<uint16_t> head;
            AZStd::atomic<uint16_t> tail;
            AZStd::atomic<uint16_t> reserve;
        };

        // The Task Queue is a lock free 4-priority queue. Its basic operation is as follows:
        // Each priority level is associated with a different queue, corresponding to the maximum size of a uint16_t.
        // Each queue is implemented as a ring buffer, and a 64 bit atomic maintains the following state per queue:
        // - offset to the "head" of the ring, from where we acquire elements
        // - offset to the "tail" of the ring, which tracks where new elements should be enqueued
        // - offset to a tail reservation index, which is used to reserve a slot to enqueue elements
        class TaskQueue final
        {
        public:
            // Preallocating upfront allows us to reserve slots to insert tasks without locks.
            // Each thread allocated by the task manager consumes ~2 MB.
            constexpr static uint16_t MaxQueueSize = 0xffff;
            constexpr static uint8_t PriorityLevelCount = static_cast<uint8_t>(TaskPriority::PRIORITY_COUNT);

            TaskQueue() = default;
            TaskQueue(const TaskQueue&) = delete;
            TaskQueue& operator=(const TaskQueue&) = delete;

            void Enqueue(Task* task);
            Task* TryDequeue();

        private:
            QueueStatus m_status[PriorityLevelCount] = {};
            Task* m_queues[PriorityLevelCount][MaxQueueSize] = {};
        };

        void TaskQueue::Enqueue(Task* task)
        {
            uint8_t priority = task->GetPriorityNumber();
            QueueStatus& status = m_status[priority];

            AZStd::exponential_backoff backoff;
            while (true)
            {
                uint16_t reserve = status.reserve.load();
                uint16_t head = status.head.load();

                // Enqueuing is done in two phases because we cannot atomically write the task to the slot we reserve
                // and simulataneously publish the fact that the slot is now available.
                if (reserve != head - 1)
                {
                    // Try to reserve a slot
                    if (status.reserve.compare_exchange_weak(reserve, reserve + 1))
                    {
                        m_queues[priority][reserve] = task;

                        uint16_t expectedReserve = reserve;

                        // Increment the tail to advertise the new task
                        while (!status.tail.compare_exchange_weak(expectedReserve, reserve + 1))
                        {
                            expectedReserve = reserve;
                        }

                        return;
                    }

                    // We failed to reserve a slot, try again
                }
                else
                {
                    backoff.wait();
                }
            }
        }

        Task* TaskQueue::TryDequeue()
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
                        Task* task = m_queues[priority][status.head];
                        if (status.head.compare_exchange_weak(head, head + 1))
                        {
                            return task;
                        }
                    }
                }
            }

            return nullptr;
        }

        class TaskWorker
        {
        public:
            void Spawn(::AZ::TaskExecutor& executor, size_t id, AZStd::semaphore& initSemaphore, bool affinitize)
            {
                m_executor = &executor;

                AZStd::string threadName = AZStd::string::format("TaskWorker %zu", id);
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

            void Enqueue(Task* task)
            {
                m_queue.Enqueue(task);

                if (!m_busy.exchange(true))
                {
                    // The worker was idle prior to enqueueing the task, release the semaphore
                    m_semaphore.release();
                }
            }

        private:
            void Run()
            {
                while (m_active)
                {
                    m_busy = false;
                    m_semaphore.acquire();

                    if (!m_active)
                    {
                        return;
                    }

                    m_busy = true;

                    Task* task = m_queue.TryDequeue();
                    while (task)
                    {
                        task->Invoke();
                        // Decrement counts for all task successors
                        for (size_t j = 0; j != task->m_outboundLinkCount; ++j)
                        {
                            Task* successor = task->m_graph->m_successors[task->m_successorOffset + j];
                            if (--successor->m_dependencyCount == 0)
                            {
                                m_executor->Submit(*successor);
                            }
                        }

                        bool isRetained = task->m_graph->m_parent != nullptr;
                        if (task->m_graph->Release() == (isRetained ? 1 : 0))
                        {
                            m_executor->ReleaseGraph();
                        }

                        task = m_queue.TryDequeue();
                    }
                }
            }

            AZStd::thread m_thread;
            AZStd::atomic<bool> m_active;
            AZStd::atomic<bool> m_busy;
            AZStd::binary_semaphore m_semaphore;

            ::AZ::TaskExecutor* m_executor;
            TaskQueue m_queue;
        };
    } // namespace Internal

    static EnvironmentVariable<TaskExecutor*> s_executor;
    constexpr static const char* s_executorName = "GlobalTaskExecutor";
    TaskExecutor& TaskExecutor::Instance()
    {
        if (!s_executor)
        {
            s_executor = AZ::Environment::FindVariable<TaskExecutor*>(s_executorName);
        }

        return **s_executor;
    }

    // TODO: Create the default executor as part of a component (as in TaskManagerComponent)
    void TaskExecutor::SetInstance(TaskExecutor* executor)
    {
        AZ_Assert(!s_executor, "Attempting to set the global task executor more than once");

        s_executor = AZ::Environment::CreateVariable<TaskExecutor*>("GlobalTaskExecutor");
        s_executor.Set(executor);
    }

    TaskExecutor::TaskExecutor(uint32_t threadCount)
    {
        // TODO: Configure thread count + affinity based on configuration
        m_threadCount = threadCount == 0 ? AZStd::thread::hardware_concurrency() : threadCount;

        m_workers = reinterpret_cast<Internal::TaskWorker*>(azmalloc(m_threadCount * sizeof(Internal::TaskWorker)));

        bool affinitize = m_threadCount == AZStd::thread::hardware_concurrency();

        AZStd::semaphore initSemaphore;

        for (size_t i = 0; i != m_threadCount; ++i)
        {
            new (m_workers + i) Internal::TaskWorker{};
            m_workers[i].Spawn(*this, i, initSemaphore, affinitize);
        }

        for (size_t i = 0; i != m_threadCount; ++i)
        {
            initSemaphore.acquire();
        }
    }

    TaskExecutor::~TaskExecutor()
    {
        for (size_t i = 0; i != m_threadCount; ++i)
        {
            m_workers[i].Join();
            m_workers[i].~TaskWorker();
        }

        azfree(m_workers);
    }

    void TaskExecutor::Submit(Internal::CompiledTaskGraph& graph)
    {
        ++m_graphsRemaining;
        // Submit all tasks that have no inbound edges
        for (Internal::Task& task : graph.Tasks())
        {
            if (task.IsRoot())
            {
                Submit(task);
            }
        }
    }

    void TaskExecutor::Submit(Internal::Task& task)
    {
        // TODO: Something more sophisticated is likely needed here.
        // First, we are completely ignoring affinity.
        // Second, some heuristics on core availability will help distribute work more effectively
        m_workers[++m_lastSubmission % m_threadCount].Enqueue(&task);
    }

    void TaskExecutor::ReleaseGraph()
    {
        --m_graphsRemaining;
    }
} // namespace AZ
