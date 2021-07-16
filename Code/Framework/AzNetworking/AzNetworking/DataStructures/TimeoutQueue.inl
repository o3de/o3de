/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AzNetworking
{
    inline TimeoutQueue::TimeoutItem::TimeoutItem(uint64_t userData, AZ::TimeMs timeoutMs)
        : m_userData(userData)
        , m_timeoutMs(timeoutMs)
        , m_nextTimeoutTimeMs(AZ::GetElapsedTimeMs() + timeoutMs)
    {
        ;
    }

    inline void TimeoutQueue::TimeoutItem::UpdateTimeoutTime(AZ::TimeMs currentTimeMs)
    {
        m_nextTimeoutTimeMs = currentTimeMs + m_timeoutMs;
    }

    inline TimeoutQueue::TimeoutQueueItem::TimeoutQueueItem(TimeoutId timeoutId, AZ::TimeMs timeoutTimeMs)
        : m_timeoutId(timeoutId)
        , m_timeoutTimeMs(timeoutTimeMs)
    {
        ;
    }

    inline bool TimeoutQueue::TimeoutQueueItem::operator <(const TimeoutQueueItem& rhs) const
    {
        return rhs.m_timeoutTimeMs < m_timeoutTimeMs;
    }
}
