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
