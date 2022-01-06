/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/base.h>
#include <AzCore/Jobs/Job.h>
#include <AzCore/Jobs/JobCancelGroup.h>
#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Jobs/JobManager.h>
#include <AzCore/std/parallel/atomic.h>

namespace AZ
{
    Job::Job(bool isAutoDelete, AZ::JobContext* context, bool isCompletion, AZ::s8 priority)
    {
        if (context)
        {
            m_context = context;
        }
        else
        {
            m_context = JobContext::GetParentContext();
        }

        unsigned int countAndFlags = 1;
        if (isAutoDelete)
        {
            countAndFlags |= (unsigned int)FLAG_AUTO_DELETE;
        }
        if (isCompletion)
        {
            countAndFlags |= (unsigned int)FLAG_COMPLETION;
        }
        countAndFlags |= (unsigned int)((priority << FLAG_PRIORITY_START_BIT) & FLAG_PRIORITY_MASK);
        SetDependentCountAndFlags(countAndFlags);
        StoreDependent(nullptr);

    #ifdef AZ_DEBUG_JOB_STATE
        SetState(STATE_SETUP);
    #endif // AZ_DEBUG_JOB_STATE
    }

    void Job::Start()
    {
        //jobs are created with a count set to 1, we remove that count to allow the job to start
    #ifdef AZ_DEBUG_JOB_STATE
        AZ_Assert(m_state == STATE_SETUP, ("Jobs must be in the setup state before they can be started"));
        SetState(STATE_STARTED);
    #endif
        DecrementDependentCount();
    }

    void Job::Reset(bool isClearDependent)
    {
    #ifdef AZ_DEBUG_JOB_STATE
        AZ_Assert((m_state == STATE_SETUP) || (m_state == STATE_PROCESSING), "Jobs must not be running when they are reset");
        SetState(STATE_SETUP);
    #endif
        unsigned int countAndFlags = GetDependentCountAndFlags();
        AZ_Assert((countAndFlags & (unsigned int)FLAG_AUTO_DELETE) == 0, "You can't call reset on AutoDelete jobs!");
        // Remove the FLAG_DEPENDENTCOUNT_MASK and FLAG_CHILD_JOBS flags
        countAndFlags = (countAndFlags & (~(FLAG_DEPENDENTCOUNT_MASK) & ~(FLAG_CHILD_JOBS))) | 1;
        SetDependentCountAndFlags(countAndFlags);
        if (isClearDependent)
        {
            StoreDependent(nullptr);
        }
        else
        {
            Job* dependent = GetDependent();
            if (dependent)
            {
    #ifdef AZ_DEBUG_JOB_STATE
                AZ_Assert(dependent->m_state == STATE_SETUP, ("Dependent must be in setup state before it can be re-initialized"));
    #endif
                dependent->IncrementDependentCount();
            }
        }
    }

    void Job::SetDependent(Job* dependent)
    {
        AZ_Assert(!GetDependent(), ("Job already has a dependent, should be cleared after the job is done"));
    #ifdef AZ_DEBUG_JOB_STATE
        AZ_Assert(m_state == STATE_SETUP, ("Dependent can only be set before the jobs are started"));
        AZ_Assert(dependent->m_state == STATE_SETUP, ("Dependent must be in the setup state"));
    #endif
        dependent->IncrementDependentCount();
        StoreDependent(dependent);
    }

    void Job::SetDependentStarted(Job* dependent)
    {
        AZ_Assert(!GetDependent(), ("Job already has a dependent, should be cleared after the job is done"));
    #ifdef AZ_DEBUG_JOB_STATE
        AZ_Assert(m_state == STATE_SETUP, ("Dependent can only be set before the jobs are started"));
        //We don't require the dependent to be in STATE_SETUP, the user can call this from a context where they
        //know the dependent has not started yet, although it is in STATE_STARTED already, e.g. if SetDependent
        //is called from a job which the dependent is already dependent on.
        //Note that if the user gets this wrong, the dependent may start before this job is finished, and the asserts
        //may not even trigger due to race conditions. Hence why this function is 'experts only'.
        AZ_Assert((dependent->m_state == STATE_SETUP) || (dependent->m_state == STATE_STARTED)
            || (dependent->m_state == STATE_SUSPENDED), "Dependent must be in the setup, started, or suspended state");
    #endif
        dependent->IncrementDependentCount();
        StoreDependent(dependent);
    }

