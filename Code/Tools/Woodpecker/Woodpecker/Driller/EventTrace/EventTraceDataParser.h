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
