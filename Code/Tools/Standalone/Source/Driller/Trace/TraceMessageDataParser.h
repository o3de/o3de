/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
