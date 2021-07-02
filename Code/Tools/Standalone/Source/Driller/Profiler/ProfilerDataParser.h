/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef DRILLER_PROFILER_DRILLER_PARSER_H
#define DRILLER_PROFILER_DRILLER_PARSER_H

#include <AzCore/Driller/Stream.h>

namespace Driller
{
    class ProfilerDataAggregator;

    class ProfilerDrillerHandlerParser
        : public AZ::Debug::DrillerHandlerParser
    {
    public:
        enum SubTags
        {
            ST_NONE = 0,
            ST_NEW_REGSTER,
            ST_UPDATE_REGSTER,
            ST_ENTER_THREAD,
            ST_EXIT_THREAD,
            ST_REGISTER_SYSTEM,
            ST_UNREGISTER_SYSTEM,
        };

        ProfilerDrillerHandlerParser()
            : m_subTag(ST_NONE)
            , m_data(NULL)
        {}

        static AZ::u32 GetDrillerId()                       { return AZ_CRC("ProfilerDriller", 0x172c5268); }

        void SetAggregator(ProfilerDataAggregator* data)    { m_data = data; }

        virtual AZ::Debug::DrillerHandlerParser* OnEnterTag(AZ::u32 tagName);
        virtual void OnExitTag(DrillerHandlerParser* handler, AZ::u32 tagName);
        virtual void OnData(const AZ::Debug::DrillerSAXParser::Data& dataNode);

    protected:
        SubTags  m_subTag;
        ProfilerDataAggregator* m_data;
    };
}

#endif
