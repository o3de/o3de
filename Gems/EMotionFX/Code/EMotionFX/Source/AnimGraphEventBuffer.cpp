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
            event.mEmitter = emitterNode;
        }
    }

    void AnimGraphEventBuffer::UpdateWeights(AnimGraphInstance* animGraphInstance)
    {
        for (EventInfo& curEvent : m_events)
        {
            AnimGraphNodeData* emitterUniqueData = curEvent.mEmitter->FindOrCreateUniqueNodeData(animGraphInstance);
            curEvent.mGlobalWeight = emitterUniqueData->GetGlobalWeight();
            curEvent.mLocalWeight = emitterUniqueData->GetLocalWeight();
        }
    }

    // log details of all events
    void AnimGraphEventBuffer::Log() const
    {
        for (const EMotionFX::EventInfo& event : m_events)
        {
            AZStd::string eventDataString;

            for (const EventDataPtr& eventData : event.mEvent->GetEventDatas())
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
                event.mTimeValue,
                eventDataString.size() ? eventDataString.c_str() : "<none>",
                event.mEmitter->GetName(),
                event.mLocalWeight,
                event.mGlobalWeight);
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

    void AnimGraphEventBuffer::Reserve(uint32 numEvents)
    {
        m_events.reserve(numEvents);
    }

    void AnimGraphEventBuffer::Resize(uint32 numEvents)
    {
        m_events.resize(numEvents);
    }

    void AnimGraphEventBuffer::AddEvent(const EventInfo& newEvent)
    {
        m_events.emplace_back(newEvent);
    }

    void AnimGraphEventBuffer::AddAllEventsFrom(const AnimGraphEventBuffer& eventBuffer)
    {
        const AZ::u32 numEventsToCopy = eventBuffer.GetNumEvents();
        const uint32 numPrevEvents = GetNumEvents();

        Resize(GetNumEvents() + numEventsToCopy);

        for (uint32 i = 0; i < numEventsToCopy; ++i)
        {
            SetEvent(numPrevEvents + i, eventBuffer.GetEvent(i));
        }
    }

    void AnimGraphEventBuffer::Clear()
    {
        m_events.clear();
    }

    void AnimGraphEventBuffer::SetEvent(uint32 index, const EventInfo& eventInfo)
    {
        m_events[index] = eventInfo;
    }
} // namespace EMotionFX
