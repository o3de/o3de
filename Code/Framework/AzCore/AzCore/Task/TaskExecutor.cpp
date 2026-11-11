/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Task/TaskExecutor.h>
#include <AzCore/Task/TaskGraph.h>
#include <AzCore/Task/Internal/WorkStealingDeque.h>

#include <AzCore/std/parallel/shared_mutex.h>
#include <AzCore/std/parallel/lock.h>
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
            TaskGraph* parent,
            const char* parentLabel)
            : m_parent{ parent }
            , m_parentLabel{ parentLabel }
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

        uint32_t CompiledTaskGraph::Release(CompiledTaskGraphTracker& eventTracker)
        {
            // Release is run from many threads, and another thread can azdestroy(this) as soon as the remaining count is decremented.
            // READ ALL NECESSARY DATA BEFORE DECREMENTING THE REMAINING COUNT!
            bool isRetained = IsRetained();
            TaskGraph* parent = m_parent;
            TaskGraphEvent* waitEvent = m_waitEvent;
            uint32_t remaining = --m_remaining;

            if (isRetained)
            {
                if (remaining == 1)
                {
                    // Allow the parent graph to be submitted again
                    parent->m_submitted = false;
                    if (waitEvent)
                    {
                        eventTracker.WriteEventInfo(this, CTGEvent::Signalled, "CTG::Release parent=true");
                        waitEvent->Signal();
                    }
                }
            }
            else if (remaining == 0)
            {
                if (waitEvent)
                {

                    eventTracker.WriteEventInfo(this, CTGEvent::Signalled, "CTG::Release parent=false");
                    waitEvent->Signal();
                }

                eventTracker.WriteEventInfo(this, CTGEvent::Deallocated, "CTG::Release parent=false");

                delete this;
            }

            return remaining;
        }

        // The Task Queue is a lock free 4-priority queue. Its basic operation is as follows:
        // Each priority level is associated with a different queue, corresponding to the maximum size of a uint16_t.
        // Each queue is implemented as a ring buffer, and a 64 bit atomic maintains the following state per queue:
        // - offset to the "head" of the ring, from where we acquire elements
        // - offset to the "tail" of the ring, which tracks where new elements should be enqueued
        // - offset to a tail reservation index, which is used to reserve a slot to enqueue elements
        class TaskQueue final
        {
        public:
            constexpr static uint8_t PriorityLevelCount = static_cast<uint8_t>(TaskPriority::PRIORITY_COUNT);

            TaskQueue() = default;
            TaskQueue(const TaskQueue&) = delete;
            TaskQueue& operator=(const TaskQueue&) = delete;

            void Enqueue(Task* task);
            Task* TryDequeue(uint8_t priority);
            Task* TrySteal(uint8_t priority);

        private:
            WorkStealingDeque m_queues[PriorityLevelCount];
            mutable AZStd::shared_mutex m_mutex; // WSQ is SPMC
        };

        void TaskQueue::Enqueue(Task* task)
        {
            uint8_t priority = task->GetPriorityNumber();
            AZStd::unique_lock lock(m_mutex);
            m_queues[priority].Enqueue(task);
        }

        Task* TaskQueue::TryDequeue(uint8_t priority)
        {
            AZStd::shared_lock lock(m_mutex);
            return m_queues[priority].TryDequeue();
        }

        Task* TaskQueue::TrySteal(uint8_t priority)
        {
            return m_queues[priority].TrySteal();
        }

        class TaskWorker
        {
        public:
            static thread_local TaskWorker* t_worker;

            void Spawn(::AZ::TaskExecutor& executor, uint32_t id, AZStd::semaphore& initSemaphore, bool affinitize)
            {
                m_executor = &executor;

                m_threadName = AZStd::string::format("TaskWorker %u", id);
                m_id = id;
                AZStd::thread_desc desc = {};
                desc.m_name = m_threadName.c_str();
                if (affinitize)
                {
                    desc.m_cpuId = 1 << id;
                }

                m_thread = AZStd::thread{ desc,
                                          [this, &initSemaphore]
                                          {
                                              t_worker = this;
                                              initSemaphore.release();
                                              Run();
                                          } };
            }

            // Threads that wait on a graph to complete are disqualified from receiving tasks until the wait finishes
            void Disable()
            {
                m_enabled = false;
            }

            void Enable()
            {
                m_enabled = true;
            }

            bool Enabled() const
            {
                return m_enabled;
            }

            void Join()
            {
                m_thread.join();
            }

            void Enqueue(Task* task)
            {
                m_queue.Enqueue(task);
                
                ++m_executor->m_taskCount;
                m_executor->m_taskCount.notify_one();
            }

            const char* GetThreadName() {return m_threadName.c_str();}

        private:
            Task* GetTask()
            {
                // steal task from higher priority before popping lower priority tasks
                for (uint8_t priority = 0; priority < TaskQueue::PriorityLevelCount; priority++)
                {
                    Task* task = m_queue.TryDequeue(priority);

                    if (task)
                    {
                        return task;
                    }

                    for (uint32_t i = 1; i < m_executor->m_threadCount; i++)
                    {
                        task = m_executor->m_workers[(i + m_id) % m_executor->m_threadCount].m_queue.TrySteal(priority);

                        if (task)
                        {
                            return task;
                        }
                    }
                }
                return nullptr;
            }

            void Run()
            {
                while (m_executor->m_active)
                {
                    m_executor->m_taskCount.wait(0);

                    if (!m_executor->m_active)
                    {
                        return;
                    }

                    Task* task = GetTask();
                    if (task == nullptr)
                    {
                        continue;
                    }

                    do
                    {
                        task->Invoke();
                        Task* next = nullptr;

                        // Decrement counts for all task successors
                        for (size_t j = 0; j != task->m_outboundLinkCount; ++j)
                        {
                            Task* successor = task->m_graph->m_successors[task->m_successorOffset + j];
                            if (--successor->m_dependencyCount == 0)
                            {
                                // retain a successor with highest priority (no less than that of the current task), enqueue the others
                                // higher priority number is lower
                                if (next == nullptr && successor->GetPriorityNumber() <= task->GetPriorityNumber())
                                {
                                    next = successor;
                                    continue;
                                }
                                else if (next && successor->GetPriorityNumber() < next->GetPriorityNumber())
                                {
                                    m_executor->Submit(*next);
                                    next = successor;
                                    continue;
                                }
                                m_executor->Submit(*successor);
                            }
                        }

                        bool isRetained = task->m_graph->m_parent != nullptr;
                        if (task->m_graph->Release(m_executor->GetEventTracker()) == (isRetained ? 1u : 0u))
                        {
                            m_executor->ReleaseGraph();
                        }
                        task = next;
                    } while (task);

                    if (m_executor->m_taskCount.fetch_sub(1) == 1)
                    {
                        if (m_executor->m_finishTasksBeforeQuit)
                        {
                            m_executor->m_taskCount.notify_all();
                        }
                    }
                }
            }

            AZStd::thread m_thread;
            AZStd::atomic<bool> m_enabled = true;
            uint32_t m_id;

            ::AZ::TaskExecutor* m_executor;
            TaskQueue m_queue;
            AZStd::string m_threadName;
            friend class ::AZ::TaskExecutor;
        };

        thread_local TaskWorker* TaskWorker::t_worker = nullptr;

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Implement basic CompiledTaskGraph event breadcrumbs to help debug
        // https://github.com/o3de/o3de/issues/12015

