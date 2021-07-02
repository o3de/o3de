/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
