/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef DRILLER_CARRIER_DATAPARSER_H
#define DRILLER_CARRIER_DATAPARSER_H

#include <AzCore/Driller/Stream.h>
#include "CarrierDataEvents.h"

namespace Driller
{
    class CarrierDataAggregator;

    enum CarrierDataType
    {
        CDT_NONE,
        CDT_STATISTICS,
        CDT_LAST_SECOND,
        CDT_EFFECTIVE_LAST_SECOND
    };

    class CarrierDataParser
        : public AZ::Debug::DrillerHandlerParser
    {
    public:
        // The parser will add events to a CarrierDataAggregator instance.
        CarrierDataParser(CarrierDataAggregator* m_aggregator);
        virtual ~CarrierDataParser();

        // AZ::Debug::DrillerHandlerParser: Callbacks for entering, leaving and finding data
        // while parsing the driller XML data.
        AZ::Debug::DrillerHandlerParser* OnEnterTag(AZ::u32 tagName) override;
        void OnExitTag(AZ::Debug::DrillerHandlerParser* handler, AZ::u32 tagName) override;
        void OnData(const AZ::Debug::DrillerSAXParser::Data& dataNode) override;

    private:
        // The current tag type while parsing the driller XML data.
        CarrierDataType m_currentType;

        // The aggregator where events are added as a result of parsing the XML data.
        CarrierDataAggregator* m_aggregator;
    };
}

#endif
