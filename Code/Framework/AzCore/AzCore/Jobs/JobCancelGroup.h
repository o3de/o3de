/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZCORE_JOBS_JOBCANCELGROUP_H
#define AZCORE_JOBS_JOBCANCELGROUP_H 1

#include <AzCore/std/parallel/atomic.h>
#include <AzCore/Memory/PoolAllocator.h>

namespace AZ
{
    /**
     * Allows jobs to be cancelled. A cancelled job will not do any processing, but otherwise behaves normally, i.e.
     * notifying dependents when it 'completes'. The JobCancelGroup a Job belongs to can be specified when the Job is
     * created. JobCancelGroups can be arranged in a tree structure, then cancelling a parent group will also cancel
     * all the child groups.
     *
     * JobCancelGroups are not deleted by the system, the user must manage their lifetimes. They must not be deleted
     * until their associated jobs have completed.
     *
     * Note: These groups are needed because the existing dependency tree goes the wrong direction for cancellation.
     * We need the ability to cancel an entire tree of work at once, this could be done by cancelling the 'join' job,
     * but then the 'fork' jobs would need to check their entire dependency chain to see if they themselves should be
     * cancelled.
     */
    class JobCancelGroup
    {
    public:
        AZ_CLASS_ALLOCATOR(JobCancelGroup, ThreadPoolAllocator);

        /**
         * Creates a group with no parent.
         */
        JobCancelGroup();

        /**
         * Adds this group as a child of the specified parent group, or with no parent if parentGroup is NULL.
         */
        JobCancelGroup(JobCancelGroup* parentGroup);

        /**
         * Cancels all jobs in this group, and recursively cancels all child groups.
         */
        void Cancel();

        bool IsCancelled() const;

        /**
         * Resets the group to the un-cancelled state, should only really be used when the jobs are not running.
         */
        void Reset();

    private:
        //non-copyable
        JobCancelGroup(const JobCancelGroup&);
        JobCancelGroup& operator=(const JobCancelGroup&);

        JobCancelGroup* m_firstChildGroup;
        JobCancelGroup* m_siblingGroup;
#ifdef AZCORE_JOBS_IMPL_SYNCHRONOUS
        bool m_isCancelled;
#else
        AZStd::atomic<bool> m_isCancelled;
#endif
    };

    //=============================================================================================================
    //=============================================================================================================
    //=============================================================================================================

    inline JobCancelGroup::JobCancelGroup()
        : m_firstChildGroup(NULL)
        , m_siblingGroup(NULL)
        , m_isCancelled(false)
    {
    }

    inline JobCancelGroup::JobCancelGroup(JobCancelGroup* parentGroup)
        : m_firstChildGroup(NULL)
        , m_isCancelled(false)
    {
        if (parentGroup)
        {
            m_siblingGroup = parentGroup->m_firstChildGroup;
            parentGroup->m_firstChildGroup = this;
        }
        else
        {
            m_siblingGroup = NULL;
        }
    }

    inline void JobCancelGroup::Cancel()
    {
#ifdef AZCORE_JOBS_IMPL_SYNCHRONOUS
        m_isCancelled = true;
#else
        m_isCancelled.store(true, AZStd::memory_order_release);
#endif

        //cancel child groups
        JobCancelGroup* child = m_firstChildGroup;
        while (child)
        {
            child->Cancel();
            child = child->m_siblingGroup;
        }
    }

#ifdef AZCORE_JOBS_IMPL_SYNCHRONOUS
    inline bool JobCancelGroup::IsCancelled() const
    {
        return m_isCancelled;
    }

    inline void JobCancelGroup::Reset()
    {
        m_isCancelled = false;
    }
#else
    inline bool JobCancelGroup::IsCancelled() const
    {
        return m_isCancelled.load(AZStd::memory_order_acquire);
    }

    inline void JobCancelGroup::Reset()
    {
        m_isCancelled.store(false, AZStd::memory_order_release);
    }
#endif
}

#endif
#pragma once
