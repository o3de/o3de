/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#ifndef AZCORE_JOBS_JOBMULTIPLEDEPENDENT_H
#define AZCORE_JOBS_JOBMULTIPLEDEPENDENT_H 1

#include <AzCore/Jobs/job.h>
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    /**
     * A job which maintains a list of dependent jobs, for the situations where having a single dependent or
     * forking new jobs is not sufficient. Note that these situations should be rare, for example a many-to-many
     * dependency tree. This class may be used stand-alone or derived from, if it is derived from make sure the
     * derived Process function calls MultipleDependentJob::Process when it finishes.
     */
    class MultipleDependentJob
        : public Job
    {
    public:
        AZ_CLASS_ALLOCATOR(MultipleDependentJob, ThreadPoolAllocator, 0)

        MultipleDependentJob(bool isAutoDelete, JobContext* context = NULL)
            : Job(isAutoDelete, context)  { }

        /**
         * Adds a new job which is dependent on this one, the dependent will not be allowed to run until this job is
         * complete.
         */
        void AddDependent(Job* dependent);

        /**
         * Resets a non-auto deleting job. The dependent list will not be cleared, and will have their dependency
         * counters incremented, so they must be reset first before resetting this job.
         */
        virtual void Reset(bool isClearDependent);

        /**
         * Make sure to call this at the end of a derived Process function!
         */
        virtual void Process();

    private:
        typedef AZStd::vector<Job*> DependentList;
        DependentList m_dependents; //jobs which are dependent on us, and will be notified when we complete
    };

    //============================================================================================================
    //============================================================================================================
    //============================================================================================================

    inline void MultipleDependentJob::AddDependent(Job* dependent)
    {
        AZ_Assert(!IsStarted(), ("Dependent can only be set before the jobs are started"));
        AZ_Assert(dependent->GetDependentCount() > 0, ("Dependant jobs is always in process"));
        dependent->IncrementDependentCount();
        m_dependents.push_back(dependent);
    }

    inline void MultipleDependentJob::Reset(bool isClearDependent)
    {
        Job::Reset(isClearDependent);
        if (isClearDependent)
        {
            m_dependents.clear();
        }
        else
        {
            for (DependentList::iterator it = m_dependents.begin(); it != m_dependents.end(); ++it)
            {
                Job* dependent = *it;
                AZ_Assert(dependent->m_state == STATE_SETUP, ("Dependent must be in setup state before it can be re-initialized"));
                dependent->IncrementDependentCount();
            }
        }
    }

    inline MultipleDependentJob::Process()
    {
        //notify all dependents
        for (DependentList::iterator it = m_dependents.begin(); it != m_dependents.end(); ++it)
        {
            Job* dependent = *it;
            dependent->DecrementDependentCount();
        }
    }
}

#endif
#pragma once