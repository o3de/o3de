/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/EBus/ScheduledEventHandle.h>
#include <AzCore/EBus/ScheduledEvent.h>
#include <AzCore/Console/ILogger.h>

namespace AZ
{
    ScheduledEventHandle::ScheduledEventHandle(TimeMs executeTimeMs, TimeMs durationTimeMs, ScheduledEvent* scheduledEvent, bool ownsScheduledEvent)
        : m_executeTimeMs(executeTimeMs)
        , m_durationMs(durationTimeMs)
        , m_event(scheduledEvent)
        , m_ownsScheduledEvent(ownsScheduledEvent)
    {
        ;
    }

    bool ScheduledEventHandle::operator >(const ScheduledEventHandle& rhs) const
    {
        return m_executeTimeMs > rhs.m_executeTimeMs;
    }

    bool ScheduledEventHandle::Notify()
    {
        if (m_event)
        {
            if (m_event->m_handle == this)
            {
                m_event->Notify();

                // Check whether or not the event was deleted during the callback
                if (m_event != nullptr)
                {
                    if (m_event->m_autoRequeue)
                    {
                        m_event->Requeue();
                        return true;
                    }
                    else // Not configured to auto-requeue, so remove the handle
                    {
                        m_event->ClearHandle();
                    }
                }
            }
            else
            {
                AZLOG_WARN("ScheduledEventHandle event pointer doesn't match to the pointer of handle to the event.");
            }
        }
        return false; // Event has been deleted, so the handle class must be deleted after this function.
    }

    TimeMs ScheduledEventHandle::GetExecuteTimeMs() const
    {
        return m_executeTimeMs;
    }

    TimeMs ScheduledEventHandle::GetDurationTimeMs() const
    {
        return m_durationMs;
    }

    bool ScheduledEventHandle::GetOwnsScheduledEvent() const
    {
        return m_ownsScheduledEvent;
    }

    ScheduledEvent* ScheduledEventHandle::GetScheduledEvent() const
    {
        return m_event;
    }

    void ScheduledEventHandle::Clear()
    {
        // We can nullptr events we don't own, but events that we do own need to go back into the free pool
        if (!m_ownsScheduledEvent)
        {
            m_event = nullptr;
        }
    }
}
