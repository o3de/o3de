/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// NOTE: If adding additional header/symbol dependencies, consider if such additions are better
// suited in the private CompiledTaskGraph implementation instead to keep this header lean.
#include <AzCore/Task/Internal/Task.h>
#include <AzCore/Task/TaskDescriptor.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/parallel/binary_semaphore.h>
#include <AzCore/RTTI/RTTI.h>

namespace AZ
{
    namespace Internal
    {
        class CompiledTaskGraph;
        class TaskWorker;
    }
    class TaskExecutor;
    class TaskGraph;

    class TaskGraphActiveInterface
    {
    public:
        AZ_RTTI(TaskGraphActiveInterface, "{08118074-B139-4EF9-B8FD-29F1D6DC9233}");

        virtual bool IsTaskGraphActive() const = 0;
    };

    // A TaskToken is returned each time a Task is added to the TaskGraph. TaskTokens are used to
    // express dependencies between tasks within the graph, and have no purpose after the graph
    // is submitted (simply let them go out of scope)
    class TaskToken final
    {
    public:
        // Indicate that this task must finish before the task token(s) passed as the argument
        template <typename... JT>
        void Precedes(JT&... tokens);

        // Indicate that this task must finish after the task token(s) passed as the argument
        template <typename... JT>
        void Follows(JT&... tokens);

    private:
        friend class TaskGraph;

        void PrecedesInternal(TaskToken& comesAfter);

        // Only the TaskGraph should be creating TaskToken
        TaskToken(TaskGraph& parent, uint32_t index);

        TaskGraph& m_parent;
        uint32_t m_index;
    };

    // A TaskGraphEvent may be used to block until one or more task graphs has finished executing. Usage
    // is NOT recommended for the majority of tasks (prefer to simply containing expanding/contracting
    // the graph without synchronization over the course of the frame). However, the event
    // is useful for the edges of the computation graph.
    //
    // You are responsible for ensuring the event object lifetime exceeds the task graph lifetime.
    //
    // After the TaskGraphEvent is signaled, you are NOT allowed to reuse the same TaskGraphEvent
    // for a future submission.
    class TaskGraphEvent
    {
    public:
        // ! The supplied string label is expected to be a string literal or otherwise outlive the lifetime of this TG event.
        explicit TaskGraphEvent(const char* label);
        bool IsSignaled();
        void Wait();

    private:
        friend class ::AZ::Internal::CompiledTaskGraph;
        friend class TaskGraph;
        friend class TaskExecutor;

        void IncWaitCount();
        void Signal();

        AZStd::binary_semaphore m_semaphore;
        AZStd::atomic_int       m_waitCount = 0;
        TaskExecutor*           m_executor = nullptr;
        [[maybe_unused]] const char* m_label = nullptr;
    };

    // The TaskGraph encapsulates a set of tasks and their interdependencies. After adding
    // tasks, and marking dependencies as necessary, the entire graph is submitted via
    // the TaskGraph::Submit method.
    //
    // The TaskGraph MAY be retained across multiple frames and resubmitted, provided the
    // user provides some guarantees (see comments associated with TaskGraph::Retain).
    class TaskGraph final
    {
    public:
        // ! The supplied string label is expected to be a string literal or otherwise outlive the lifetime of this TG.
        explicit TaskGraph(const char* graphlabel);
        TaskGraph(AZStd::nullptr_t) = delete;
        ~TaskGraph();

        // Reset the state of the task graph to begin recording tasks and edges again
        // NOTE: Graph must be in a "settled" state (cannot be in-flight)
        void Reset();
        
        // Returns false if 1 or more tasks have been added to the graph
        bool IsEmpty();

        // Add a task to the graph, retrieiving a token that can be used to express dependencies
        // between tasks. The first argument specifies the TaskKind, used for tracking the task.
        // NOTE: This operation is invalid if the graph is in-flight
        template<typename Lambda>
        TaskToken AddTask(TaskDescriptor const& descriptor, Lambda&& lambda);

        template <typename... Lambdas>
        AZStd::array<TaskToken, sizeof...(Lambdas)> AddTasks(TaskDescriptor const& descriptor, Lambdas&&... lambdas);

        // By default, you are responsible for retaining the TaskGraph, indicating you promise that
        // this TaskGraph will live as long as it takes for all constituent tasks to complete.
        // Once retained, this task graph can be resubmitted after completion without any
        // modifications. TaskTokens that were created as a result of adding tasks used to
        // mark dependencies DO NOT need to outlive the task graph.
        //
        // Invoking Detach PRIOR to submission indicates you wish the tasks associated with this
        // TaskGraph to deallocate upon completion. After invoking Detach, you may let this TaskGraph
        // go out of scope or deallocate after submission.
        //
        // NOTE: The TaskGraph has no concept of resources used by design. Resubmission
        // of the task graph is expected to rely on either indirection, or safe overwriting
        // of previously used memory to supply new data (this can even be done as the first
        // task in the graph).
        // NOTE: This operation is invalid if the graph is in-flight
        void Detach();

        // Invoke the task graph, asserting if there are dependency violations. Note that
        // submitting the same graph multiple times to process simultaneously is VALID
        // behavior. This is, for example, a mechanism that allows a task graph to loop
        // in perpetuity (in fact, the entire frame could be modeled as a single task graph,
        // where the final task resubmits the task graph again).
        //
        // This API is not designed to protect against memory safety violations (nothing
        // can prevent a user from incorrectly aliasing memory unsafely even without repeated
        // submission). To catch memory safety violations, it is ENCOURAGED that you access
        // data through TaskResource<T> handles.
        void Submit(TaskGraphEvent* waitEvent = nullptr);

        // Same as submit but run on a different executor than the default system executor
        void SubmitOnExecutor(TaskExecutor& executor, TaskGraphEvent* waitEvent = nullptr);

    private:
        friend class TaskToken;
        friend class Internal::CompiledTaskGraph;

        Internal::CompiledTaskGraph* m_compiledTaskGraph = nullptr;

        AZStd::vector<Internal::Task> m_tasks;

        // Task index |-> Dependent task indices
        AZStd::unordered_map<uint32_t, AZStd::vector<uint32_t>> m_links;

        char const* m_label;
        uint32_t m_linkCount = 0;
        bool m_retained = true;
        AZStd::atomic<bool> m_submitted = false;
    };
} // namespace AZ

#include <AzCore/Task/TaskGraph.inl>
