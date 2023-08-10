/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/AsyncWorkQueue.h>

#include <AzCore/Debug/Profiler.h>

AZ_DECLARE_BUDGET(RHI);

namespace AZ::RHI
{
    AsyncWorkQueue::~AsyncWorkQueue()
    {
        ShutDown();
    }

    void AsyncWorkQueue::Init()
    {
        m_isQuitting = false;
        m_workItemIndex = 0;
        m_lastCompletedWorkItem = AsyncWorkHandle::Null;
        AZStd::thread_desc threadDesc{ "AsyncWorkQueue" };
        m_thread = AZStd::thread(threadDesc, [&]() { ProcessQueue(); });
        m_isInitialized = true;
    }

    void AsyncWorkQueue::ShutDown()
    {
        if (!m_isInitialized)
        {
            return;
        }

        m_isQuitting = true;
        m_workQueueCondition.notify_all();
        if (m_thread.joinable())
        {
            m_thread.join();
        }

        m_thread = AZStd::thread();
        m_waitWorkItemCondition.notify_all();
        m_isInitialized = false;
    }
        
    AsyncWorkHandle AsyncWorkQueue::CreateAsyncWork(WorkFunc&& work)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_workQueueMutex);
        // Generate a new index for the new work item.
        m_workQueue.emplace_back(WorkItem{ AsyncWorkHandle(m_workItemIndex++), AZStd::move(work) });
        return m_workQueue.back().m_handle;
    }
    
    void AsyncWorkQueue::UnlockAsyncWorkQueue()
    {
        m_workQueueCondition.notify_all();
    }

    void AsyncWorkQueue::CancelWork(const AsyncWorkHandle& workHandle)
    {
        {
            // Check if the work item is waiting for execution.
            AZStd::lock_guard<AZStd::mutex> lock(m_workQueueMutex);
            auto findIter = AZStd::find_if(m_workQueue.begin(), m_workQueue.end(), [&workHandle](const WorkItem& item)
            {
                return item.m_handle == workHandle;
            });

            if (findIter != m_workQueue.end())
            {
                m_workQueue.erase(findIter);
                return;
            }
        }

        // Wait until it finishes if it's running.
        WaitToFinish(workHandle);
    }

    void AsyncWorkQueue::ProcessQueue()
    {
        WorkItem workItem;
        for (;;)
        {
            {
                AZStd::unique_lock<AZStd::mutex> lock(m_workQueueMutex);

                if (m_workQueue.empty())
                {
                    m_workQueueCondition.wait(lock, [&]() {return !m_workQueue.empty() || m_isQuitting; });
                }

                if (m_isQuitting)
                {
                    return;
                }
                    
                workItem = AZStd::move(m_workQueue.front());
                m_workQueue.pop_front();
            }

            workItem.m_func();
            {
                AZStd::unique_lock<AZStd::mutex> lock(m_waitWorkItemMutex);
                m_lastCompletedWorkItem = workItem.m_handle;
            }
            m_waitWorkItemCondition.notify_all();
        }
    }

    bool AsyncWorkQueue::HasFinishedWork(const AsyncWorkHandle& workHandle) const
    {
        // Since the work items have a non monotonic index and we don't reorder the work item
        // queue, we can just compare the index of the last completed work item.
        return !m_lastCompletedWorkItem.IsNull() && workHandle <= m_lastCompletedWorkItem;
    }

    void AsyncWorkQueue::WaitToFinish(const AsyncWorkHandle workHandle) const
    {
        if (workHandle.IsNull())
        {
            return;
        }

        AZ_PROFILE_SCOPE(RHI, "AsyncWorkQueue: WaitToFinish");

        AZStd::unique_lock<AZStd::mutex> lock(m_waitWorkItemMutex);
        m_waitWorkItemCondition.wait(lock, [&]() {return HasFinishedWork(workHandle); });
    }
}
