/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZCORE_JOBS_JOB_H
#define AZCORE_JOBS_JOB_H 1

#include <AzCore/base.h>
#include <AzCore/Jobs/JobCancelGroup.h>
#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Jobs/JobManager.h>
#include <AzCore/std/parallel/atomic.h>

#include <AzCore/Memory/PoolAllocator.h>

#if defined(_DEBUG)
#   define AZ_DEBUG_JOB_STATE
#endif // AZ_DEBUG_BUILD

namespace AZ
{
    namespace Internal
    {
        class JobManagerBase;
    }

    /**
     * Job class, representing a small unit of processing which can be parallelized with other jobs. This is the generic
     * class for jobs, you will need to inherit from it and implement the Process function. In addition you can use a
     * provided templates for a generic function JobFunction an EmptyJob (used for tracking) or JobCompletion/JobCompletionSpin
     * used to wait for all dependent jobs to finish.
     * Jobs by default will auto-delete when they are complete, which
     * simplifies typical fork-join usage, and is not inefficient since Jobs have a custom pool allocator.
     *
     * Dependency notes: Each job has at most one dependent job, which will be notified when this job completes.
     * This is not as limiting as it may seem at first, since jobs can fork additional jobs from their Process function,
     * effectively starting dependent jobs. Also jobs can manipulate the dependent count directly, allowing derived
     * jobs to manage their dependents directly. See MultipleDependentJob for an example of this.
     */
    class Job
    {
    public:
        enum State
        {
            STATE_SETUP,      //job is currently being created and initialized, is not yet ready for processing
            STATE_STARTED,    //job has been started, should not be touched anymore as could be processed at any time
            STATE_PENDING,    //job dependency counter has reached zero, job is passed to JobManager for processing
            STATE_PROCESSING, //job is currently being processed or already processed.
            STATE_SUSPENDED,  //job is suspended on the stack, will be resumed again once count reaches 0
        };

        /**
         * If a JobContext is not specified, then the currently processing job's context will be
         * used, or the global context from JobContext::SetGlobalContext will be used if this is a top-level job.
         * isAutoDelete if true will call delete on the job after it's complete.
         * isCompletion will allow the job to run when the dependent count is zero without being scheduled.
         * priority is used to sort jobs such that higher priority jobs are run before lower priority ones.
         *          The valid range is -128 (lowest priority) to 127 (highest priority), the default is 0,
         *          and jobs with equal priority values will be run in the same order as added to the queue.
         */
        Job(bool isAutoDelete, JobContext* context, bool isCompletion = false, AZ::s8 priority = 0);

        virtual ~Job() { }

        /**
         * Allows the job to start executing. Auto-delete jobs should not be accessed after calling this function.
         */
        void Start();

        /**
         * Resets a non-auto-deleting job so it can be used again. If the dependent is not cleared, then it
         * should be already in the reset state, in order to increment the dependent count.
         */
        virtual void Reset(bool isClearDependent);

        /**
         * Sets the dependent job. The dependent job will not not be allowed to run until this job is complete. Both
         * jobs should be in the un-started state when calling this function.
         */
        void SetDependent(Job* dependent);

        /**
         * Alternative version of SetDependent, for experts only. Almost identical except it does not require the
         * dependent to be in the un-started state. Use the regular SetDependent wherever possible, this function is
         * tricky to use correctly, and not all errors can be caught by assertions. It is the users responsibility to
         * ensure that the dependent has not already been run, e.g. if this is called from a job on which the dependent
         * job is waiting.
         */
        void SetDependentStarted(Job* dependent);

        /**
         * A continuation job is a job which will follow this job, and continues its work, in the sense that this jobs
         * dependent will not run until the continuation job is also complete. It can only be called while this job is
         * currently processing, otherwise a regular dependent should be used.
         *
         * A common usage for continuation jobs is when forking new jobs from a processing job. The forked jobs (or
         * the join job if present) will be set as continuations of the current job, which ensures that the dependent
         * set by the caller of this job will not run until the forked jobs have completed.
         *
         * The continuation is implemented by setting the dependent of the continuation job to be the dependent of this
         * job, i.e. this function is equivalent to calling:
         * continuationJob->SetDependentStarted(this->GetDependent()), with some extra asserts for safety.
         */
        void SetContinuation(Job* continuationJob);

        /**
         * This starts the specified job and adds it as a child to this job. This job must be currently processing
         * when this function is called. Before processing is finished, this job must call WaitForChildren to block
         * until all child jobs are complete.
         */
        void StartAsChild(Job* childJob);

        /**
         * Suspends processing of this job until all children are complete. The thread currently running the job will
         * resume running other jobs until the children are complete.
         */
        void WaitForChildren();

        /**
         * Checks if this job has been canceled. You can poll this during processing if you like, to return early.
         */
        bool IsCancelled() const;