    void Job::SetDependentChild(Job* dependent)
    {
        AZ_Assert(!GetDependent(), ("Job already has a dependent, should be cleared after the job is done"));
    #ifdef AZ_DEBUG_JOB_STATE
        AZ_Assert(m_state == STATE_SETUP, ("Dependent can only be set before the jobs are started"));
        AZ_Assert(dependent->m_state == STATE_PROCESSING, "Dependent must be processing to add a child");
    #endif
        dependent->IncrementDependentCountAndSetChildFlag();
        StoreDependent(dependent);
    }

    void Job::SetContinuation(Job* continuationJob)
    {
    #ifdef AZ_DEBUG_JOB_STATE
        AZ_Assert(m_state == STATE_PROCESSING, "Continuation jobs can only be set while we are processing, otherwise a regular dependent should be used");
    #endif
        Job* dependent = GetDependent();
        if (dependent)  //nothing to do if there is no dependent... doesn't usually happen, except with synchronous processing and assists
        {
            continuationJob->SetDependentStarted(dependent);
        }
    }

    void Job::StartAsChild(Job* childJob)
    {
    #ifdef AZ_DEBUG_JOB_STATE
        AZ_Assert(m_state == STATE_PROCESSING, "Child jobs can only be added while we are processing");
    #endif
        childJob->SetDependentChild(this);
        childJob->Start();
    }

    void Job::WaitForChildren()
    {
    #ifdef AZ_DEBUG_JOB_STATE
        AZ_Assert(m_state == STATE_PROCESSING, "We must be currently processing in order to suspend");
    #endif
        if (GetDependentCount() != 0)
        {
    #ifdef AZ_DEBUG_JOB_STATE
            SetState(STATE_SUSPENDED);
    #endif // AZ_DEBUG_JOB_STATE
            m_context->GetJobManager().SuspendJobUntilReady(this);
    #ifdef AZ_DEBUG_JOB_STATE
            SetState(STATE_PROCESSING);
    #endif // AZ_DEBUG_JOB_STATE
        }
        AZ_Assert(GetDependentCount() == 0, "Suspended job has resumed, but still has non-zero dependent count, bug in JobManager?");
    }

    bool Job::IsCancelled() const
    {
        JobCancelGroup* cancelGroup = m_context->GetCancelGroup();
        if (cancelGroup && cancelGroup->IsCancelled())
        {
            if (!IsCompletion()) // always run completion jobs, as they can be holding a synchronization primitive
            {
                return true;
            }
        }
        return false;
    }

    bool Job::IsAutoDelete() const
    {
        return (GetDependentCountAndFlags() & (unsigned int)FLAG_AUTO_DELETE) ? true : false;
    }

    bool Job::IsCompletion() const
    {
        return (GetDependentCountAndFlags() & (unsigned int)FLAG_COMPLETION) ? true : false;
    }

    void Job::StartAndAssistUntilComplete()
    {
        m_context->GetJobManager().StartJobAndAssistUntilComplete(this);
    }

    void Job::StartAndWaitForCompletion()
    {
        //check if we are in a worker thread or a general user thread
        Job* currentJob = m_context->GetJobManager().GetCurrentJob();
        if (currentJob)
        {
            //worker thread, so just suspend this current job until the empty job completes
            currentJob->StartAsChild(this);
            currentJob->WaitForChildren();
        }
        else
        {
            StartAndAssistUntilComplete();
        }
    }

    unsigned int Job::GetDependentCount() const
    {
        return (GetDependentCountAndFlags() & FLAG_DEPENDENTCOUNT_MASK);
    }

    void Job::IncrementDependentCount()
    {
        AZ_Assert(GetDependentCount() < FLAG_DEPENDENTCOUNT_MASK, "Dependent count overflow");
    #ifdef AZCORE_JOBS_IMPL_SYNCHRONOUS
        ++m_dependentCountAndFlags;
    #else
        m_dependentCountAndFlags.fetch_add(1, AZStd::memory_order_acq_rel);
    #endif
    }

