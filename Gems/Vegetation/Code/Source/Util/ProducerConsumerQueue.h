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
#include <AzCore/std/containers/deque.h>
#include "ConcurrentQueue.h"

namespace Vegetation
{
    /**
    * A simple producer-consumer class to handle dual-threaded working queues
    */
    template <typename TItem, typename ProducerQueueType = ConcurrentQueue<TItem>, typename ConsumerQueueType = AZStd::list<TItem>>
    class ProducerConsumerQueue final
    {
    public:
        AZ_INLINE void EmplaceBack(TItem item)
        {
            m_producerQueue.EmplaceBack(AZStd::move(item));
        }

        AZ_INLINE void CopyBack(TItem item)
        {
            m_producerQueue.CopyBack(item);
        }

        AZ_INLINE bool IsEmpty() const
        {
            if (m_producerQueue.IsCurrentEmpty())
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_consumerQueueMutex);
                return m_consumerQueue.empty();
            }
            return false;
        }

        using ItemFunc = AZStd::function<bool(TItem&)>;
        using ContinueFunc = AZStd::function<bool()>;

        // on ItemFunc return TRUE, remove from consumer queue
        AZ_INLINE void Consume(ItemFunc consumeItemFunc, ContinueFunc continueFunc)
        {
            if (CanConsume())
            {
                PrepareConsumer();
            }

            // attempt to consume the items
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_consumerQueueMutex);
            auto itItem = m_consumerQueue.begin();
            while (itItem != m_consumerQueue.end())
            {
                if (consumeItemFunc(*itItem))
                {
                    itItem = m_consumerQueue.erase(itItem);
                }
                else
                {
                    ++itItem;
                }
                if (!continueFunc())
                {
                    break;
                }
            }
        }

        // on ItemFunc return TRUE, stop processing
        AZ_INLINE void Process(ItemFunc processItemFunc)
        {
            if (CanConsume())
            {
                PrepareConsumer();
            }

            // process the locked queue
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_consumerQueueMutex);
            auto itItem = m_consumerQueue.begin();
            while (itItem != m_consumerQueue.end())
            {
                if (processItemFunc(*itItem))
                {
                    break;
                }
                ++itItem;
            }
        }

    protected:
        AZ_INLINE bool CanConsume() const
        {
            return !m_producerQueue.IsCurrentEmpty();
        }

        AZ_INLINE void PrepareConsumer()
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_consumerQueueMutex);
            auto& itemList = m_producerQueue.ClaimQueueNoSort();
            while (!itemList.empty())
            {
                m_consumerQueue.emplace_back(AZStd::move(itemList.back()));
                itemList.pop_back();
            }
        }

    private:
        ProducerQueueType m_producerQueue;
        ConsumerQueueType m_consumerQueue;
        mutable AZStd::recursive_mutex m_consumerQueueMutex;
    };
}
