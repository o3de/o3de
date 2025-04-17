/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Handle.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/conditional_variable.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/containers/deque.h>

namespace AZ::RHI
{
    class AsyncWorkQueue;
    //! Handle for a work item in an AsyncWorkQueue
    using AsyncWorkHandle = RHI::Handle<uint64_t, AsyncWorkQueue>;

    //! Helper class to manage executing work in a background thread.
    //! Work items are processed in the order that they were received.
    class AsyncWorkQueue
    {
    public:
        AZ_CLASS_ALLOCATOR(AsyncWorkQueue, AZ::SystemAllocator);

        AZ_DISABLE_COPY_MOVE(AsyncWorkQueue);            

        AsyncWorkQueue() = default;
        ~AsyncWorkQueue();

        void Init();
        void ShutDown();

        using WorkFunc = AZStd::function<void()>;
            
        //! Creates and queue new work into m_workQueue.
        //! @return Returns a handle that can be use to cancel or wait for the work to finish.
        AsyncWorkHandle CreateAsyncWork(WorkFunc&& work);
            
        //! Unlocks the async work queue in order  to start processing work
        void UnlockAsyncWorkQueue();
            
        //! Cancel a previously queued work item if it hasn't run. If the work item already
        //! finished, then this function does nothing. If the work item is in progress, it
        //! waits until it finishes.
        void CancelWork(const AsyncWorkHandle& workHandle);

        //! Blocks until a previously submitted work has finished.
        void WaitToFinish(const AsyncWorkHandle workHandle) const;

    private:
        struct WorkItem
        {
            AsyncWorkHandle m_handle;
            WorkFunc m_func;
        };

        void ProcessQueue();
        bool HasFinishedWork(const AsyncWorkHandle& workHandle) const;

        AZStd::thread m_thread;
        AZStd::atomic_bool m_isQuitting = { false };
        AZStd::mutex m_workQueueMutex;
        AZStd::deque<WorkItem> m_workQueue;
        AZStd::condition_variable m_workQueueCondition;
        // Non monotonic index that is used for new work items.
        uint64_t m_workItemIndex = 0;
        bool m_isInitialized = false;

        AsyncWorkHandle m_lastCompletedWorkItem;
        mutable AZStd::mutex m_waitWorkItemMutex;
        mutable AZStd::condition_variable m_waitWorkItemCondition;
    };
}
