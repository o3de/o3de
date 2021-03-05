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
#pragma once

#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/containers/list.h>

namespace Vegetation
{
    /**
      * Manages a light weight producer consumer storage container
      */
    template <typename TItem, typename QueueType = AZStd::list<TItem>>
    class ConcurrentQueue final
    {
    public:
        AZ_INLINE QueueType& ClaimQueue()
        {
            int lastQueue = Flip();
            return m_queueData[lastQueue];
        }

        AZ_INLINE QueueType& ClaimQueueNoSort()
        {
            int lastQueue = FlipNoSort();
            return m_queueData[lastQueue];
        }

        AZ_INLINE bool IsCurrentEmpty() const
        {
            return m_queueData[m_currentQueueIndex].empty();
        }

        AZ_INLINE void EmplaceBack(TItem item)
        {
            m_queueData[m_currentQueueIndex].emplace_back(AZStd::move(item));
        }

        AZ_INLINE void CopyBack(TItem item)
        {
            m_queueData[m_currentQueueIndex].push_back(item);
        }

        AZ_INLINE void Insert(TItem item)
        {
            m_queueData[m_currentQueueIndex].insert(item);
        }

    protected:
        AZ_INLINE int Flip()
        {
            // get rid of possible duplicates
            int processIndex = FlipNoSort();
            m_queueData[processIndex].sort();
            m_queueData[processIndex].unique();
            return processIndex;
        }

        AZ_INLINE int FlipNoSort()
        {
            int processIndex = m_currentQueueIndex;
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_queueMutex);
                m_currentQueueIndex = 1 - m_currentQueueIndex;
            }
            return processIndex;
        }

    private:
        QueueType m_queueData[2];
        AZStd::atomic_int m_currentQueueIndex{0};
        AZStd::recursive_mutex m_queueMutex;
    };
}
