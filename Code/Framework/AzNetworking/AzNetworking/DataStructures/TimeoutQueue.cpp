/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/DataStructures/TimeoutQueue.h>
#include <AzCore/Console/ILogger.h>
#include <climits>
#include <cinttypes>

namespace AzNetworking
{
    void TimeoutQueue::Reset()
    {
        m_timeoutItemMap.clear();
        m_timeoutItemQueue = TimeoutItemQueue();
        m_nextTimeoutId = TimeoutId{0};
    }

    TimeoutId TimeoutQueue::RegisterItem(uint64_t userData, AZ::TimeMs timeoutMs)
    {
        const TimeoutId timeoutId = m_nextTimeoutId;
        const AZ::TimeMs timeoutTimeMs = AZ::GetElapsedTimeMs() + timeoutMs;
        AZLOG(TimeoutQueue, "Pushing timeoutid %u with user data %" PRIu64 " to expire at time %u",
            aznumeric_cast<uint32_t>(timeoutId),
            userData,
            aznumeric_cast<uint32_t>(timeoutTimeMs)
        );

        TimeoutQueueItem queueItem(timeoutId, timeoutTimeMs);
        m_timeoutItemMap[timeoutId] = TimeoutItem(userData, timeoutMs);
        m_timeoutItemQueue.push(queueItem);
        ++m_nextTimeoutId;

        return timeoutId;
    }

    TimeoutQueue::TimeoutItem *TimeoutQueue::RetrieveItem(TimeoutId timeoutId)
    {
        TimeoutItemMap::iterator iter = m_timeoutItemMap.find(timeoutId);
        if (iter != m_timeoutItemMap.end())
        {
            return &(iter->second);
        }
        return nullptr;
    }

    void TimeoutQueue::RemoveItem(TimeoutId timeoutId)
    {
        m_timeoutItemMap.erase(timeoutId);
    }

    void TimeoutQueue::UpdateTimeouts(const TimeoutHandler& timeoutHandler, int32_t maxTimeouts)
    {
        int32_t numTimeouts = 0;
        if (maxTimeouts < 0)
        {
            maxTimeouts = INT_MAX;
        }
        AZ::TimeMs currentTimeMs = AZ::GetElapsedTimeMs();
        while (m_timeoutItemQueue.size() > 0)
        {
            const TimeoutQueueItem queueItem = m_timeoutItemQueue.top();
            const TimeoutId    itemTimeoutId = queueItem.m_timeoutId;
            const AZ::TimeMs   itemTimeoutMs = queueItem.m_timeoutTimeMs;

            // Head item has not timed out yet, we can terminate because we've run out of timed out items
            if (itemTimeoutMs >= currentTimeMs)
            {
                break;
            }

            ++numTimeouts;

            if (numTimeouts >= maxTimeouts)
            {
                AZLOG_WARN("Terminating timeout queue iteration due to hitting timeout count limit: %d", numTimeouts);
                break;
            }

            // Pop the item, we're either going to time it out or reinsert it
            m_timeoutItemQueue.pop();

            TimeoutItemMap::iterator iter = m_timeoutItemMap.find(itemTimeoutId);
            if (iter == m_timeoutItemMap.end())
            {
                // Item has already been deleted, just continue
                continue;
            }

            TimeoutItem mapItem = iter->second;

            // Check to see if the item has been refreshed since it was inserted
            if (mapItem.m_nextTimeoutTimeMs > currentTimeMs)
            {
                TimeoutQueueItem reQueueItem(itemTimeoutId, mapItem.m_nextTimeoutTimeMs);
                m_timeoutItemQueue.push(reQueueItem);
                continue;
            }

            // By this point, the item is definitely timed out
            // Invoke the timeout function to see how to proceed
            const TimeoutResult result = timeoutHandler(mapItem);

            if (result == TimeoutResult::Refresh)
            {
                mapItem.UpdateTimeoutTime(currentTimeMs);
                // Re-insert into priority queue
                TimeoutQueueItem reQueueItem(itemTimeoutId, mapItem.m_nextTimeoutTimeMs);
                m_timeoutItemQueue.push(reQueueItem);
                continue;
            }

            AZLOG(TimeoutQueue, "Popping timeoutid %u with user data %" PRIu64 ", expire time %d, current time %u",
                aznumeric_cast<uint32_t>(itemTimeoutId),
                mapItem.m_userData,
                aznumeric_cast<uint32_t>(mapItem.m_nextTimeoutTimeMs),
                aznumeric_cast<uint32_t>(currentTimeMs));
            m_timeoutItemMap.erase(itemTimeoutId);
        }
    }

    void TimeoutQueue::UpdateTimeouts(ITimeoutHandler& timeoutHandler, int32_t maxTimeouts)
    {
        TimeoutHandler handler([&timeoutHandler](TimeoutQueue::TimeoutItem& item) { return timeoutHandler.HandleTimeout(item); });
        UpdateTimeouts(handler, maxTimeouts);
    }
}
