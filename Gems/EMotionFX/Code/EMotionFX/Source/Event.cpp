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

#include "Allocators.h"
#include "Event.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(Event, MotionEventAllocator, 0)

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
