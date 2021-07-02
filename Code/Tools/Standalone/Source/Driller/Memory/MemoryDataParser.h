/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef DRILLER_MEMORY_DRILLER_PARSER_H
#define DRILLER_MEMORY_DRILLER_PARSER_H

#include <AzCore/Driller/Stream.h>

namespace Driller
{
    class MemoryDataAggregator;

    class MemoryDrillerHandlerParser
        : public AZ::Debug::DrillerHandlerParser
    {
    public:
        enum SubTags
        {
            ST_NONE = 0,
            ST_REGISTER_ALLOCATOR,
            ST_REGISTER_ALLOCATION,
            ST_UNREGISTER_ALLOCATION,
            ST_RESIZE_ALLOCATION,
        };

        MemoryDrillerHandlerParser()
            : m_subTag(ST_NONE)
            , m_data(NULL)
        {}

        static AZ::u32 GetDrillerId()                   { return AZ_CRC("MemoryDriller", 0x1b31269d); }

        void SetAggregator(MemoryDataAggregator* data)  { m_data = data; }

        virtual AZ::Debug::DrillerHandlerParser* OnEnterTag(AZ::u32 tagName);
        virtual void OnExitTag(DrillerHandlerParser* handler, AZ::u32 tagName);
        virtual void OnData(const AZ::Debug::DrillerSAXParser::Data& dataNode);

    protected:
        SubTags m_subTag;
        MemoryDataAggregator* m_data;
    };
}

#endif
