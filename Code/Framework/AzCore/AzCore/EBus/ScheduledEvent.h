/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Time/ITime.h>
#include <AzCore/Name/Name.h>
#include <AzCore/std/functional.h>

namespace AZ
{
    class EventSchedulerSystemComponent;
    class ScheduledEventHandle;

    //! @class ScheduledEvent
    //! The ScheduledEvent class is used to run a registered callback function at a specified execution time.
    //! Total time duration between queuing and the scheduled event triggering is not guaranteed, and will at least be quantized to frametime.
    //! This event can trigger continuously at the specified interval in ms if set with the auto-re-queue function.
    //! This event should be declared as a member of class, not in a local function as it will not trigger if it goes out of scope.
    class ScheduledEvent
    {
    public:
        //! Default constructor only for AZStd::deque compatibility.
        ScheduledEvent() = default;

        //! Constructor of ScheduledEvent class.
        //! @param callback  a call back function to be executed when the event triggers
        //! @param eventName name of the scheduled event for easier debugging
        ScheduledEvent(const AZStd::function<void()>& callback, const Name& eventName);

        ~ScheduledEvent();

        //! Enqueue this event with the IEventScheduler.
        //! @param durationMs the number of milliseconds to wait before triggering this scheduled event
        //! @param autoRequeue true for running this event infinitely, false for running once
        void Enqueue(TimeMs durationMs, bool autoRequeue = false);

        //! Requeue this scheduled event so that it triggers again.
        void Requeue();

        //! ReQueue this event with a new delay.
        //! @param durationMs the number of milliseconds to wait before triggering this scheduled event
        void Requeue(TimeMs durationMs);

        //! Disable auto re-queue and clear handle not to run this event.
        void RemoveFromQueue();

        //! Gets whether or not this scheduled event is currently scheduled for execution.
        //! @return true if the scheduled event is registered with the IEventScheduler
        bool IsScheduled() const;

        //! Returns whether or not auto-requeue is enabled.
        //! @return true for auto re-queued, false for not auto re-queued
        bool GetAutoRequeue() const;

        //! Gets the elapsed time (Ms) from when this event was inserted.
        //! @return TimeMs since this scheduled event was last scheduled
        TimeMs TimeInQueueMs() const;

        //! Gets the remaining time (Ms) before this event is scheduled to be triggered.
        //! @return TimeMs before this event will be triggered
        TimeMs RemainingTimeInQueueMs() const;

        //! Returns the duration this event was initially scheduled with.
        //! @return TimeMs provided during event scheduling
        TimeMs GetIntervalMs() const;

        //! Returns the name of this scheduled event.
        //! @return the name of this scheduled event
        const Name& GetEventName() const;

    private:
        //! Runs the callback function.
        void Notify();

        //! Clears any currently set handle pointer.
        void ClearHandle();

        Name m_eventName; //< Scheduled event name
        AZStd::function<void()> m_callback; //< A callback function to run when the scheduled event triggers
        ScheduledEventHandle* m_handle = nullptr; //< Handle pointer to protect running a deleted event callback function
        TimeMs m_durationMs = TimeMs{ 0 }; //< Interval in milliseconds to run an event
        TimeMs m_timeInserted = TimeMs{ 0 }; //< Time stamp in ms of when this event was inserted
        bool m_autoRequeue = false; //< Automatically re-queue option. true for re-queue

        friend class ScheduledEventHandle;
        friend class EventSchedulerSystemComponent;
    };
}

