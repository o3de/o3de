/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include "TraceMessageDataParser.h"
#include "TraceMessageDataAggregator.hxx"
#include "TraceMessageEvents.h"

namespace Driller
{
    AZ::Debug::DrillerHandlerParser* TraceMessageHandlerParser::OnEnterTag(AZ::u32 tagName)
    {
        AZ_Assert(m_data, "You must set a valid aggregator before we can process the data!");
        if (tagName == AZ_CRC("OnPrintf", 0xd4b5c294))
        {
            m_data->AddEvent(aznew TraceMessageEvent(TraceMessageEvent::ET_PRINTF));
            return this;
        }
        else if (tagName == AZ_CRC("OnWarning", 0x7d90abea))
        {
            m_data->AddEvent(aznew TraceMessageEvent(TraceMessageEvent::ET_WARNING));
            return this;
        }
        else if (tagName == AZ_CRC("OnError", 0x4993c634))
        {
            m_data->AddEvent(aznew TraceMessageEvent(TraceMessageEvent::ET_ERROR));
            return this;
        }
        return nullptr;
    }

    void TraceMessageHandlerParser::OnData(const AZ::Debug::DrillerSAXParser::Data& dataNode)
    {
        AZ_Assert(m_data, "You must set a valid aggregator before we can process the data!");
        if (dataNode.m_name == AZ_CRC("Window", 0x8be4f9dd))
        {
            TraceMessageEvent* event = static_cast<TraceMessageEvent*>(m_data->GetEvents().back());
            event->m_window = dataNode.ReadPooledString();
            event->ComputeCRC();
        }
        if (dataNode.m_name == AZ_CRC("Message", 0xb6bd307f))
        {
            TraceMessageEvent* event = static_cast<TraceMessageEvent*>(m_data->GetEvents().back());
            event->m_message = dataNode.ReadPooledString();
        }
        else if (dataNode.m_name == AZ_CRC("OnAssert", 0xb74db4ce))
        {
            TraceMessageEvent* event = aznew TraceMessageEvent(TraceMessageEvent::ET_ASSERT);
            event->m_window = "System";
            event->m_message = dataNode.ReadPooledString();
            event->m_windowCRC = AZ_CRC("System", 0xc94d118b);
            m_data->AddEvent(event);
        }
        else if (dataNode.m_name == AZ_CRC("OnException", 0xfe457d12))
        {
            TraceMessageEvent* event = aznew TraceMessageEvent(TraceMessageEvent::ET_EXCEPTION);
            event->m_window = "System";
            event->m_message = dataNode.ReadPooledString();
            event->m_windowCRC = AZ_CRC("System", 0xc94d118b);
            m_data->AddEvent(event);
        }
    }
} // namespace Driller
