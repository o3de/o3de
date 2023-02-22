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
    AZ_CLASS_ALLOCATOR_IMPL(Event, MotionEventAllocator)

    Event::Event(EventDataPtr&& data)
        : m_eventDatas{ AZStd::move(data) }
    {
    }

    Event::Event(EventDataSet&& datas)
        : m_eventDatas(AZStd::move(datas))
    {
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
    }

    void Event::RemoveEventData(size_t index)
    {
        m_eventDatas.erase(AZStd::next(m_eventDatas.begin(), index));
    }

    void Event::SetEventData(size_t index, EventDataPtr&& newData)
    {
        m_eventDatas[index] = AZStd::move(newData);
    }

    void Event::InsertEventData(size_t index, EventDataPtr&& newData)
    {
        m_eventDatas.emplace(AZStd::next(m_eventDatas.begin(), index), AZStd::move(newData));
    }

} // end namespace EMotionFX
