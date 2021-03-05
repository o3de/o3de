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
