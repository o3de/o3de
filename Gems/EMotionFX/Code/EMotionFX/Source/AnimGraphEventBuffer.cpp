/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AnimGraphEventBuffer.h"
#include "EventManager.h"
#include "AnimGraphInstance.h"
#include <EMotionFX/Source/MotionEvent.h>
#include <MCore/Source/LogManager.h>


namespace EMotionFX
{
    AnimGraphEventBuffer::AnimGraphEventBuffer()
    {
    }

    // set the emitter pointers
    void AnimGraphEventBuffer::UpdateEmitters(AnimGraphNode* emitterNode)
    {
        for (EventInfo& event : m_events)
        {
            event.m_emitter = emitterNode;
        }
    }

    void AnimGraphEventBuffer::UpdateWeights(AnimGraphInstance* animGraphInstance)
    {
        for (EventInfo& curEvent : m_events)
        {
            AnimGraphNodeData* emitterUniqueData = curEvent.m_emitter->FindOrCreateUniqueNodeData(animGraphInstance);
            curEvent.m_globalWeight = emitterUniqueData->GetGlobalWeight();
            curEvent.m_localWeight = emitterUniqueData->GetLocalWeight();
        }
    }

    // log details of all events
    void AnimGraphEventBuffer::Log() const
    {
        for (const EMotionFX::EventInfo& event : m_events)
        {
            AZStd::string eventDataString;

            for (const EventDataPtr& eventData : event.m_event->GetEventDatas())
            {
                if (eventData)
                {
                    eventDataString += '{' + eventData->ToString() + '}';
                }
                else {
                    eventDataString += "{<null>}";
                }
            }

            MCore::LogInfo("Event: (time=%f) (eventData=%s) (emitter=%s) (locWeight=%.4f  globWeight=%.4f)",
                event.m_timeValue,
                eventDataString.size() ? eventDataString.c_str() : "<none>",
                event.m_emitter->GetName(),
                event.m_localWeight,
                event.m_globalWeight);
        }
    }

    void AnimGraphEventBuffer::TriggerEvents() const
    {
        for (const EventInfo& event : m_events)
        {
            if (event.m_eventState != EventInfo::EventState::ACTIVE)
            {
                GetEventManager().OnEvent(event);
            }
        }
    }

    void AnimGraphEventBuffer::Reserve(size_t numEvents)
    {
        m_events.reserve(numEvents);
    }

    void AnimGraphEventBuffer::Resize(size_t numEvents)
    {
        m_events.resize(numEvents);
    }

    void AnimGraphEventBuffer::AddEvent(const EventInfo& newEvent)
    {
        m_events.emplace_back(newEvent);
    }

    void AnimGraphEventBuffer::AddAllEventsFrom(const AnimGraphEventBuffer& eventBuffer)
    {
        const size_t numEventsToCopy = eventBuffer.GetNumEvents();
        const size_t numPrevEvents = GetNumEvents();

        Resize(GetNumEvents() + numEventsToCopy);

        for (size_t i = 0; i < numEventsToCopy; ++i)
        {
            SetEvent(numPrevEvents + i, eventBuffer.GetEvent(i));
        }
    }

    void AnimGraphEventBuffer::Clear()
    {
        m_events.clear();
    }

    void AnimGraphEventBuffer::SetEvent(size_t index, const EventInfo& eventInfo)
    {
        m_events[index] = eventInfo;
    }
} // namespace EMotionFX
