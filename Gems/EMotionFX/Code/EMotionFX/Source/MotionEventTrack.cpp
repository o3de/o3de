/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MotionEventTrack.h"
#include "MotionEvent.h"
#include "EventManager.h"
#include "EventHandler.h"
#include "TwoStringEventData.h"
#include "AnimGraphEventBuffer.h"
#include <MCore/Source/LogManager.h>
#include <MCore/Source/StringIdPool.h>
#include <EMotionFX/Source/Allocators.h>
#include <AzCore/Math/MathUtils.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(MotionEventTrack, MotionEventAllocator)

    MotionEventTrack::MotionEventTrack(Motion* motion)
        : m_motion(motion)
    {
    }

    MotionEventTrack::MotionEventTrack(const char* name, Motion* motion)
        : m_motion(motion)
        , m_name(name)
    {
    }

    MotionEventTrack::MotionEventTrack(const MotionEventTrack& other)
    {
        *this = other;
    }

    MotionEventTrack& MotionEventTrack::operator=(const MotionEventTrack& other)
    {
        if (this == &other)
        {
            return *this;
        }
        m_events = other.m_events;
        m_motion = other.m_motion;
        m_name = other.m_name;
        return *this;
    }

    void MotionEventTrack::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<MotionEventTrack>()
            ->Version(2, VersionConverter)
            ->Field("name", &MotionEventTrack::m_name)
            ->Field("enabled", &MotionEventTrack::m_enabled)
            ->Field("deletable", &MotionEventTrack::m_deletable)
            ->Field("events", &MotionEventTrack::m_events)
            ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<MotionEventTrack>("MotionEventTrack", "")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::Default, &MotionEventTrack::m_events, "Events", "List of events in this track")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
            ;
    }

    bool MotionEventTrack::VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        const unsigned int version = classElement.GetVersion();
        if (version < 2)
        {
            int nameElementIndex = classElement.FindElement(AZ_CRC_CE("name"));
            if (nameElementIndex < 0)
            {
                return false;
            }
            AZ::SerializeContext::DataElementNode& nameElement = classElement.GetSubElement(nameElementIndex);

            MCore::StringIdPoolIndex oldName;
            const bool result = nameElement.GetData<MCore::StringIdPoolIndex>(oldName);

            classElement.RemoveElement(nameElementIndex);
            if (result)
            {
                AZStd::string newName = MCore::GetStringIdPool().GetName(oldName.m_index);
                classElement.AddElementWithData(context, "name", newName);
            }
        }
        return true;
    }


    // creation
    MotionEventTrack* MotionEventTrack::Create(Motion* motion)
    {
        return aznew MotionEventTrack(motion);
    }


    // creation
    MotionEventTrack* MotionEventTrack::Create(const char* name, Motion* motion)
    {
        return aznew MotionEventTrack(name, motion);
    }


    // set the name of the motion event track
    void MotionEventTrack::SetName(const char* name)
    {
        m_name = name;
    }


    // the main method to add an event, which automatically inserts it at the right sorted location
    size_t MotionEventTrack::AddEvent(float startTimeValue, float endTimeValue, EventDataSet&& eventData)
    {
        auto compFunc = [](const float time, const EMotionFX::MotionEvent& event)
        {
            return time < event.GetStartTime();
        };
        const MotionEvent* insertPosition = AZStd::upper_bound(m_events.cbegin(), m_events.cend(), startTimeValue, compFunc);
        const MotionEvent* positionAfterInsert = m_events.emplace(insertPosition, startTimeValue, endTimeValue, AZStd::move(eventData));

        return AZStd::distance(m_events.cbegin(), positionAfterInsert);
    }


    size_t MotionEventTrack::AddEvent(float startTimeValue, float endTimeValue, EventDataPtr&& eventData)
    {
        return AddEvent(startTimeValue, endTimeValue, EventDataSet {AZStd::move(eventData)});
    }

    // Add a tick event
    size_t MotionEventTrack::AddEvent(float timeValue, EventDataSet&& data)
    {
        return AddEvent(timeValue, timeValue, AZStd::move(data));
    }

    // Add a tick event
    size_t MotionEventTrack::AddEvent(float timeValue, EventDataPtr&& data)
    {
        return AddEvent(timeValue, timeValue, EventDataSet {AZStd::move(data)});
    }


    class EventManagerOnEventFunctor
    {
    public:
        template <typename... Args>
        void operator()(Args&&... args) const
        {
            GetEventManager().OnEvent(EventInfo(AZStd::forward<Args>(args)...));
        }
    };

    class AnimGraphEventBufferAddEventsFunctor
    {
    public:
        AnimGraphEventBufferAddEventsFunctor(AnimGraphEventBuffer* buffer) : m_buffer(buffer) {}

        template <typename... Args>
        void operator()(Args&&... args) const
        {
            m_buffer->AddEvent(AZStd::forward<Args>(args)...);
        }

    private:
        mutable AnimGraphEventBuffer* m_buffer;
    };

    // process all events within a given time range
    void MotionEventTrack::ProcessEvents(float startTime, float endTime, const MotionInstance* motionInstance) const
    {
        ExtractEvents(startTime, endTime, motionInstance, EventManagerOnEventFunctor());
    }

    void MotionEventTrack::ExtractEvents(float startTime, float endTime, const MotionInstance* motionInstance, AnimGraphEventBuffer* outEventBuffer) const
    {
        ExtractEvents(startTime, endTime, motionInstance, AnimGraphEventBufferAddEventsFunctor(outEventBuffer));
    }

    template <typename Functor>
    void MotionEventTrack::ExtractEvents(float startTime, float endTime, const MotionInstance* motionInstance, const Functor& processFunc, bool handleLoops) const
    {
        const float duration = motionInstance->GetDuration();
        startTime = AZ::GetClamp(startTime, 0.0f, duration);
        endTime = AZ::GetClamp(endTime, 0.0f, duration);

        const bool playForward = (motionInstance->GetPlayMode() == PLAYMODE_FORWARD);
        if (handleLoops)
        {
            const bool looped = playForward ? (startTime > endTime) : (startTime < endTime);
            if (!looped)
            {
                ExtractEvents(startTime, endTime, motionInstance, processFunc, false);
            }
            else
            {
                if (playForward)
                {
                    AZ_Assert(startTime > endTime, "Expecting start time to be after the end time when looping in forward play mode.");
                    ExtractEvents(startTime, motionInstance->GetDuration(), motionInstance, processFunc, false);
                    ExtractEvents(0.0f, endTime, motionInstance, processFunc, false);
                }
                else
                {
                    AZ_Assert(startTime < endTime, "Expecting start time to be before the end time when looping in backward play mode.");
                    ExtractEvents(startTime, 0.0f, motionInstance, processFunc, false);
                    ExtractEvents(motionInstance->GetDuration(), endTime, motionInstance, processFunc, false);
                }
            }
        }
        else
        {
            if (playForward)
            {
                AZ_Assert(startTime <= endTime, "Expecting start time to smaller or equal to the end time in forward play mode.");
                for (const EMotionFX::MotionEvent& event : m_events)
                {
                    const float eventStartTime = event.GetStartTime();
                    const float eventEndTime = event.GetEndTime();

                    // We can quit processing events if it's impossible that they execute.
                    // All events are stored in a time sorted order.
                    if (eventStartTime >= endTime)
                    {
                        break;
                    }

                    // Trigger the event start.
                    bool triggeredStart = false;
                    if (eventStartTime >= startTime && eventStartTime < endTime)
                    {
                        processFunc(
                            eventStartTime,
                            motionInstance->GetActorInstance(),
                            motionInstance,
                            const_cast<MotionEvent*>(&event),
                            EventInfo::EventState::START
                        );
                        triggeredStart = true;
                    }

                    // In case we deal with a range based event.
                    if (!event.GetIsTickEvent())
                    {
                        // Trigger the event end.
                        if (eventEndTime >= startTime && eventEndTime < endTime)
                        {
                            processFunc(
                                eventEndTime,
                                motionInstance->GetActorInstance(),
                                motionInstance,
                                const_cast<MotionEvent*>(&event),
                                EventInfo::EventState::END
                            );
                        }
                        else if (!triggeredStart && eventEndTime >= endTime) // We're somewhere inside the range based event.
                        {
                            processFunc(
                                endTime,
                                motionInstance->GetActorInstance(),
                                motionInstance,
                                const_cast<MotionEvent*>(&event),
                                EventInfo::EventState::ACTIVE
                            );
                        }
                    }
                } // For all events.
            }
            else // Playing backward.
            {
                AZ_Assert(startTime >= endTime, "Expecting start time to be larger than then end time in backward mode.");
                for (auto i = m_events.crbegin(); i != m_events.crend(); ++i)
                {
                    const EMotionFX::MotionEvent& event = *i;
                    const float eventStartTime = event.GetStartTime();
                    const float eventEndTime = event.GetEndTime();

                    // We can quit processing events if it's impossible that they execute.
                    // All events are stored in a time sorted order.
                    if (eventEndTime < endTime)
                    {
                        break;
                    }

                    // Trigger the event start.
                    bool triggeredStart = false;
                    if (eventEndTime > endTime && eventEndTime <= startTime)
                    {
                        processFunc(
                            eventEndTime,
                            motionInstance->GetActorInstance(),
                            motionInstance,
                            const_cast<MotionEvent*>(&event),
                            EventInfo::EventState::START
                        );

                        triggeredStart = true;
                    }

                    // If we're dealing with a range based event.
                    if (!event.GetIsTickEvent())
                    {
                        // Trigger the event end.
                        if (eventStartTime > endTime && eventStartTime <= startTime)
                        {
                            processFunc(
                                eventStartTime,
                                motionInstance->GetActorInstance(),
                                motionInstance,
                                const_cast<MotionEvent*>(&event),
                                EventInfo::EventState::END
                            );
                        }
                        else if (!triggeredStart && eventStartTime <= endTime) // We're somewhere inside the range based event.
                        {
                            processFunc(
                                endTime,
                                motionInstance->GetActorInstance(),
                                motionInstance,
                                const_cast<MotionEvent*>(&event),
                                EventInfo::EventState::ACTIVE
                            );
                        }
                    }
                } // for all events
            } // If (playforward)
        }
    }


    // get the number of events
    size_t MotionEventTrack::GetNumEvents() const
    {
        return m_events.size();
    }


    // remove a given event
    void MotionEventTrack::RemoveEvent(size_t eventNr)
    {
        m_events.erase(AZStd::next(m_events.begin(), eventNr));
    }


    // remove all events from the table
    void MotionEventTrack::RemoveAllEvents()
    {
        m_events.clear();
    }


    // remove all events and parameters from the table
    void MotionEventTrack::Clear()
    {
        RemoveAllEvents();
    }

    const char* MotionEventTrack::GetName() const
    {
        return m_name.c_str();
    }

    const AZStd::string& MotionEventTrack::GetNameString() const
    {
        return m_name;
    }

    // copy the track contents to a target track
    // this overwrites all existing contents of the target track
    void MotionEventTrack::CopyTo(MotionEventTrack* targetTrack) const
    {
        targetTrack->m_name = m_name;
        targetTrack->m_events = m_events;
        targetTrack->m_enabled = m_enabled;
    }

    // reserve memory for a given amount of events
    void MotionEventTrack::ReserveNumEvents(size_t numEvents)
    {
        m_events.reserve(numEvents);
    }

    void MotionEventTrack::SetIsEnabled(bool enabled)
    {
        m_enabled = enabled;
    }


    bool MotionEventTrack::GetIsEnabled() const
    {
        return m_enabled;
    }


    bool MotionEventTrack::GetIsDeletable() const
    {
        return m_deletable;
    }


    void MotionEventTrack::SetIsDeletable(bool isDeletable)
    {
        m_deletable = isDeletable;
    }


    Motion* MotionEventTrack::GetMotion() const
    {
        return m_motion;
    }

    void MotionEventTrack::SetMotion(Motion* newMotion)
    {
        m_motion = newMotion;
    }
} // namespace EMotionFX
