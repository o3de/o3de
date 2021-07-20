/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

        Event() = default;
        explicit Event(EventDataPtr&& data);
        explicit Event(EventDataSet&& datas);
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
