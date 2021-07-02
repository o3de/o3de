/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Driller/Stream.h>

namespace Driller
{
    class EventTraceDataAggregator;

    class EventTraceDataParser
        : public AZ::Debug::DrillerHandlerParser
    {
    public:
        EventTraceDataParser()
            : m_data(NULL)
        {}

        static AZ::u32 GetDrillerId()
        {
            return AZ_CRC("EventTraceDriller");
        }

        void SetAggregator(EventTraceDataAggregator* data)
        {
            m_data = data;
        }

        virtual AZ::Debug::DrillerHandlerParser* OnEnterTag(AZ::u32 tagName);
        virtual void OnData(const AZ::Debug::DrillerSAXParser::Data& dataNode);

    protected:
        EventTraceDataAggregator* m_data;
    };
}
