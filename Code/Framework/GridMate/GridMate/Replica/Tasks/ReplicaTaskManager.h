/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_REPLICA_TASKMANAGER_H
#define GM_REPLICA_TASKMANAGER_H

#include <AzCore/Debug/Trace.h>
#include <GridMate/Memory.h>
#include <GridMate/Replica/Tasks/ReplicaTask.h>
#include <AzCore/std/containers/intrusive_set.h>
#include <AzCore/Memory/PoolAllocator.h>


/**
*  Simple task manager system to execute replica related work (e.g. Marshaling) in ordered manner.
*  Used to avoid execution of unnecessary work. E.g. only execute marshaling for replicas that require to be forwarded to another peer.
*/

namespace GridMate {
    class ReplicaManager;
    class ReplicaPriorityPolicy;

    /**
    *  ReplicaTaskManager manages queue of replica tasks.
    *  Tasks execution order is based task priority policy. The amount of tasks executed
    */
    template<class ProcessPolicy, class PriorityPolicy>
    class ReplicaTaskManager
    {
    public:

        explicit ReplicaTaskManager(AZ::PoolAllocator* allocator)
            : m_allocator(allocator)
        {
        }

        ~ReplicaTaskManager()
        {
            Clear();
        }

        template<class T, class ... Args>
        T* Add(Args&& ... args)
        {
            T* task = CreateReplicaTask<T>(AZStd::forward<Args>(args) ...);
            PriorityPolicy::UpdatePriority(*task);
            Queue(task);
            return task;
        }

        void Clear()
        {
            while (m_tasks.begin() != m_tasks.end())
            {
                FreeReplicaTask(m_tasks.back());
                m_tasks.pop_back();
            }
        }

        void UpdatePriority(ReplicaTask* task)
        {
            PriorityPolicy::UpdatePriority(*task);
            AZStd::make_heap(m_tasks.begin(), m_tasks.end());
        }

        void Run(ReplicaManager* replicaMgr)
        {
            ReplicaTask::RunContext context;
            context.m_replicaManager = replicaMgr;

            m_processPolicy.BeginFrame(context);

            vector<ReplicaTask*> tasksToProcess = AZStd::move(m_tasks);

            while (!tasksToProcess.empty())
            {
                AZStd::pop_heap(tasksToProcess.begin(), tasksToProcess.end(), m_comp);
                ReplicaTask* task = tasksToProcess.back();
                tasksToProcess.pop_back();

                if (task->IsCancelled()) // task cancelled -> removing from the task list
                {
                    FreeReplicaTask(task);
                }
                else
                {
                    ReplicaTask::TaskStatus status = ReplicaTask::TaskStatus::Repeat;
                    unsigned int age = 0;

                    bool shouldProcess = m_processPolicy.ShouldProcess(context, *task);
                    if (shouldProcess)
                    {
                        status = task->Run(context);
                    }
                    else // task was postponed -> aging it
                    {
                        age = (task->GetAge() == k_replicaPriorityRealTime) ? k_replicaPriorityRealTime : task->GetAge() + 1;
                    }

                    if (status == ReplicaTask::TaskStatus::Done)
                    {
                        FreeReplicaTask(task);
                    }
                    else
                    {
                        task->SetAge(age);
                        PriorityPolicy::UpdatePriority(*task);
                        Queue(task);
                    }
                }
            }

            m_processPolicy.EndFrame(context);
        }

    private:

        template<class T, class ... Args>
        T* CreateReplicaTask(Args&& ... args)
        {
            T* task = new(m_allocator->Allocate(sizeof(T), 4))T(AZStd::forward<Args>(args) ...);
            return task;
        }

        void FreeReplicaTask(ReplicaTask* task)
        {
            task->~ReplicaTask();
            m_allocator->DeAllocate(task);
        }

        void Queue(ReplicaTask* task)
        {
            m_tasks.push_back(task);
            AZStd::push_heap(m_tasks.begin(), m_tasks.end(), m_comp);
        }

        AZ::PoolAllocator* m_allocator;
        vector<ReplicaTask*> m_tasks;
        ProcessPolicy m_processPolicy;
        typename PriorityPolicy::Compare m_comp;
    };


    template<class T, class ... Args>
    void WaitReplicaTask(const ReplicaTask::RunContext& context, Args&& ... args)
    {
        T task(AZStd::forward<Args>(args) ...);
        task.Run(context);
    }
} // namespace GridMate

#endif // GM_REPLICA_TASKMANAGER_H
