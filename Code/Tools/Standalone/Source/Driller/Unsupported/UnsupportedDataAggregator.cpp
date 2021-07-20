/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include "UnsupportedDataAggregator.hxx"
#include <Source/Driller/Unsupported/moc_UnsupportedDataAggregator.cpp>

#include "UnsupportedEvents.h"

#include <AzCore/std/containers/queue.h>

namespace Driller
{
    UnsupportedDataAggregator::UnsupportedDataAggregator(AZ::u32 drillerId)
        : m_parser(drillerId)
        , Aggregator(0)
    {
        m_parser.SetAggregator(this);
    }

    float UnsupportedDataAggregator::ValueAtFrame(FrameNumberType frame)
    {
        const float maxEventsPerFrame = 500.0f;
        float numEventsPerFrame = static_cast<float>(NumOfEventsAtFrame(frame));
        return AZStd::GetMin(numEventsPerFrame / maxEventsPerFrame, 1.0f) * 2.0f - 1.0f;
    }

    QColor UnsupportedDataAggregator::GetColor() const
    {
        return QColor(40, 40, 40);
    }

    QString UnsupportedDataAggregator::GetName() const
    {
        char buf[64];
        azsnprintf(buf, AZ_ARRAY_SIZE(buf), "Id: 0x%08x", m_parser.GetDrillerId());
        return buf;
    }

    QString UnsupportedDataAggregator::GetChannelName() const
    {
        return ChannelName();
    }

    QString UnsupportedDataAggregator::GetDescription() const
    {
        return QString("Unsupported driller");
    }

    QString UnsupportedDataAggregator::GetToolTip() const
    {
        return QString("Unknown Driller");
    }

    AZ::Uuid UnsupportedDataAggregator::GetID() const
    {
        return AZ::Uuid("{368D6FB2-9A92-4DFE-8DB4-4F106194BA6F}");
    }

    QWidget* UnsupportedDataAggregator::DrillDownRequest(FrameNumberType frame)
    {
        (void)frame;
        return NULL;
    }
    void UnsupportedDataAggregator::OptionsRequest()
    {
    }
} // namespace Driller
