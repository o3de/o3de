/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Allocators.h"
#include "Event.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(Event, MotionEventAllocator, 0)

    Event::Event(const Event& data)
        : m_eventDatas(data.m_eventDatas)
    {
    }

    Event::Event(Event&& event)
        : m_eventDatas(AZStd::move(event.m_eventDatas)),
          m_eventDatasChangeEvent(AZStd::move(event.m_eventDatasChangeEvent))
    {
    }

    Event::Event(EventDataPtr&& data)
        : m_eventDatas{ AZStd::move(data) }
    {
    }

    Event::Event(EventDataSet&& datas)
        : m_eventDatas(AZStd::move(datas))
    {
    }
    
    Event& Event::operator=(const Event& other) 
    {
        m_eventDatas = other.m_eventDatas;
        return *this;
    }

    Event& Event::operator=(Event&& other) 
    {
        m_eventDatas = AZStd::move(other.m_eventDatas);
        return *this;
    }

    void Event::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<Event>()
            ->Version(1)
            ->Field("eventDatas", &Event::m_eventDatas)
            ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<Event>("Event", "")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ;
    }

    const EventDataSet& Event::GetEventDatas() const
    {
        return m_eventDatas;
    }

    EventDataSet& Event::GetEventDatas()
    {
        return m_eventDatas;
    }

    void Event::AppendEventData(EventDataPtr&& newData)
    {
        m_eventDatas.emplace_back(AZStd::move(newData));
        m_eventDatasChangeEvent.Signal();
    }

    void Event::RemoveEventData(size_t index)
    {
        m_eventDatas.erase(AZStd::next(m_eventDatas.begin(), index));
        m_eventDatasChangeEvent.Signal();
    }

    void Event::SetEventData(size_t index, EventDataPtr&& newData)
    {
        m_eventDatas[index] = AZStd::move(newData);
        m_eventDatasChangeEvent.Signal();
    }

    void Event::InsertEventData(size_t index, EventDataPtr&& newData)
    {
        m_eventDatas.emplace(AZStd::next(m_eventDatas.begin(), index), AZStd::move(newData));
        m_eventDatasChangeEvent.Signal();
    }

    void Event::SetEventDataChangeEvent(Event::EventDataChangeEvent::Handler& handler)
    {
        handler.Connect(m_eventDatasChangeEvent);
    }

} // end namespace EMotionFX
