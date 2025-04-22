/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Time/ITime.h>

namespace AZ
{
    class ScheduledEvent;

    //! @struct ScheduledEventHandle
    //! This is a handle class that wraps a scheduled event.
    //! This is created inside of EventSchedulerSystemComponent and deleted in EventSchedulerSystemComponent::OnTick()
    class ScheduledEventHandle
    {
    public:
        //! Default constructor for AZStd::deque compatibility.
        ScheduledEventHandle() = default;

        //! Constructor of ScheduledEventHandle struct.
        //! @param executeTimeMs  an absolute time in ms at which point the scheduled event should trigger
        //! @param durationTimeMs the interval time in ms used for prioritization as well as re-queueing
        //! @param scheduledEvent a scheduled event to run
        //! @param ownsScheduledEvent true if the event handle owns its own scheduled event instance
        ScheduledEventHandle(TimeMs executeTimeMs, TimeMs durationTimeMs, ScheduledEvent* scheduledEvent, bool ownsScheduledEvent = false);

        //! operator of comparing a scheduled event by execute time.
        //! @param a_Rhs a scheduled event handle to compare
        bool operator >(const ScheduledEventHandle& rhs) const;

        //! Run a callback function.
        //! @return true for re-queuing a scheduled event or false for deleting this class.
        bool Notify();

        //! Get the execution time in ms for this scheduled event.
        //! @return the execution time in ms for this scheduled event
        TimeMs GetExecuteTimeMs() const;

        //! Get the duration time in ms for this scheduled event.
        //! This is the time between queuing and triggering, not how long the event is expected to execute for
        //! @return the duration time in ms for this scheduled event
        TimeMs GetDurationTimeMs() const;

        //! Gets whether or not the event handle owns its own scheduled event.
        //! @return true if the event handle owns 
        bool GetOwnsScheduledEvent() const;

        //! Gets the scheduled event instance bound to this event handle.
        //! @return the scheduled event instance bound to this event handle
        ScheduledEvent* GetScheduledEvent() const;

        //! Used internally to clear the event associated with this handle.
        void Clear();

    private:

        TimeMs m_executeTimeMs = TimeMs{ 0 }; //< execution time of the scheduled event
        TimeMs m_durationMs = TimeMs{ 0 };    //< interval time of the scheduled event
        ScheduledEvent* m_event = nullptr;    //< pointer to the scheduled event
        bool m_ownsScheduledEvent = false;    //< if the handle manages the memory of its own event
    };
}

