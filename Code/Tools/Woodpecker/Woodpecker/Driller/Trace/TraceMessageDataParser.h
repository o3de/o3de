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

#ifndef DRILLER_TRACE_MESSAGE_PARSER_H
#define DRILLER_TRACE_MESSAGE_PARSER_H

#include <AzCore/Driller/Stream.h>

namespace Driller
{
    class TraceMessageDataAggregator;

    class TraceMessageHandlerParser
        : public AZ::Debug::DrillerHandlerParser
    {
    public:
        TraceMessageHandlerParser()
            : m_data(nullptr)
        {
        }

        static AZ::u32 GetDrillerId()                           { return AZ_CRC("TraceMessagesDriller", 0xa61d1b00); }
        void SetAggregator(TraceMessageDataAggregator* data)    { m_data = data; }

        // AZ::Debug::DrillerHandlerParser
        virtual AZ::Debug::DrillerHandlerParser* OnEnterTag(AZ::u32 tagName);
        virtual void                             OnData(const AZ::Debug::DrillerSAXParser::Data& dataNode);

    protected:
        TraceMessageDataAggregator* m_data;
    };
}

#endif
