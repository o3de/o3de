/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Task/Internal/Task.h>
#include <AzCore/Task/TaskDescriptor.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/binary_semaphore.h>
#include <AzCore/Memory/PoolAllocator.h>

#ifdef AZ_DEBUG_BUILD
#define ENABLE_COMPILED_TASK_GRAPH_EVENT_TRACKING
#endif

namespace AZ
{
    class TaskGraphEvent;
    class TaskGraph;
    class TaskExecutor;

    namespace Internal
    {
        class CompiledTaskGraphTracker;

        // Implement basic CompiledTaskGraph event breadcrumbs to help debug
        // https://github.com/o3de/o3de/issues/12015
        enum CTGEvent
        {
            None,
            Allocated,
            Deallocated,
            Submitted,
            Signalled,
        };
        class CompiledTaskGraphTracker final
        {
        public:
            CompiledTaskGraphTracker(TaskExecutor* executor [[maybe_unused]])
#ifdef ENABLE_COMPILED_TASK_GRAPH_EVENT_TRACKING
                : m_taskExecutor(executor)
#endif
            {};
#ifdef ENABLE_COMPILED_TASK_GRAPH_EVENT_TRACKING
            void WriteEventInfo(const CompiledTaskGraph* ctg, CTGEvent eventCode, const char* identifier);
#else
            void WriteEventInfo(const CompiledTaskGraph* ctg [[maybe_unused]], CTGEvent eventCode [[maybe_unused]], const char* identifier [[maybe_unused]]) {};
#endif

        private:
#ifdef ENABLE_COMPILED_TASK_GRAPH_EVENT_TRACKING
            static constexpr uint32_t NumTrackedRecentEvents = 1024u;
            struct CTGEventData
            {
                const CompiledTaskGraph* m_ctg = nullptr;
                const char* m_parentLabel = nullptr;
                const char* m_identifier = nullptr;
                const char* m_threadName = nullptr;
                uint32_t m_remainingCount = 0;
                CTGEvent m_eventCode = CTGEvent::None;
                bool m_retained = true;
            };
            TaskExecutor* m_taskExecutor = nullptr;
            CTGEventData m_recentEvents[NumTrackedRecentEvents]; // Allocation breadcrumbs to help look for a double delete
            uint32_t m_nextEventSlot = 0;
            AZStd::mutex m_mutex;
#endif
        };

        class CompiledTaskGraph final
        {
        public:
            AZ_CLASS_ALLOCATOR(CompiledTaskGraph, SystemAllocator);

            CompiledTaskGraph(
                AZStd::vector<Task>&& tasks,
                AZStd::unordered_map<uint32_t, AZStd::vector<uint32_t>>& links,
                size_t linkCount,
                TaskGraph* parent,
                const char* parentLabel);

            AZStd::vector<Task>& Tasks() noexcept
            {
                return m_tasks;
            }

            // Indicate that a constituent task has finished and decrement a counter to determine if the
            // graph should be freed (returns the value after atomic decrement)
            uint32_t Release(CompiledTaskGraphTracker& allocationTracker);

            // Debug access
            const char* GetParentLabel() const { return m_parentLabel; }
            bool IsRetained() const { return m_parent != nullptr; }
            uint32_t GetRemainingCount() const { return m_remaining.load(); }

        private:
            friend class ::AZ::TaskGraph;
            friend class TaskWorker;

            AZStd::vector<Task> m_tasks;
            AZStd::vector<Task*> m_successors;
            TaskGraphEvent* m_waitEvent = nullptr;
            // The pointer to the parent graph is set only if it is retained
            TaskGraph* m_parent = nullptr;
            AZStd::atomic<uint32_t> m_remaining;
            const char* m_parentLabel;
        };

        class TaskWorker;
    } // namespace Internal

    class TaskExecutor final
    {
    public:
        AZ_CLASS_ALLOCATOR(TaskExecutor, SystemAllocator);

        static TaskExecutor& Instance();

        // Invoked by a system component on program launch
        static void SetInstance(TaskExecutor* executor);

        // Passing 0 for the threadCount requests for the thread count to match the hardware concurrency
        explicit TaskExecutor(uint32_t threadCount = 0);
        ~TaskExecutor();

        // Submit a task graph for execution. Waitable task graphs cannot enqueue work on the task thread
        // that is currently active
        void Submit(Internal::CompiledTaskGraph& graph, TaskGraphEvent* event);

        void Submit(Internal::Task& task);

        Internal::CompiledTaskGraphTracker& GetEventTracker() {return m_eventTracker;}

    private:
        friend class Internal::TaskWorker;
        friend class TaskGraphEvent;
        friend class Internal::CompiledTaskGraphTracker;

        Internal::TaskWorker* GetTaskWorker();
        void ReleaseGraph();
        void ReactivateTaskWorker();

        Internal::TaskWorker* m_workers;
        uint32_t m_threadCount = 0;
        AZStd::atomic<uint32_t> m_lastSubmission;
        AZStd::atomic<uint64_t> m_graphsRemaining;

        // Implement basic CompiledTaskGraph event breadcrumbs to help debug
        // https://github.com/o3de/o3de/issues/12015
        Internal::CompiledTaskGraphTracker m_eventTracker;
    };
} // namespace AZ
