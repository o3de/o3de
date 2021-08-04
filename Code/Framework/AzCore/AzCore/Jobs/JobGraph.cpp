/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Jobs/JobGraph.h>

#include <AzCore/Jobs/JobExecutor.h>

namespace AZ
{
    using Internal::CompiledJobGraph;

    void JobToken::PrecedesInternal(JobToken& comesAfter)
    {
        AZ_Assert(!m_parent.m_submitted, "Cannot mutate a JobGraph that was previously submitted.");

        // Increment inbound/outbound edge counts
        m_parent.m_jobs[m_index].Link(m_parent.m_jobs[comesAfter.m_index]);

        m_parent.m_links[m_index].emplace_back(comesAfter.m_index);

        ++m_parent.m_linkCount;
    }

    JobGraph::~JobGraph()
    {
        if (m_retained && m_compiledJobGraph)
        {
            // This job graph has already finished and we are potentially responsible for its destruction
            if (m_compiledJobGraph->Release() == 0)
            {
                azdestroy(m_compiledJobGraph);
            }
        }
    }

    void JobGraph::Reset()
    {
        AZ_Assert(!m_submitted, "Cannot reset a job graph while it is in flight");
        if (m_compiledJobGraph)
        {
            azdestroy(m_compiledJobGraph);
            m_compiledJobGraph = nullptr;
        }
        m_jobs.clear();
        m_links.clear();
        m_linkCount = 0;
    }

    void JobGraph::Submit(JobGraphEvent* waitEvent)
    {
        SubmitOnExecutor(JobExecutor::Instance(), waitEvent);
    }

    void JobGraph::SubmitOnExecutor(JobExecutor& executor, JobGraphEvent* waitEvent)
    {
        if (!m_compiledJobGraph)
        {
            m_compiledJobGraph = aznew CompiledJobGraph(AZStd::move(m_jobs), m_links, m_linkCount, m_retained ? this : nullptr);
        }

        m_compiledJobGraph->m_waitEvent = waitEvent;
        m_compiledJobGraph->m_remaining = m_compiledJobGraph->m_jobs.size() + (m_retained ? 1 : 0);
        for (size_t i = 0; i != m_compiledJobGraph->m_jobs.size(); ++i)
        {
            m_compiledJobGraph->m_jobs[i].Init();
        }

        executor.Submit(*m_compiledJobGraph);

        if (m_retained)
        {
            m_submitted = true;
        }
        else
        {
            m_compiledJobGraph = nullptr;
            Reset();
        }
    }
}
