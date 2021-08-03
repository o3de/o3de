/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AZ
{
    inline JobToken::JobToken(JobGraph& parent, size_t index)
        : m_parent{ parent }
        , m_index{ index }
    {
    }

    template<typename... JT>
    inline void JobToken::Precedes(JT&... tokens)
    {
        (PrecedesInternal(tokens), ...);
    }

    inline bool JobGraphEvent::IsSignaled()
    {
        AZ_Assert(m_submitted, "Querying the status of a job graph event that was never submitted along with the jobgraph");
        return m_semaphore.try_acquire_for(AZStd::chrono::milliseconds{ 0 });
    }

    inline void JobGraphEvent::Wait()
    {
        AZ_Assert(m_submitted, "Waiting on a job graph event that was never submitted along with the jobgraph");
        m_semaphore.acquire();
    }

    inline void JobGraphEvent::Signal()
    {
        m_semaphore.release();
    }

    template<typename Lambda>
    inline JobToken JobGraph::AddJob(JobDescriptor const& desc, Lambda&& lambda)
    {
        AZ_Assert(!m_submitted, "Cannot mutate a JobGraph that was previously submitted.");

        m_jobs.emplace_back(desc, AZStd::forward<Lambda>(lambda));

        return { *this, m_jobs.size() - 1 };
    }

    template <typename... Lambdas>
    inline AZStd::fixed_vector<JobToken, sizeof...(Lambdas)>  AddJobs(JobDescriptor const& descriptor, Lambdas&&... lambdas)
    {
        return { AddJob(descriptor, lambdas)... };
    }

    inline void JobGraph::Detach()
    {
        m_retained = false;
    }
} // namespace AZ
