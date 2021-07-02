/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef DRILLER_UNSUPPORTED_PARSER_H
#define DRILLER_UNSUPPORTED_PARSER_H

#include <AzCore/Driller/Stream.h>

namespace Driller
{
    class UnsupportedDataAggregator;

    class UnsupportedHandlerParser
        : public AZ::Debug::DrillerHandlerParser
    {
    public:
        UnsupportedHandlerParser(AZ::u32 drillerId)
            : DrillerHandlerParser(false)
            , m_drillerId(drillerId)
            , m_data(nullptr)
        {
        }

        AZ::u32 GetDrillerId() const                            { return m_drillerId; }
        void SetAggregator(UnsupportedDataAggregator* data)     { m_data = data; }

        // AZ::Debug::DrillerHandlerParser
        virtual AZ::Debug::DrillerHandlerParser* OnEnterTag(AZ::u32 tagName);

    protected:
        AZ::u32 m_drillerId;
        UnsupportedDataAggregator* m_data;
    };
}

#endif
