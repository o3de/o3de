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

// include the required headers
#include "EMotionFXConfig.h"
#include "Event.h"

#include <AzCore/std/string/string.h>

namespace AZ
{
    class ReflectContext;
}

namespace EMotionFX
{
    /**
     * @brief A description of an event that happens at a given time in a motion.
     *
     * A MotionEvent could be a footstep sound that needs to be played, or a
     * particle system that needs to be spawened or a script that needs to be
     * executed.  Motion events are completely generic, which means EMotion FX
     * does not handle the events for you. It is up to you how you handle the
     * events.  Also we do not specify any set of available events.
     *
     * Each MotionEvent has a list of EventData instances that are attached to
     * the event. The EventData list is used by an Event Handler to perform the
     * required actions.
     *
     * All motion events are stored in a motion event table. This table
     * contains the event data for the event types and parameters, which can
     * be shared between events. This will mean if you have 100 events that
     * contain the strings "SOUND" and "Footstep.wav", those strings will only
     * be stored in memory once.
     *
     * To manually add motion events to a motion, do something like this:
     *
     * @code
     * motion->GetEventTable().AddEvent(0.0f, GetEMotionFX().GetEventManager()->FindOrCreateEventData<SoundEvent>("Footstep.wav"));
     * motion->GetEventTable().AddEvent(3.0f, GetEMotionFX().GetEventManager()->FindOrCreateEventData<ScriptEvent>("OpenDoor.script"));
     * motion->GetEventTable().AddEvent(7.0f, GetEMotionFX().GetEventManager()->FindOrCreateEventData<SoundEvent>("Footstep.wav"));
     * @endcode
     *
     * To listen to motion events, connect to the ActorNotificationBus, and
     * implement OnMotionEvent().
     */
    class EMFX_API MotionEvent
        : public Event
    {
    public:
        AZ_RTTI(MotionEvent, "{4A3C24AC-F924-40E1-B274-FF5A60023181}", Event)
        AZ_CLASS_ALLOCATOR_DECL

        MotionEvent();

        /**
         * Creates a tick event
         * @param timeValue The time value, in seconds, when the motion event should occur.
         * @param data The values to emit when the event is triggered
         */
        MotionEvent(float timeValue, EventDataPtr&& data);

        /**
         * Creates a ranged event
         * @param startTimeValue The start time value, in seconds, when the motion event should start.
         * @param endTimeValue The end time value, in seconds, when the motion
         *        event should end. When this is equal to the start time value
         *        we won't trigger an end event, but only a start event at the
         *        specified time.
         * @param data The values to emit when the event is triggered
         */
        MotionEvent(float startTimeValue, float endTimeValue, EventDataPtr&& data);

        /**
         * Creates a tick event
         * @param timeValue The time value, in seconds, when the motion event should occur.
         * @param datas The list of values to emit when the event is triggered
         */
        MotionEvent(float timeValue, EventDataSet&& datas);

        /**
         * Creates a ranged event
         * @param startTimeValue The start time value, in seconds, when the motion event should start.
         * @param endTimeValue The end time value, in seconds, when the motion
         *        event should end. When this is equal to the start time value
         *        we won't trigger an end event, but only a start event at the
         *        specified time.
         * @param data The list of values to emit when the event is triggered
         */
        MotionEvent(float startTimeValue, float endTimeValue, EventDataSet&& datas);

        static void Reflect(AZ::ReflectContext* context);

        /**
         * Set the start time value of the event, which is when the event should be processed.
         * @param timeValue The time on which the event shall be processed, in seconds.
         */
        void SetStartTime(float timeValue);

        /**
         * Set the end time value of the event, which is when the event should be processed.
         * @param timeValue The time on which the event shall be processed, in seconds.
         */
        void SetEndTime(float timeValue);

        /**
         * Get the start time value of this event, which is when it should be executed.
         * @result The start time, in seconds, on which the event should be executed/processed.
         */
        float GetStartTime() const { return m_startTime; }

        /**
         * Get the end time value of this event, which is when it should stop.
         * @result The end time, in seconds, on which the event should be stop.
         */
        float GetEndTime() const { return m_endTime; }

        /**
         * Check whether this is a tick event or not.
         * It is a tick event when both start and end time are equal.
         * @result Returns true when start and end time are equal, otherwise false is returned.
         */
        bool GetIsTickEvent() const;

        /**
         * Convert this event into a tick event.
         * This makes the end time equal to the start time.
         */
        void ConvertToTickEvent();

        /**
         * Check if this event is a sync event.
         *
         * The Sync track of a motion can only contain events where GetIsSyncEvent() is true.
         */
        bool GetIsSyncEvent() const { return m_isSyncEvent; }

        /**
         * Set if this event is a sync event.
         */
        void SetIsSyncEvent(bool newValue);

        size_t HashForSyncing(bool isMirror) const;

    private:
        /// Time value in seconds when the event start should be triggered.
        float m_startTime;

        /// Time value in seconds when the event end should be triggered.
        float m_endTime;

        bool m_isSyncEvent;
    };
} // namespace EMotionFX
