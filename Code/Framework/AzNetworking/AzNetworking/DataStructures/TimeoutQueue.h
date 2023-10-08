/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Time/ITime.h>
#include <AzCore/RTTI/TypeSafeIntegral.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/queue.h>

namespace AzNetworking
{
    AZ_TYPE_SAFE_INTEGRAL(TimeoutId, uint32_t);

    enum class TimeoutResult
    {
        Refresh,
        Delete
    };

    //! @class TimeoutQueue
    //! @brief class for managing timeout items.
    class TimeoutQueue
    {
    public:

        struct TimeoutItem
        {
            TimeoutItem() = default;
            TimeoutItem(uint64_t userData, AZ::TimeMs timeoutMs);

            void UpdateTimeoutTime(AZ::TimeMs currentTimeMs);

            uint64_t m_userData = 0;
            AZ::TimeMs m_timeoutMs = AZ::Time::ZeroTimeMs;
            AZ::TimeMs m_nextTimeoutTimeMs = AZ::Time::ZeroTimeMs;
        };

        TimeoutQueue() = default;
        ~TimeoutQueue() = default;

        //! Resets all internal state for this timeout queue.
        void Reset();

        //! Registers a new item with the TimeoutQueue.
        //! @param userData  value to register a timeout callback for
        //! @param timeoutMs number of milliseconds to trigger the callback after
        //! @return boolean true if registration was successful
        TimeoutId RegisterItem(uint64_t userData, AZ::TimeMs timeoutMs);

        //! Returns the provided timeout item if it exists, also refreshes the timeout value.
        //! @param timeoutId the identifier of the item to fetch
        //! @return pointer to the timeout item if it exists
        TimeoutItem* RetrieveItem(TimeoutId timeoutId);

        //! Removes an item from the TimeoutQueue.
        //! @param timeoutId the identifier of the item to remove
        void RemoveItem(TimeoutId timeoutId);

        //! Updates timeouts for all items, invokes the provided timeout functor if required.
        //! @param timeoutHandler lambda to invoke for all timeouts
        //! @param maxTimeouts    the maximum number of timeouts to process before breaking iteration
        using TimeoutHandler = AZStd::function<TimeoutResult(TimeoutQueue::TimeoutItem&)>;
        void UpdateTimeouts(const TimeoutHandler& timeoutHandler, int32_t maxTimeouts = -1);

    private:

        struct TimeoutQueueItem
        {
            TimeoutQueueItem(TimeoutId timeoutId, AZ::TimeMs timeoutTimeMs);

            bool operator < (const TimeoutQueueItem &rhs) const;

            TimeoutId m_timeoutId;
            AZ::TimeMs m_timeoutTimeMs;
        };

        using TimeoutItemMap   = AZStd::map<TimeoutId, TimeoutItem>;
        using TimeoutItemQueue = AZStd::priority_queue<TimeoutQueueItem>;

        TimeoutId        m_nextTimeoutId = TimeoutId{ 0 };
        TimeoutItemMap   m_timeoutItemMap;
        TimeoutItemQueue m_timeoutItemQueue;

        // TimeoutQueue is a shared resource among connections. A mutex (or a read-write sync object) is required with multi-threaded sends.
        // See @sv_multithreadedConnectionUpdates cvar.
        AZStd::recursive_mutex m_mutex;
    };
}

#include <AzNetworking/DataStructures/TimeoutQueue.inl>
