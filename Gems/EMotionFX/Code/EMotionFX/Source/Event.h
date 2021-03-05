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

#include "EventData.h"

#include <AzCore/std/smart_ptr/shared_ptr.h>

namespace AZ
{
    class ReflectContext;
}

namespace EMotionFX
{
    using EventDataPtr = AZStd::shared_ptr<const EventData>;
    using EventDataSet = AZStd::vector<EventDataPtr>;

    class EMFX_API Event
    {
    public:
        AZ_RTTI(Event, "{67549E9F-8E3F-4336-BDB8-716AFCBD4985}");
        AZ_CLASS_ALLOCATOR_DECL

        Event(EventDataPtr&& data = nullptr)
            : m_eventDatas{AZStd::move(data)}
        {
        }

        Event(EventDataSet&& datas)
            : m_eventDatas(AZStd::move(datas))
        {
        }

        virtual ~Event() = default;

        static void Reflect(AZ::ReflectContext* context);

        const EventDataSet& GetEventDatas() const;
        EventDataSet& GetEventDatas();
        void AppendEventData(EventDataPtr&& newData);
        void RemoveEventData(size_t index);
        void SetEventData(size_t index, EventDataPtr&& newData);
        void InsertEventData(size_t index, EventDataPtr&& newData);

    protected:
        EventDataSet m_eventDatas;
    };
} // end namespace EMotionFX
