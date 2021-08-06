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
        m_compiledTaskGraph->m_remaining = m_compiledTaskGraph->m_tasks.size() + (m_retained ? 1 : 0);
        for (size_t i = 0; i != m_compiledTaskGraph->m_tasks.size(); ++i)
        {
            m_compiledTaskGraph->m_tasks[i].Init();
        }

        executor.Submit(*m_compiledTaskGraph);

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