        /**
         * Checks if this job will automatically be deleted when it has finished processing, this was specified when
         * job was created.
         */
        bool IsAutoDelete() const;

        /*
         * Check if the job is a completion job.
         */
        bool IsCompletion() const;

        /**
         * Starts this job, and then uses this thread to assist with job processing until it is complete. This can
         * only be called on a non-worker thread. Please think twice when using this function, it is not guaranteed to
         * be an automatic performance boost, as many factors come into play.
         */
        void StartAndAssistUntilComplete();

        /**
         * If we are in a worker thread this will start the job as a child job and wait for children to complete. If
         * we are in a non-worker this will use a StartAndAssistUntilComplete to block until the job completes. The job must not
         * have a dependent when this function is called.
         */
        void StartAndWaitForCompletion();

        /**
         * Gets the context to which this job belongs.
         */
        JobContext* GetContext() const;

        /**
         * Gets the dependent job, the dependent job will not start until this job has completed.
         */
        Job* GetDependent() const;

        /**
         * Dependency counter functions, these should not usually be used, unless you know what you're doing.
         */
        /*@{*/
        unsigned int GetDependentCount() const;
        void IncrementDependentCount();
        void DecrementDependentCount();
        /*@}*/

        /*
         * Get the priority of this job.
         */
        AZ::s8 GetPriority() const;

#ifdef AZ_DEBUG_JOB_STATE
        int GetState() const    { return m_state; }
#endif // AZ_DEBUG_JOB_STATE

    protected:

        /// Override to implement your processing.
        virtual void Process() = 0;

#ifdef AZ_DEBUG_JOB_STATE
        void SetState(int state);
#endif // AZ_ENABLE_TRACING

        void StoreDependent(Job* job);
        void SetDependentChild(Job* dependent);
        void IncrementDependentCountAndSetChildFlag();
        void SetDependentCountAndFlags(unsigned int countAndFlags);
        unsigned int GetDependentCountAndFlags() const;

        friend class Internal::JobManagerBase;

    private:
        //non-copyable
        Job(const Job&);
        Job& operator=(const Job&);

        enum
        {
            //flags
            FLAG_AUTO_DELETE = (1 << 31),
            FLAG_CHILD_JOBS  = (1 << 30),
            FLAG_COMPLETION  = (1 << 29), // Completion runs in place when it's dependency count is zero (no need to be scheduled)
            FLAG_RESERVED    = (1 << 28), // Reserved, can be used in the future if needed

            //8 bits for priority
            FLAG_PRIORITY_MASK = 0x0ff00000,
            FLAG_PRIORITY_START_BIT = 20,

            //20 bits for count
            FLAG_DEPENDENTCOUNT_MASK = 0x000fffff
        };

    protected:
        //16 bytes of overhead per job, including the vtable pointer (20 bytes in debug)

        JobContext* volatile m_context;

        //storing dependent count together with some other values, to save space. Dependent count is stored in lowest
        //bits to allow us to atomically increment/decrement it.
#ifdef AZCORE_JOBS_IMPL_SYNCHRONOUS
        unsigned int m_dependentCountAndFlags; //number of jobs we're currently waiting on
        Job* m_dependent; //job which is dependent on us, and will be notified when we complete
#else
        AZStd::atomic<unsigned int> m_dependentCountAndFlags; //number of jobs we're currently waiting on
        AZStd::atomic<Job*> m_dependent; //job which is dependent on us, and will be notified when we complete
#endif

        //state is only really necessary for debugging... we could squeeze it into the dependent count member, but it
        //would require atomic ops to set/read it, so not really worth it.
        int m_state;
    };

    //============================================================================================================
    //============================================================================================================
    //============================================================================================================

