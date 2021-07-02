/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef DRILLER_VRAM_DRILLER_PARSER_H
#define DRILLER_VRAM_DRILLER_PARSER_H

#include <AzCore/Driller/Stream.h>

namespace Driller
{
    namespace VRAM
    {
        //=========================================================================

        class VRAMDataAggregator;

        class VRAMDrillerHandlerParser
            : public AZ::Debug::DrillerHandlerParser
        {
        public:
            enum SubTags
            {
                ST_NONE = 0,
                ST_REGISTER_ALLOCATION,
                ST_UNREGISTER_ALLOCATION,
                ST_REGISTER_CATEGORY,
                ST_UNREGISTER_CATEGORY
            };

            VRAMDrillerHandlerParser()
                : m_subTag(ST_NONE)
                , m_data(NULL)
            {}

            static AZ::u32 GetDrillerId()
            {
                return AZ_CRC("VRAMDriller");
            }

            void SetAggregator(VRAMDataAggregator* data)
            {
                m_data = data;
            }

            virtual AZ::Debug::DrillerHandlerParser* OnEnterTag(AZ::u32 tagName);
            virtual void OnExitTag(DrillerHandlerParser* handler, AZ::u32 tagName);
            virtual void OnData(const AZ::Debug::DrillerSAXParser::Data& dataNode);

        protected:
            SubTags m_subTag;
            VRAMDataAggregator* m_data;
        };

        //=========================================================================
    }
}

#endif
