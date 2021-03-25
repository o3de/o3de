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
        //! @param autoDelete if the event handle will be automatically deleted after execution completes
        ScheduledEventHandle(TimeMs executeTimeMs, TimeMs durationTimeMs, ScheduledEvent* scheduledEvent, bool isAutoDelete);

        //! operator of comparing a scheduled event by execute time.
        //! @param a_Rhs a scheduled event handle to compare
        bool operator >(const ScheduledEventHandle& rhs) const;

        //! Run a callback function.
        //! @return true for re-queuing a scheduled event or false for deleting this class.
        bool Notify();

        //! Set nullptr for a scheduled event pointer.
        void Clear();

        //! Get the execution time in ms for this scheduled event.
        //! @return the execution time in ms for this scheduled event
        TimeMs GetExecuteTimeMs() const;

        //! Get the duration time in ms for this scheduled event.
        //! This is the time between queuing and triggering, not how long the event is expected to execute for
        //! @return the duration time in ms for this scheduled event
        TimeMs GetDurationTimeMs() const;

    private:

        TimeMs m_executeTimeMs = TimeMs{ 0 }; //< execution time of the scheduled event
        TimeMs m_durationMs = TimeMs{ 0 };    //< interval time of the scheduled event
        ScheduledEvent* m_event = nullptr;    //< pointer to the scheduled event
        bool m_autoDelete = false;            //< if the handle manages the memory of its own event
    };
}

