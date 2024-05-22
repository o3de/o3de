/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include "EMotionFXConfig.h"
#include <MCore/Source/RefCounted.h>
#include "MotionEvent.h"

namespace AZ
{
    class ReflectContext;
}

namespace EMotionFX
{
    // forward declarations
    class MotionInstance;
    class ActorInstance;
    class Motion;
    class AnimGraphEventBuffer;
    class EventInfo;

    /**
     * The motion event track, which stores all events and their data on a memory efficient way.
     * Events have three generic properties: a time value, an event type string and a parameter string.
     * Unique strings are only stored once in memory, so if you have for example ten events of the type "SOUND"
     * only 1 string will be stored in memory, and the events will index into the table to retrieve the string.
     * The event table can also figure out what events to process within a given time range.
     * The handling of those events is done by the MotionEventHandler class that you specify to the MotionEventManager singleton.
     */
    class EMFX_API MotionEventTrack
    {
        friend class MotionEvent;

    public:
        AZ_RTTI(MotionEventTrack, "{D142399D-C7DF-4E4A-A099-7E4E662F1E81}")
        AZ_CLASS_ALLOCATOR_DECL

        MotionEventTrack() = default;
        virtual ~MotionEventTrack() = default;

        /**
         * The constructor.
         * @param motion The motion object this track belongs to.
         */
        MotionEventTrack(Motion* motion);

        /**
         * Extended constructor.
         * @param name The name of the track.
         * @param motion The motion object this track belongs to.
         */
        MotionEventTrack(const char* name, Motion* motion);
        MotionEventTrack(const MotionEventTrack& other);

        MotionEventTrack& operator=(const MotionEventTrack& other);

        static void Reflect(AZ::ReflectContext* context);

        /**
         * Creation method.
         * @param motion The motion object this track belongs to.
         */
        static MotionEventTrack* Create(Motion* motion);

        /**
         * Extended creation.
         * @param name The name of the track.
         * @param motion The motion object this track belongs to.
         */
        static MotionEventTrack* Create(const char* name, Motion* motion);

        /**
         * Set the name of the motion event track.
         * @param name The name of the motion event track.
         */
        void SetName(const char* name);

        /**
         * Add a tick event to the event table.
         *
         * A tick event is an event whose start time and end time are the same.
         * The events can be ordered in any order on time. So you do not need to add them in a sorted order based on time.
         * This is already done automatically. The events will internally be stored sorted on time value.
         * @param timeValue The time, in seconds, at which the event should occur.
         * @param eventType The string describing the type of the event, this could for example be "SOUND" or whatever your game can process.
         * @param parameters The parameters of the event, which could be the filename of a sound file or anything else.
         * @result Returns the index to the event inside the table.
         * @note Please beware that when you use this method, the event numbers/indices might change! This is because the events are stored
         *       in an ordered way, sorted on their time value. Adding events might insert events somewhere in the middle of the array, changing all event numbers.
         */
        size_t AddEvent(float timeValue, EventDataPtr&& data);
        size_t AddEvent(float timeValue, EventDataSet&& datas);

        /**
         * Add an event to the event table.
         * The events can be ordered in any order on time. So you do not need to add them in a sorted order based on time.
         * This is already done automatically. The events will internally be stored sorted on time value.
         * @param startTimeValue The start time, in seconds, at which the event should occur.
         * @param endTimeValue The end time, in seconds, at which this event should stop.
         * @param eventTypeID The unique event ID that you wish to add. Please keep in mind that you need to have the events registered with the EMotion FX event manager before you can add them!
         * @param parameters The parameters of the event, which could be the filename of a sound file or anything else.
         * @result Returns the index to the event inside the table.
         * @note Please beware that when you use this method, the event numbers/indices might change! This is because the events are stored
         *       in an ordered way, sorted on their time value. Adding events might insert events somewhere in the middle of the array, changing all event numbers.
         */
        size_t AddEvent(float startTimeValue, float endTimeValue, EventDataPtr&& data);
        size_t AddEvent(float startTimeValue, float endTimeValue, EventDataSet&& data);

        /**
         * Process all events within a given time range.
         * @param startTime The start time of the range, in seconds.
         * @param endTime The end time of the range, in seconds.
         * @param actorInstance The actor instance on which to trigger the event.
         * @param motionInstance The motion instance which triggers the event.
         * @note The end time is also allowed to be smaller than the start time.
         */
        void ProcessEvents(float startTime, float endTime, const MotionInstance* motionInstance) const;
        void ExtractEvents(float startTime, float endTime, const MotionInstance* motionInstance, AnimGraphEventBuffer* outEventBuffer) const;

        /**
         * Get the number of events stored inside the table.
         * @result The number of events stored inside this table.
         * @see GetEvent
         */
        size_t GetNumEvents() const;

        /**
         * Get a given event from the table.
         * @param eventNr The event number you wish to retrieve. This must be in range of 0..GetNumEvents() - 1.
         * @result A const reference to the event.
         */
        const MotionEvent& GetEvent(size_t eventNr) const { return m_events[eventNr]; }

        /**
         * Get a given event from the table.
         * @param eventNr The event number you wish to retrieve. This must be in range of 0..GetNumEvents() - 1.
         * @result A reference to the event.
         */
        MotionEvent& GetEvent(size_t eventNr) { return m_events[eventNr]; }

        /**
         * Remove a given event from the table.
         * @param eventNr The event number to remove. This must be in range of 0..GetNumEvents() - 1.
         */
        void RemoveEvent(size_t eventNr);

        /**
         * Remove all motion events from the table. Does not remove the parameters.
         */
        void RemoveAllEvents();

        /**
         * Remove all motion events and parameters from the table.
         */
        void Clear();

        const char* GetName() const;
        const AZStd::string& GetNameString() const;

        void SetIsEnabled(bool enabled);
        bool GetIsEnabled() const;

        bool GetIsDeletable() const;
        void SetIsDeletable(bool isDeletable);

        Motion* GetMotion() const;
        void SetMotion(Motion* newMotion);

        void CopyTo(MotionEventTrack* targetTrack) const;
        void ReserveNumEvents(size_t numEvents);

    protected:
        AZStd::vector<MotionEvent> m_events;
        AZStd::string m_name;

        /// The motion where this track belongs to.
        Motion* m_motion;

        /// Is this track enabled?
        bool m_enabled = true;
        bool m_deletable = true;

    private:
        void ProcessEventsImpl(float startTime, float endTime, ActorInstance* actorInstance, const MotionInstance* motionInstance, const AZStd::function<void(EMotionFX::EventInfo&)>& processFunc);

        template <typename Functor>
        void ExtractEvents(float startTime, float endTime, const MotionInstance* motionInstance, const Functor& processFunc, bool handleLoops = true) const;

        static bool VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
    };
} // namespace EMotionFX
