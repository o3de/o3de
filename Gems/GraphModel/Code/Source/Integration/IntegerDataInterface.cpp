/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GraphModel/Integration/IntegerDataInterface.h>
#include <GraphModel/Integration/IntegrationBus.h>

namespace GraphModelIntegration
{
    IntegerDataInterface::IntegerDataInterface(GraphModel::SlotPtr slot)
        : m_slot(slot)
    {
    }

    double IntegerDataInterface::GetNumber() const
    {
        if (GraphModel::SlotPtr slot = m_slot.lock())
        {
            return slot->GetValue<int>();
        }
        else
        {
            return 0.0;
        }
    }
    void IntegerDataInterface::SetNumber(double value)
    {
        if (GraphModel::SlotPtr slot = m_slot.lock())
        {
            if (static_cast<int>(value) != slot->GetValue<int>())
            {
                slot->SetValue(static_cast<int>(value));

                GraphCanvas::GraphId graphCanvasSceneId;
                IntegrationBus::BroadcastResult(graphCanvasSceneId, &IntegrationBusInterface::GetActiveGraphCanvasSceneId);
                IntegrationBus::Broadcast(&IntegrationBusInterface::SignalSceneDirty, graphCanvasSceneId);
            }
        }
    }

    int IntegerDataInterface::GetDecimalPlaces() const
    {
        return 0;
    }
    int IntegerDataInterface::GetDisplayDecimalPlaces() const
    {
        return 0;
    }
    double IntegerDataInterface::GetMin() const
    {
        return std::numeric_limits<int>::min();
    }
    double IntegerDataInterface::GetMax() const
    {
        return std::numeric_limits<int>::max();
    }
}
