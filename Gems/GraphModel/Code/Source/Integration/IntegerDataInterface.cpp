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
