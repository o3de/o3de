/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_REPLICA_TASK_H
#define GM_REPLICA_TASK_H

#include <AzCore/Memory/Memory.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/std/containers/intrusive_list.h>
#include <AzCore/std/containers/queue.h>
#include <GridMate/Replica/Replica.h>

namespace GridMate
{
    class ReplicaManager;

    /**
    *  Base class for the all tasks in task system. Every subclassed class should implement GetPriority() and Run() methods
    *  to be able to be queued and executed from within TaskSystem.
    */
    class ReplicaTask
    {
    public:
        ReplicaTask()
            : m_replica(nullptr)
            , m_cancelled(false)
            , m_age(0)
            , m_priority(0)
        {}

        // TaskStatus returned after task execution is finished
        enum class TaskStatus
        {
            Done,   //< tells TaskManager that task is accomplished and can be removed
            Repeat  //< task needs to be repeated on next TaskManager update
        };

        struct RunContext
        {
            ReplicaManager* m_replicaManager; //< current replica manager
        };

        typedef AZ::u64 PriorityType;

        virtual ~ReplicaTask() { }

        /**
        * Called when task is executed
        * @param context execution context, provides access to ReplicaManager and TaskManager
        * @return TaskStatus::Done if task is completed, TaskStatus::Repeat if task needs to be repeated next tick.
        */
        virtual TaskStatus Run(const RunContext& context) = 0;

        /**
        * This method sets cancelled state on the task. The task will not be executed and will be deleted on next TaskManager's tick.
        * The task will be deleted upon completion when it's cancelled while executing.
        */
        void Cancel() { m_cancelled = true; }

        /**
        * Indicates whether the task is cancelled.
        * @return true if cancelled, false otherwise
        */
        bool IsCancelled() const { return m_cancelled; }

        /**
        * Returns 'age' of the task. Age indicates for how many frame ticks this task was postponed.
        */
        unsigned int GetAge() const { return m_age; }

        /**
        * Setter for age. Typically age is only modified by task manager.
        */
        void SetAge(unsigned int age) { m_age = age; }

        /**
        * Returns replica associated with the task, nullptr if task is not bound to any replica
        */
        ReplicaPtr GetReplica() const { return m_replica; }

        /**
        * Saves priority in a task.
        */
        void SetPriority(PriorityType priority) { m_priority = priority; }

        /**
        * Returns a cached priority for a task
        */
        PriorityType GetPriority() const { return m_priority; }


        static const unsigned int k_ageScale = 10;

    protected:
        ReplicaPtr m_replica;

    private:
        bool m_cancelled;
        unsigned int m_age;
        PriorityType m_priority;
    };
} // namespace GridMate

#endif // GM_REPLICA_TASK_H
