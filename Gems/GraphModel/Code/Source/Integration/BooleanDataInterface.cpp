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

#include <GraphModel/Integration/BooleanDataInterface.h>
#include <GraphModel/Integration/IntegrationBus.h>

namespace GraphModelIntegration
{
    BooleanDataInterface::BooleanDataInterface(GraphModel::SlotPtr slot)
        : m_slot(slot)
    {
    }

    bool BooleanDataInterface::GetBool() const
    {
        if (GraphModel::SlotPtr slot = m_slot.lock())
        {
            return slot->GetValue<bool>();
        }
        else
        {
            return false;
        }
    }

    void BooleanDataInterface::SetBool(bool enabled)
    {
        if (GraphModel::SlotPtr slot = m_slot.lock())
        {
            if (enabled != slot->GetValue<bool>())
            {
                slot->SetValue(enabled);

                GraphCanvas::GraphId graphCanvasSceneId;
                IntegrationBus::BroadcastResult(graphCanvasSceneId, &IntegrationBusInterface::GetActiveGraphCanvasSceneId);
                IntegrationBus::Broadcast(&IntegrationBusInterface::SignalSceneDirty, graphCanvasSceneId);
            }
        }
    }
}
