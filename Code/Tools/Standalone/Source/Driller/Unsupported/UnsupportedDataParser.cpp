/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

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
