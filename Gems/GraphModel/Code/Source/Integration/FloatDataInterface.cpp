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

#include <GraphModel/Integration/FloatDataInterface.h>
#include <GraphModel/Integration/IntegrationBus.h>

namespace GraphModelIntegration
{
    FloatDataInterface::FloatDataInterface(GraphModel::SlotPtr slot)
        : m_slot(slot)
    {
    }

    double FloatDataInterface::GetNumber() const
    {
        if (GraphModel::SlotPtr slot = m_slot.lock())
        {
            return slot->GetValue<float>();
        }
        else
        {
            return 0.0;
        }
    }
    void FloatDataInterface::SetNumber(double value)
    {
        if (GraphModel::SlotPtr slot = m_slot.lock())
        {
            if (static_cast<float>(value) != slot->GetValue<float>())
            {
                slot->SetValue(static_cast<float>(value));

                GraphCanvas::GraphId graphCanvasSceneId;
                IntegrationBus::BroadcastResult(graphCanvasSceneId, &IntegrationBusInterface::GetActiveGraphCanvasSceneId);
                IntegrationBus::Broadcast(&IntegrationBusInterface::SignalSceneDirty, graphCanvasSceneId);
            }
        }
    }

    int FloatDataInterface::GetDecimalPlaces() const
    {
        return 7;
    }
    int FloatDataInterface::GetDisplayDecimalPlaces() const
    {
        return 4;
    }

    double FloatDataInterface::GetMin() const
    {
        return std::numeric_limits<float>::lowest();
    }
    double FloatDataInterface::GetMax() const
    {
        return std::numeric_limits<float>::max();
    }
}
