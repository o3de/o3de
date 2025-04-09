/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/EBus/ScheduledEvent.h>
#include <AzCore/EBus/ScheduledEventHandle.h>
#include <AzCore/EBus/EventSchedulerSystemComponent.h>

namespace AZ
{
    ScheduledEvent::ScheduledEvent(const AZStd::function<void()>& callback, const Name& eventName)
        : m_eventName(eventName)
        , m_callback(callback)
    {
        ;
    }

    ScheduledEvent::~ScheduledEvent()
    {
        RemoveFromQueue();
    }

    void ScheduledEvent::Enqueue(TimeMs durationMs, bool autoRequeue)
    {
        IEventScheduler* eventScheduler = Interface<IEventScheduler>::Get();
        if (eventScheduler)
        {
            RemoveFromQueue();
            m_durationMs = durationMs;
            m_autoRequeue = autoRequeue;
            m_handle = eventScheduler->AddEvent(this, durationMs);
        }
    }

    void ScheduledEvent::Requeue()
    {
        const TimeMs cachedDurationMs = m_durationMs;
        const TimeMs elapsedTimeMs = TimeInQueueMs();
        const TimeMs overscheduledTimeMs = (elapsedTimeMs < cachedDurationMs) ? TimeMs{ 0 } : elapsedTimeMs - cachedDurationMs;
        // Remove any unexpected time we spent in queue prior to triggering
        const TimeMs requeueDurationMs = cachedDurationMs - overscheduledTimeMs;
        Requeue(requeueDurationMs);
        // Restore the desired duration after queuing so that the next requeue will use the correct base duration
        m_durationMs = cachedDurationMs;
    }

    void ScheduledEvent::Requeue(TimeMs durationMs)
    {
        m_durationMs = durationMs;
        IEventScheduler* eventScheduler = Interface<IEventScheduler>::Get();
        if (eventScheduler)
        {
            m_handle = eventScheduler->AddEvent(this, m_durationMs);
        }
    }

    void ScheduledEvent::RemoveFromQueue()
    {
        ClearHandle();
        m_autoRequeue = false; // In the case that someone is removing an event that's auto queued inside a notify we don't want to re-queue that event.
    }

    bool ScheduledEvent::IsScheduled() const
    {
        return m_handle != nullptr;
    }

    bool ScheduledEvent::GetAutoRequeue() const
    {
        return m_autoRequeue;
    }

    TimeMs ScheduledEvent::TimeInQueueMs() const
    {
        return AZ::GetElapsedTimeMs() - m_timeInserted;
    }

    TimeMs ScheduledEvent::RemainingTimeInQueueMs() const
    {
        if (!IsScheduled())
        {
            return TimeMs{ 0 };
        }
        const TimeMs targetTimeInQueueMs = m_durationMs;
        const TimeMs elapsedTimeMs = TimeInQueueMs();
        if (elapsedTimeMs > targetTimeInQueueMs)
        {
            return TimeMs{ 0 };
        }
        const TimeMs timeRemainingInQueueMs(targetTimeInQueueMs - elapsedTimeMs);
        return std::max<TimeMs>(timeRemainingInQueueMs, TimeMs{ 0 });
    }

    TimeMs ScheduledEvent::GetIntervalMs() const
    {
        return m_durationMs;
    }

    const Name& ScheduledEvent::GetEventName() const
    {
        return m_eventName;
    }

    void ScheduledEvent::Notify()
    {
        if (m_callback)
        {
            m_callback();
        }
    }

    void ScheduledEvent::ClearHandle()
    {
        if (m_handle)
        {
            m_handle->Clear();
        }
        m_handle = nullptr;
    }
}