    inline Job::Job(bool isAutoDelete, JobContext* context, bool isCompletion, AZ::s8 priority)
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
        StoreDependent(NULL);

#ifdef AZ_DEBUG_JOB_STATE
        SetState(STATE_SETUP);
#endif // AZ_DEBUG_JOB_STATE
    }

    AZ_FORCE_INLINE void Job::Start()
    {
        //jobs are created with a count set to 1, we remove that count to allow the job to start
#ifdef AZ_DEBUG_JOB_STATE
        AZ_Assert(m_state == STATE_SETUP, ("Jobs must be in the setup state before they can be started"));
        SetState(STATE_STARTED);
#endif
        DecrementDependentCount();
    }

    inline void Job::Reset(bool isClearDependent)
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
            StoreDependent(NULL);
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

    AZ_FORCE_INLINE void Job::SetDependent(Job* dependent)
    {
        AZ_Assert(!GetDependent(), ("Job already has a dependent, should be cleared after the job is done"));
#ifdef AZ_DEBUG_JOB_STATE
        AZ_Assert(m_state == STATE_SETUP, ("Dependent can only be set before the jobs are started"));
        AZ_Assert(dependent->m_state == STATE_SETUP, ("Dependent must be in the setup state"));
#endif
        dependent->IncrementDependentCount();
        StoreDependent(dependent);
    }

    AZ_FORCE_INLINE void Job::SetDependentStarted(Job* dependent)
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

    AZ_FORCE_INLINE void Job::SetDependentChild(Job* dependent)
    {
        AZ_Assert(!GetDependent(), ("Job already has a dependent, should be cleared after the job is done"));
#ifdef AZ_DEBUG_JOB_STATE
        AZ_Assert(m_state == STATE_SETUP, ("Dependent can only be set before the jobs are started"));
        AZ_Assert(dependent->m_state == STATE_PROCESSING, "Dependent must be processing to add a child");
#endif
        dependent->IncrementDependentCountAndSetChildFlag();
        StoreDependent(dependent);
    }

    AZ_FORCE_INLINE void Job::SetContinuation(Job* continuationJob)
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

    AZ_FORCE_INLINE void Job::StartAsChild(Job* childJob)
    {
#ifdef AZ_DEBUG_JOB_STATE
        AZ_Assert(m_state == STATE_PROCESSING, "Child jobs can only be added while we are processing");
#endif
        childJob->SetDependentChild(this);
        childJob->Start();
    }

    AZ_FORCE_INLINE void Job::WaitForChildren()
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

    AZ_FORCE_INLINE bool Job::IsCancelled() const
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

    AZ_FORCE_INLINE bool Job::IsAutoDelete() const
    {
        return (GetDependentCountAndFlags() & (unsigned int)FLAG_AUTO_DELETE) ? true : false;
    }

    AZ_FORCE_INLINE bool Job::IsCompletion() const
    {
        return (GetDependentCountAndFlags() & (unsigned int)FLAG_COMPLETION) ? true : false;
    }

    AZ_FORCE_INLINE void Job::StartAndAssistUntilComplete()
    {
        m_context->GetJobManager().StartJobAndAssistUntilComplete(this);
    }

    inline void Job::StartAndWaitForCompletion()
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

    AZ_FORCE_INLINE JobContext* Job::GetContext() const
    {
        return m_context;
    }

    AZ_FORCE_INLINE unsigned int Job::GetDependentCount() const
    {
        return (GetDependentCountAndFlags() & FLAG_DEPENDENTCOUNT_MASK);
    }

    AZ_FORCE_INLINE void Job::IncrementDependentCount()
    {
        AZ_Assert(GetDependentCount() < FLAG_DEPENDENTCOUNT_MASK, "Dependent count overflow");
#ifdef AZCORE_JOBS_IMPL_SYNCHRONOUS
        ++m_dependentCountAndFlags;
#else
        m_dependentCountAndFlags.fetch_add(1, AZStd::memory_order_acq_rel);
#endif
    }

    inline void Job::IncrementDependentCountAndSetChildFlag()
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

    inline void Job::DecrementDependentCount()
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

    inline AZ::s8 Job::GetPriority() const
    {
        return (GetDependentCountAndFlags() >> FLAG_PRIORITY_START_BIT) & 0xff;
    }

#ifdef AZ_DEBUG_JOB_STATE
    AZ_FORCE_INLINE void Job::SetState(int state)
    {
        m_state = state;
    }
#endif

#ifdef AZCORE_JOBS_IMPL_SYNCHRONOUS
    AZ_FORCE_INLINE void Job::StoreDependent(Job* job)
    {
        m_dependent = job;
    }

    AZ_FORCE_INLINE Job* Job::GetDependent() const
    {
        return m_dependent;
    }

    AZ_FORCE_INLINE void Job::SetDependentCountAndFlags(unsigned int countAndFlags)
    {
        m_dependentCountAndFlags = countAndFlags;
    }

    AZ_FORCE_INLINE unsigned int Job::GetDependentCountAndFlags() const
    {
        return m_dependentCountAndFlags;
    }
#else
    AZ_FORCE_INLINE void Job::StoreDependent(Job* job)
    {
        m_dependent.store(job, AZStd::memory_order_release);
    }

    AZ_FORCE_INLINE Job* Job::GetDependent() const
    {
        return m_dependent.load(AZStd::memory_order_acquire);
    }

    AZ_FORCE_INLINE void Job::SetDependentCountAndFlags(unsigned int countAndFlags)
    {
        m_dependentCountAndFlags.store(countAndFlags, AZStd::memory_order_release);
    }

    AZ_FORCE_INLINE unsigned int Job::GetDependentCountAndFlags() const
    {
        return m_dependentCountAndFlags.load(AZStd::memory_order_acquire);
    }
#endif
}

#endif
#pragma once
