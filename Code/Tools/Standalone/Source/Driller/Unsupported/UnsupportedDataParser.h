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
