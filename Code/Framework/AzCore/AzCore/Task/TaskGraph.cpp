/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Task/TaskGraph.h>

#include <AzCore/Task/TaskExecutor.h>

namespace AZ
{
    using Internal::CompiledTaskGraph;

    void TaskGraphEvent::Wait()
    {
        AZ_Assert(m_executor->GetTaskWorker() == nullptr, "Waiting in a task is unsupported");
        m_semaphore.acquire();
    }

    void TaskGraphEvent::IncWaitCount()
    {
        int expectedValue = 0; // guess zero to optimize for single task graph using an event, if multiple are using it then this will take 2+ comp_exch calls
        while(!m_waitCount.compare_exchange_weak(expectedValue, expectedValue + 1))
        {
            AZ_Assert(expectedValue >= 0, "Called TaskGraphEvent::IncWaitCount on a signalled event"); // value will be negative once event is ready to signal or has been signaled. Shouldn't happen.
            if (expectedValue < 0) // event already signaled, skip
            {
                return;
            }
        };
    }

    void TaskGraphEvent::Signal()
    {
        int expectedValue = 1; // guess one to optimize for single task graph using an event, if multiple are using it then this will take 2+ comp_exch calls
        while(!m_waitCount.compare_exchange_weak(expectedValue, expectedValue - 1))
        {
            AZ_Assert(expectedValue > 0, "Called TaskGraphEvent::Signal when event is either signaled or unused"); // It's an error for Signal to be called if no one is waiting, or the event has already been signaled
            if (expectedValue < 0) // return if already signaled
            {
                return;
            }
        };

        if (expectedValue == 1) // This call decremented the value to 0.
        {
            expectedValue = 0;
            if (m_waitCount.compare_exchange_strong(expectedValue, -1)) // validate no one incremented the wait count and mark signalling state
            {
                m_semaphore.release();
            }
        }
    }

    void TaskToken::PrecedesInternal(TaskToken& comesAfter)
    {
        AZ_Assert(!m_parent.m_submitted, "Cannot mutate a TaskGraph that was previously submitted.");

        // Increment inbound/outbound edge counts
        m_parent.m_tasks[m_index].Link(m_parent.m_tasks[comesAfter.m_index]);

        m_parent.m_links[m_index].emplace_back(comesAfter.m_index);

        ++m_parent.m_linkCount;
    }

    TaskGraph::~TaskGraph()
    {
        if (m_retained && m_compiledTaskGraph)
        {
            // This job graph has already finished and we are potentially responsible for its destruction
            if (m_compiledTaskGraph->Release() == 0)
            {
                azdestroy(m_compiledTaskGraph);
            }
        }
    }

    void TaskGraph::Reset()
    {
        AZ_Assert(!m_submitted, "Cannot reset a job graph while it is in flight");
        if (m_compiledTaskGraph)
        {
            azdestroy(m_compiledTaskGraph);
            m_compiledTaskGraph = nullptr;
        }
        m_tasks.clear();
        m_links.clear();
        m_linkCount = 0;
    }

    void TaskGraph::Submit(TaskGraphEvent* waitEvent)
    {
        SubmitOnExecutor(TaskExecutor::Instance(), waitEvent);
    }

    void TaskGraph::SubmitOnExecutor(TaskExecutor& executor, TaskGraphEvent* waitEvent)
    {
        if (!m_compiledTaskGraph)
        {
            m_compiledTaskGraph = aznew CompiledTaskGraph(AZStd::move(m_tasks), m_links, m_linkCount, m_retained ? this : nullptr);
        }

        m_compiledTaskGraph->m_waitEvent = waitEvent;
        uint32_t taskCount = aznumeric_cast<uint32_t>(m_compiledTaskGraph->m_tasks.size());
        m_compiledTaskGraph->m_remaining = taskCount + (m_retained ? 1 : 0);
        for (uint32_t i = 0; i != taskCount; ++i)
        {
            m_compiledTaskGraph->m_tasks[i].Init();
        }

        executor.Submit(*m_compiledTaskGraph, waitEvent);

        if (m_retained)
        {
            m_submitted = true;
        }
        else
        {
            m_compiledTaskGraph = nullptr;
            Reset();
        }
    }
}