    void Job::IncrementDependentCountAndSetChildFlag()
    {
        AZ_Assert(GetDependentCount() < FLAG_DEPENDENTCOUNT_MASK, "Dependent count overflow");
    #ifdef AZCORE_JOBS_IMPL_SYNCHRONOUS
        int oldCount = m_dependentCountAndFlags & FLAG_DEPENDENTCOUNT_MASK;
        m_dependentCountAndFlags = (m_dependentCountAndFlags & ~FLAG_DEPENDENTCOUNT_MASK) | (oldCount + 1) | FLAG_CHILD_JOBS;
    #else
        //use a single atomic operation to increment the count and set the child flag if possible
        unsigned int oldCountAndFlags, newCountAndFlags;
        do
        {
            oldCountAndFlags = m_dependentCountAndFlags.load(AZStd::memory_order_acquire);
            int oldCount = oldCountAndFlags & FLAG_DEPENDENTCOUNT_MASK;
            newCountAndFlags = (oldCountAndFlags & ~FLAG_DEPENDENTCOUNT_MASK) | (oldCount + 1) | FLAG_CHILD_JOBS;
        } while (!m_dependentCountAndFlags.compare_exchange_weak(oldCountAndFlags, newCountAndFlags, AZStd::memory_order_acq_rel, AZStd::memory_order_acquire));
    #endif
    }

    void Job::DecrementDependentCount()
    {
    #ifdef AZ_DEBUG_JOB_STATE
        AZ_Assert((m_state == STATE_SETUP) || (m_state == STATE_STARTED)
            || (m_state == STATE_PROCESSING) || (m_state == STATE_SUSPENDED), //child jobs
            "Job dependent count should not be decremented after job is already pending");
    #endif
        AZ_Assert(GetDependentCount() > 0, ("Job dependent count is already zero"));
    #ifdef AZCORE_JOBS_IMPL_SYNCHRONOUS
        unsigned int countAndFlags = m_dependentCountAndFlags--;
    #else
        unsigned int countAndFlags = m_dependentCountAndFlags.fetch_sub(1, AZStd::memory_order_acq_rel);
    #endif
        unsigned int count = countAndFlags & FLAG_DEPENDENTCOUNT_MASK;
        if (count == 1)
        {
            if (!(countAndFlags & FLAG_CHILD_JOBS))
            {
    #ifdef AZ_DEBUG_JOB_STATE
                AZ_Assert(m_state == STATE_STARTED, "Job has not been started but the dependent count is zero, must be a dependency error");
                SetState(STATE_PENDING);
    #endif
                m_context->GetJobManager().AddPendingJob(this);
            }
        }
    }

    AZ::s8 Job::GetPriority() const
    {
        return (GetDependentCountAndFlags() >> FLAG_PRIORITY_START_BIT) & 0xff;
    }

#ifdef AZCORE_JOBS_IMPL_SYNCHRONOUS
    void Job::StoreDependent(Job* job)
    {
        m_dependent = job;
    }

    Job* Job::GetDependent() const
    {
        return m_dependent;
    }

    void Job::SetDependentCountAndFlags(unsigned int countAndFlags)
    {
        m_dependentCountAndFlags = countAndFlags;
    }

    unsigned int Job::GetDependentCountAndFlags() const
    {
        return m_dependentCountAndFlags;
    }
#else
    void Job::StoreDependent(Job* job)
    {
        m_dependent.store(job, AZStd::memory_order_release);
    }

    Job* Job::GetDependent() const
    {
        return m_dependent.load(AZStd::memory_order_acquire);
    }

    void Job::SetDependentCountAndFlags(unsigned int countAndFlags)
    {
        m_dependentCountAndFlags.store(countAndFlags, AZStd::memory_order_release);
    }

    unsigned int Job::GetDependentCountAndFlags() const
    {
        return m_dependentCountAndFlags.load(AZStd::memory_order_acquire);
    }
#endif
}
