/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include "CarrierDataEvents.h"
#include "CarrierDataAggregator.hxx"
#include "CarrierDataParser.h"

namespace Driller
{
    CarrierDataParser::CarrierDataParser(CarrierDataAggregator* m_aggregator)
        : DrillerHandlerParser(false)
        , m_currentType(CDT_NONE)
        , m_aggregator(m_aggregator)
    {
    }

    CarrierDataParser::~CarrierDataParser()
    {
    }

    AZ::Debug::DrillerHandlerParser* CarrierDataParser::OnEnterTag(AZ::u32 tagName)
    {
        // Start of a new event.
        if (tagName == AZ_CRC("Statistics"))
        {
            m_currentType = CDT_STATISTICS;
            m_aggregator->AddEvent(aznew CarrierDataEvent);
            return this;
        }
        else if (tagName == AZ_CRC("LastSecond"))
        {
            m_currentType = CDT_LAST_SECOND;
            return this;
        }
        else if (tagName == AZ_CRC("EffectiveLastSecond"))
        {
            m_currentType = CDT_EFFECTIVE_LAST_SECOND;
            return this;
        }
        else
        {
            return nullptr;
        }
    }

    void CarrierDataParser::OnExitTag(AZ::Debug::DrillerHandlerParser* handler, AZ::u32 tagName)
    {
        (void)handler;

        if (tagName == AZ_CRC("LastSecond") || tagName == AZ_CRC("EffectiveLastSecond"))
        {
            m_currentType = CDT_NONE;
        }
    }

    void CarrierDataParser::OnData(const AZ::Debug::DrillerSAXParser::Data& dataNode)
    {
        // Ignore unsupported tags.
        if (m_currentType == CDT_NONE)
        {
            return;
        }

        CarrierDataEvent* event = static_cast<CarrierDataEvent*>(m_aggregator->GetEvents().back());

        if (m_currentType == CDT_STATISTICS)
        {
            dataNode.Read(event->mID);
            return;
        }

        CarrierData* eventData = nullptr;
        switch (m_currentType)
        {
        case CDT_LAST_SECOND:
            eventData = &event->mLastSecond;
            break;
        case CDT_EFFECTIVE_LAST_SECOND:
            eventData = &event->mEffectiveLastSecond;
            break;
        default:
            return;
        }

        if (dataNode.m_name == AZ_CRC("DataSend"))
        {
            dataNode.Read(eventData->mDataSend);
        }
        else if (dataNode.m_name == AZ_CRC("DataReceived"))
        {
            dataNode.Read(eventData->mDataReceived);
        }
        else if (dataNode.m_name == AZ_CRC("DataResent"))
        {
            dataNode.Read(eventData->mDataResent);
        }
        else if (dataNode.m_name == AZ_CRC("DataAcked"))
        {
            dataNode.Read(eventData->mDataAcked);
        }
        else if (dataNode.m_name == AZ_CRC("PacketSend"))
        {
            dataNode.Read(eventData->mPacketSend);
        }
        else if (dataNode.m_name == AZ_CRC("PacketReceived"))
        {
            dataNode.Read(eventData->mPacketReceived);
        }
        else if (dataNode.m_name == AZ_CRC("PacketLost"))
        {
            dataNode.Read(eventData->mPacketLost);
        }
        else if (dataNode.m_name == AZ_CRC("PacketAcked"))
        {
            dataNode.Read(eventData->mPacketAcked);
        }
        else if (dataNode.m_name == AZ_CRC("PacketLoss"))
        {
            dataNode.Read(eventData->mPacketLoss);
        }
        else if (dataNode.m_name == AZ_CRC("rtt"))
        {
            dataNode.Read(eventData->mRTT);
        }
    }
}