#ifdef ENABLE_COMPILED_TASK_GRAPH_EVENT_TRACKING
        void CompiledTaskGraphTracker::WriteEventInfo(const CompiledTaskGraph* ctg, CTGEvent eventCode, const char* identifier)
        {
            const char* threadName = "None TaskGraph thread";
            if (m_taskExecutor)
            {
                if (auto taskWorker = m_taskExecutor->GetTaskWorker(); taskWorker)
                {
                    threadName = taskWorker->GetThreadName();
                }
            }
            // record event breadcrumbs
            AZStd::scoped_lock<AZStd::mutex> lock(m_mutex);

            CTGEventData& slot = m_recentEvents[m_nextEventSlot++ % NumTrackedRecentEvents];
            if (ctg)
            {
                slot.m_parentLabel = ctg->GetParentLabel();
                slot.m_remainingCount = ctg->GetRemainingCount();
                slot.m_retained = ctg->IsRetained();
            }
            else
            {
                slot.m_parentLabel = "Null ctg";
                slot.m_remainingCount = 0;
                slot.m_retained = true;
            }
            slot.m_identifier = identifier;
            slot.m_threadName = threadName;
            slot.m_ctg = ctg;
            slot.m_eventCode = eventCode;
        }
#endif
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
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

    void TaskExecutor::SetInstance(TaskExecutor* executor)
    {
        if (!executor) // allow unsetting the executor
        {
            s_executor.Reset();
        }
        else if (!s_executor) // ignore any extra executors after the first (this happens during unit tests)
        {
            s_executor = AZ::Environment::CreateVariable<TaskExecutor*>(s_executorName, executor);
        }
    }

    TaskExecutor::TaskExecutor(uint32_t threadCount, bool finishTasksBeforeQuit)
        : m_eventTracker(this)
        , m_finishTasksBeforeQuit(finishTasksBeforeQuit)
    {
        // TODO: Configure thread count + affinity based on configuration
        m_threadCount = threadCount == 0 ? AZStd::thread::hardware_concurrency() : threadCount;

        m_workers = reinterpret_cast<Internal::TaskWorker*>(azmalloc(m_threadCount * sizeof(Internal::TaskWorker)));

        AZStd::semaphore initSemaphore;

        m_active.store(true, AZStd::memory_order_release);
        for (uint32_t i = 0; i != m_threadCount; ++i)
        {
            new (m_workers + i) Internal::TaskWorker{};
            m_workers[i].Spawn(*this, i, initSemaphore, false);
        }

        for (size_t i = 0; i != m_threadCount; ++i)
        {
            initSemaphore.acquire();
        }
    }

    TaskExecutor::~TaskExecutor()
    {
        if (m_finishTasksBeforeQuit)
        {
            while (const auto count = m_taskCount.load()) {
                m_taskCount.wait(count);
            }
        }
        m_active.store(false, AZStd::memory_order_release);
        m_taskCount += m_threadCount;
        m_taskCount.notify_all();

        for (size_t i = 0; i != m_threadCount; ++i)
        {
            m_workers[i].Join();
            m_workers[i].~TaskWorker();
        }

        azfree(m_workers);
    }

    Internal::TaskWorker* TaskExecutor::GetTaskWorker()
    {
        if (Internal::TaskWorker::t_worker && Internal::TaskWorker::t_worker->m_executor == this)
        {
            return Internal::TaskWorker::t_worker;
        }
        return nullptr;
    }

    void TaskExecutor::Submit(Internal::CompiledTaskGraph& graph, TaskGraphEvent* event)
    {

        if (event)
        {
            event->IncWaitCount();
            event->m_executor = this; // Used to validate event is not waited for inside a job
        }

        // If there are no task in compiled task graph
        // and a TaskGraphEvent has been supplied, then it needs
        // to be signaled to release the semaphore so that the TaskGraphEvent::Wait
        // function does not deadlock
        auto& compiledTasks = graph.Tasks();
        if (compiledTasks.empty())
        {
            if (event != nullptr)
            {
                event->Signal();
            }
            return;
        }

        // Since there is at least one compiled task, it is safe
        // to increment the graphs remaining member
        ++m_graphsRemaining;

        // Submit all tasks that have no inbound edges
        for (Internal::Task& task : compiledTasks)
        {
            if (task.IsRoot())
            {
                Submit(task);
            }
        }
    }

    void TaskExecutor::Submit(Internal::Task& task)
    {
        // Prefer submitting in the current task worker queue for sake of cache locality
        if (Internal::TaskWorker::t_worker)
        {
            Internal::TaskWorker::t_worker->Enqueue(&task);
            return;
        }
        // TODO: Something more sophisticated is likely needed here.
        // First, we are completely ignoring affinity.
        // Second, some heuristics on core availability will help distribute work more effectively
        uint32_t nextWorker = ++m_lastSubmission % m_threadCount;
        while (!m_workers[nextWorker].Enabled())
        {
            // Graphs that are waiting for the completion of a task graph cannot enqueue tasks onto
            // the thread issuing the wait.
            nextWorker = ++m_lastSubmission % m_threadCount;
        }

        m_workers[nextWorker].Enqueue(&task);
    }

    void TaskExecutor::ReleaseGraph()
    {
        --m_graphsRemaining;
    }

    void TaskExecutor::ReactivateTaskWorker()
    {
        GetTaskWorker()->Enable();
    }
} // namespace AZ
