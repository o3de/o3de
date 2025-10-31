/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include "MotionEvent.h"
#include "MotionEventTrack.h"
#include "EventManager.h"
#include "EventHandler.h"
#include "TwoStringEventData.h"

#include <MCore/Source/Compare.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(MotionEvent, MotionEventAllocator)

    MotionEvent::MotionEvent()
        : Event()
        , m_startTime(0.0f)
        , m_endTime(0.0f)
        , m_isSyncEvent(false)
    {
    }

    MotionEvent::MotionEvent(float timeValue, EventDataPtr&& data)
        : Event(AZStd::move(data))
        , m_startTime(timeValue)
        , m_endTime(timeValue)
        , m_isSyncEvent(false)
    {
    }

    MotionEvent::MotionEvent(float startTimeValue, float endTimeValue, EventDataPtr&& data)
        : Event(AZStd::move(data))
        , m_startTime(startTimeValue)
        , m_endTime(endTimeValue)
        , m_isSyncEvent(false)
    {
    }

    MotionEvent::MotionEvent(float timeValue, EventDataSet&& datas)
        : Event(AZStd::move(datas))
        , m_startTime(timeValue)
        , m_endTime(timeValue)
        , m_isSyncEvent(false)
    {
    }

    MotionEvent::MotionEvent(float startTimeValue, float endTimeValue, EventDataSet&& datas)
        : Event(AZStd::move(datas))
        , m_startTime(startTimeValue)
        , m_endTime(endTimeValue)
        , m_isSyncEvent(false)
    {
    }

    void MotionEvent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<MotionEvent, Event>()
            ->Version(1)
            ->Field("startTime", &MotionEvent::m_startTime)
            ->Field("endTime", &MotionEvent::m_endTime)
            ->Field("isSyncEvent", &MotionEvent::m_isSyncEvent)
            ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<MotionEvent>("MotionEvent", "")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::Default, &MotionEvent::m_startTime, "Start time", "")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
            ->DataElement(AZ::Edit::UIHandlers::Default, &MotionEvent::m_endTime, "End time", "")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
            ;
    }

    // set the start time value
    void MotionEvent::SetStartTime(float timeValue)
    {
        m_startTime = timeValue;
    }


    // set the end time value
    void MotionEvent::SetEndTime(float timeValue)
    {
        m_endTime = timeValue;
    }


    // is this a tick event?
    bool MotionEvent::GetIsTickEvent() const
    {
        return AZ::IsClose(m_startTime, m_endTime, MCore::Math::epsilon);
    }


    // toggle tick event on or off
    void MotionEvent::ConvertToTickEvent()
    {
        m_endTime = m_startTime;
    }

    void MotionEvent::SetIsSyncEvent(bool newValue)
    {
        m_isSyncEvent = newValue;
    }

    size_t MotionEvent::HashForSyncing(bool isMirror) const
    {
        if (m_eventDatas.size())
        {
            if (const AZStd::shared_ptr<const EventDataSyncable> syncable = AZStd::rtti_pointer_cast<const EventDataSyncable>(m_eventDatas[0]))
            {
                return syncable->HashForSyncing(isMirror);
            }
        }
        return 0;
    }


}   // namespace EMotionFX
