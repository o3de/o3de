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

#include "Woodpecker_precompiled.h"

#include "UnsupportedDataParser.h"
#include "UnsupportedDataAggregator.hxx"
#include "UnsupportedEvents.h"

namespace Driller
{
    AZ::Debug::DrillerHandlerParser* UnsupportedHandlerParser::OnEnterTag(AZ::u32 tagName)
    {
        AZ_Assert(m_data, "You must set a valid aggregator before we can process the data!");
        m_data->AddEvent(aznew UnsupportedEvent(tagName));
        return nullptr;
    }
} // namespace Driller
