/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include "EventTraceDataParser.h"
#include "EventTraceDataAggregator.h"
#include "EventTraceEvents.h"

namespace Driller
{
    AZ::Debug::DrillerHandlerParser* EventTraceDataParser::OnEnterTag(AZ::u32 tagName)
    {
        AZ_Assert(m_data, "You must set a valid aggregator before we can process the data!");
        if (tagName == AZ_CRC("Slice"))
        {
            m_data->AddEvent(aznew EventTrace::SliceEvent());
            return this;
        }
        else if (tagName == AZ_CRC("Instant"))
        {
            m_data->AddEvent(aznew EventTrace::InstantEvent());
            return this;
        }
        else if (tagName == AZ_CRC("ThreadInfo"))
        {
            m_data->AddEvent(aznew EventTrace::ThreadInfoEvent());
            return this;
        }
        return nullptr;
    }

    void EventTraceDataParser::OnData(const AZ::Debug::DrillerSAXParser::Data& dataNode)
    {
        AZ_Assert(m_data, "You must set a valid aggregator before we can process the data!");

        DrillerEvent& drillerEvent = static_cast<DrillerEvent&>(*m_data->GetEvents().back());

        switch (drillerEvent.GetEventType())
        {
        case EventTrace::ET_SLICE:
        {
            EventTrace::SliceEvent& slice = static_cast<EventTrace::SliceEvent&>(drillerEvent);
            if (dataNode.m_name == AZ_CRC("Name"))
            {
                slice.m_Name = dataNode.ReadPooledString();
            }
            if (dataNode.m_name == AZ_CRC("Category"))
            {
                slice.m_Category = dataNode.ReadPooledString();
            }
            else if (dataNode.m_name == AZ_CRC("ThreadId"))
            {
                dataNode.Read(slice.m_ThreadId);
            }
            else if (dataNode.m_name == AZ_CRC("Timestamp"))
            {
                dataNode.Read(slice.m_Timestamp);
            }
            else if (dataNode.m_name == AZ_CRC("Duration"))
            {
                dataNode.Read(slice.m_Duration);
            }
        } break;

        case EventTrace::ET_INSTANT:
        {
            EventTrace::InstantEvent& instant = static_cast<EventTrace::InstantEvent&>(drillerEvent);
            if (dataNode.m_name == AZ_CRC("Name"))
            {
                instant.m_Name = dataNode.ReadPooledString();
            }
            if (dataNode.m_name == AZ_CRC("Category"))
            {
                instant.m_Category = dataNode.ReadPooledString();
            }
            else if (dataNode.m_name == AZ_CRC("ThreadId"))
            {
                dataNode.Read(instant.m_ThreadId);
            }
            else if (dataNode.m_name == AZ_CRC("Timestamp"))
            {
                dataNode.Read(instant.m_Timestamp);
            }

        } break;

        case EventTrace::ET_THREAD_INFO:
        {
            EventTrace::ThreadInfoEvent& threadInfo = static_cast<EventTrace::ThreadInfoEvent&>(drillerEvent);
            if (dataNode.m_name == AZ_CRC("Name"))
            {
                threadInfo.m_Name = dataNode.ReadPooledString();
            }
            else if (dataNode.m_name == AZ_CRC("ThreadId"))
            {
                dataNode.Read(threadInfo.m_ThreadId);
            }

        } break;
        }
    }
} // namespace Driller
