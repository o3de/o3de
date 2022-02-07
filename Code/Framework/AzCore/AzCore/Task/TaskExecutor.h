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

namespace AZ
{
    class TaskGraphEvent;
    class TaskGraph;

    namespace Internal
    {
        class CompiledTaskGraph final
        {
        public:
            AZ_CLASS_ALLOCATOR(CompiledTaskGraph, SystemAllocator, 0)

            CompiledTaskGraph(
                AZStd::vector<Task>&& tasks,
                AZStd::unordered_map<uint32_t, AZStd::vector<uint32_t>>& links,
                size_t linkCount,
                TaskGraph* parent);

            AZStd::vector<Task>& Tasks() noexcept
            {
                return m_tasks;
            }

            // Indicate that a constituent task has finished and decrement a counter to determine if the
            // graph should be freed (returns the value after atomic decrement)
            uint32_t Release();

        private:
            friend class ::AZ::TaskGraph;
            friend class TaskWorker;

            AZStd::vector<Task> m_tasks;
            AZStd::vector<Task*> m_successors;
            TaskGraphEvent* m_waitEvent = nullptr;
            // The pointer to the parent graph is set only if it is retained
            TaskGraph* m_parent = nullptr;
            AZStd::atomic<uint32_t> m_remaining;
        };

        class TaskWorker;
    } // namespace Internal

    class TaskExecutor final
    {
    public:
        AZ_CLASS_ALLOCATOR(TaskExecutor, SystemAllocator, 0);

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

    private:
        friend class Internal::TaskWorker;
        friend class TaskGraphEvent;

        Internal::TaskWorker* GetTaskWorker();
        void ReleaseGraph();
        void ReactivateTaskWorker();

        Internal::TaskWorker* m_workers;
        uint32_t m_threadCount = 0;
        AZStd::atomic<uint32_t> m_lastSubmission;
        AZStd::atomic<uint64_t> m_graphsRemaining;
    };
} // namespace AZ
